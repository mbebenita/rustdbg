#ifndef MDBMACHREADER_H
#define MDBMACHREADER_H

class MDBMachFile;

class MDBSymbol {
public:
    const char *name;
    MDBMachFile *file;
    uintptr_t address;
    MDBSymbol(const char *name, MDBMachFile *file, uintptr_t address) :
        name(name), file(file), address(address) {
        // Nop.
    }
};

class MDBDylib {
public:
    const char *fileName;
    MDBDylib(const char *fileName) :
        fileName(fileName) {
        // Nop.
    }
};

class MDBMachFile {
    char *buffer;
    size_t bufferSize;
public:
    const char *fileName;
    MBList<MDBSymbol *> symbols;
    MBList<MDBDylib *> dylibs;
    MDBMachFile(const char* fileName);
    bool parse();
    ~MDBMachFile();
    static const char *findImageName(uintptr_t address);
};

#endif
