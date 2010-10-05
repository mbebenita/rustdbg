#ifndef MDBCODE_H
#define MDBCODE_H

#include "MDBMachReader.h"

class MDBInstruction {
    MDBInstruction();
};

class MDBCodeRegion {
public:
    MDBCode *code;
    uintptr_t address;
    MDBSymbol *symbol;
    size_t size;
    MDBCodeRegion(MDBCode *code, uintptr_t address, MDBSymbol *symbol, size_t size);
    int disassemble(char *buffer, size_t bufferSize);
    static MDBCodeRegion *fromProcedure(MDBDebugger *debugger, uintptr_t address);

    /**
     * Returns the offset of the specified address from the region start
     * address, or a negative number if the address is outside of the region.
     */
    int getOffset(uintptr_t address);
    bool contains(uintptr_t address);
};

class MDBCode {
bool scanProcedure(uintptr_t address);
public:
    MBList<MDBMachFile *> files;
    MBList<MDBCodeRegion *> regions;
    MDBDebugger *debugger;
    MDBCode(MDBDebugger *debugger);
    MDBCodeRegion *getRegion(uintptr_t address);
    bool loadSymbols(const char *fileName);
    MDBSymbol *findSymbolByNameOrAddress(const char *symbolName, uintptr_t address);
    size_t getInstructionLength(uintptr_t address);
    bool isCallInstruction(uintptr_t address);
    void logState();
};

#endif
