#ifndef MDBCODE_H
#define MDBCODE_H

#include "MDBMachReader.h"

class MDBInstruction {
    MDBInstruction();
};

class MDBCodeRegion {
public:
    MDBCodeRegionManager *code;
    uintptr_t address;
    MDBMachSymbol *symbol;
    size_t size;
    MDBCodeRegion(MDBCodeRegionManager *code, uintptr_t address, MDBMachSymbol *symbol, size_t size);
    int disassemble(char *buffer, size_t bufferSize);
    static MDBCodeRegion *fromProcedure(MDBDebugger *debugger, uintptr_t address);

    /**
     * Returns the offset of the specified address from the region start
     * address, or a negative number if the address is outside of the region.
     */
    int getOffset(uintptr_t address);
    bool contains(uintptr_t address);
};

class MDBCodeRegionManager {
bool scanProcedure(uintptr_t address);
public:
    ArrayList<MDBMachReader *> machFiles;
    ArrayList<MDBCodeRegion *> regions;
    MDBDebugger *debugger;
    MDBCodeRegionManager(MDBDebugger *debugger);
    MDBCodeRegion *getRegion(uintptr_t address);
    bool loadSymbols(const char *fileName);
    MDBMachSymbol *findSymbolByNameOrAddress(const char *symbolName, uintptr_t address);
    size_t getInstructionLength(uintptr_t address);
    bool isCallInstruction(uintptr_t address);
};

#endif
