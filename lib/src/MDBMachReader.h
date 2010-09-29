#ifndef MDBMACHREADER_H
#define MDBMACHREADER_H

class MDBMachSymbol {
public:
    const char *name;
    uintptr_t address;
    MDBMachSymbol(const char *name, uintptr_t address) : 
        name(name), address(address) {
        // Nop.
    }
};

class MDBMachReader {
    char *buffer;
    size_t bufferSize;
public:
    ArrayList<MDBMachSymbol *> symbols;
    bool read(const char* fileName);
    ~MDBMachReader();
};

#endif
