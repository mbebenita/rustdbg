#include "MDBShared.h"
#include "MDBCode.h"
#include "MDBDebugger.h"
#include "udis86.h"

MDBCodeRegion::MDBCodeRegion(MDBCodeRegionManager *code, uintptr_t address, MDBMachSymbol *symbol, size_t size) :
    code(code), address(address), symbol(symbol), size(size) {
    
};

int
MDBCodeRegion::disassemble(char *buffer, size_t bufferSize) {      
    void *localBuffer = malloc(size);
    code->debugger->read(localBuffer, address, size);
    ud_t ud_obj;
    ud_init(&ud_obj); 
    ud_set_input_buffer(&ud_obj, (uint8_t*)localBuffer, size);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    uint32_t remainingBytes = size;
    do {
        int pc = ud_obj.pc;
        size_t insnSize = ud_disassemble(&ud_obj);
        remainingBytes -= insnSize;
        appendString(buffer, "0x%llx: %-16s %s\n", 
                     address + pc, ud_insn_hex(&ud_obj), ud_insn_asm(&ud_obj));
    } while (remainingBytes > 0);
    free(localBuffer);
    return 0;
}

MDBCodeRegion *
MDBCodeRegion::fromProcedure(MDBDebugger *debugger, uintptr_t address) {
    size_t bufferSize = 128;
    char buffer[bufferSize]; 
    
    if (debugger->read(buffer, address, sizeof(buffer)) != (int) sizeof(buffer)) {
        log("Cannot read procedure memory.");
    }
    
    ud_t ud_obj;
    ud_init(&ud_obj); 
    ud_set_input_buffer(&ud_obj, (uint8_t*)buffer, bufferSize);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    
    size_t remaining = bufferSize;
    do {
        int pc = ud_obj.pc;
        size_t instructionSize = ud_disassemble(&ud_obj);
        remaining -= instructionSize;
        log("decoded: 0x%llx %s", pc, ud_insn_asm(&ud_obj));
    } while (remaining > 0);
    return NULL;
}

bool
MDBCodeRegion::contains(uintptr_t address) {
    return getOffset(address) >= 0;
}

int32_t
MDBCodeRegion::getOffset(uintptr_t address) {
    int32_t offset = address - this->address;
    if (offset > (int) size) {
        return -1;
    }
    return offset;
}

MDBCodeRegionManager::MDBCodeRegionManager(MDBDebugger *debugger) : 
    debugger(debugger) {
    // Nop.
};

MDBCodeRegion *
MDBCodeRegionManager::getRegion(uintptr_t address) {
    for (uint32_t i = 0; i < regions.length(); i++) {
        if (regions[i]->contains(address)) {
            return regions[i];
        }
    }
    return NULL;
}

bool
MDBCodeRegionManager::loadSymbols(const char *fileName) {
    MDBMachReader *machFile = new MDBMachReader();
    if (machFile->read(fileName) == false) {
        return false;
    }
    machFiles.add(machFile);
    for (uint32_t i = 0; i < machFile->symbols.length(); i++) {
        MDBMachSymbol *symbol = machFile->symbols[i];
        log("scanning symbol %s", symbol->name);
        scanProcedure(symbol->address);
    }
    return true;
}

MDBMachSymbol *
MDBCodeRegionManager::findSymbolByNameOrAddress(const char *symbolName, uintptr_t address) {
    for (uint32_t i = 0; i < machFiles.length(); i++) {
        MDBMachReader *machFile = machFiles[i];    
        for (uint32_t j = 0; j < machFile->symbols.length(); j++) {
            MDBMachSymbol *symbol = machFile->symbols[j];
            if (strcmp(symbol->name, symbolName ? symbolName : "") == 0 ||
                symbol->address == address) {
                return symbol;
            }
        }
    }
    return NULL;
}

size_t
MDBCodeRegionManager::getInstructionLength(uintptr_t address) {
    char buffer[32];
    if (debugger->read(buffer, address, sizeof(buffer)) != sizeof(buffer)) {
        log("Cannot read instruction memory.");
    }
    ud_t insn;
    ud_init(&insn); 
    ud_set_input_buffer(&insn, (uint8_t*)buffer, 32);
    ud_set_mode(&insn, 32);
    ud_set_syntax(&insn, UD_SYN_INTEL);
    size_t length = ud_disassemble(&insn);
    log("Instruction at 0x%X is %d bytes long.", address, length);
    return length;
}

bool 
MDBCodeRegionManager::isCallInstruction(uintptr_t address) {
    char buffer[32];
    if (debugger->read(buffer, address, sizeof(buffer)) != sizeof(buffer)) {
        log("Cannot read instruction memory.");
    }
    ud_t insn;
    ud_init(&insn); 
    ud_set_input_buffer(&insn, (uint8_t*)buffer, 32);
    ud_set_mode(&insn, 32);
    ud_set_syntax(&insn, UD_SYN_INTEL);
    size_t length = ud_disassemble(&insn);
    log("Instruction at 0x%X is %d bytes long.", address, length);
    return insn.mnemonic == UD_Icall;
}


/**
 * Recursively generate code regions by statically analyzing machine code.
 */
bool
MDBCodeRegionManager::scanProcedure(uintptr_t address) {
    if (getRegion(address)) {
        log("a regoin already exists");
        // A region already exists for this address.
        return true;
    }

    log("scanning address %llx", address);
    
    size_t bufferSize = 1024 * 8;
    char buffer[bufferSize];
    
    if (debugger->read(buffer, address, sizeof(buffer)) != (int) sizeof(buffer)) {
        log("Cannot read procedure memory.");
    }
    
    ArrayList<uintptr_t> callTargets;
    
    ud_t insn;
    ud_init(&insn); 
    ud_set_input_buffer(&insn, (uint8_t*)buffer, bufferSize);
    ud_set_mode(&insn, 32);
    ud_set_syntax(&insn, UD_SYN_INTEL);
    
    size_t remainingBytes = bufferSize;
    while (remainingBytes > 0) {
        int offset = insn.pc;
        size_t insnSize = ud_disassemble(&insn);
        remainingBytes -= insnSize;
        if (insnSize == 0) {
            error("cannot disassemble");
            break;
        }
        log("decoded: 0x%-8llx 0x%-8llx %s", 
            address + offset,
            offset,
            ud_insn_asm(&insn));
        switch (insn.mnemonic) {
            case UD_Icall: {
                uintptr_t target = 0;
                if (insn.operand[0].type == UD_OP_JIMM) {
                    if (insn.operand[0].size == 32) {
                        target = address + insn.pc + insn.operand[0].lval.sdword;
                    }
                }
                if (target) {
                    if (callTargets.contains(target) == false) {
                        log("discovered new call target 0x%llx", target);
                        callTargets.append(target);
                    }
                }
                break;
            }
            case UD_Iret: {
                MDBCodeRegion *region = new MDBCodeRegion(this, address, findSymbolByNameOrAddress(NULL, address), insn.pc);
                regions.append(region);
                remainingBytes = 0;
                break;
            }
            default:
                break;
        }
    }
    
    for (uint32_t i = 0; i < callTargets.length(); i++) {
        scanProcedure(callTargets[i]);
    }
    
    return true;
}

MDBInstruction::MDBInstruction() {
    
};
