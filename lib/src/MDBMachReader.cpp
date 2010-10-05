#include "MDBShared.h"
#include "MDBMachReader.h"
#include "MDBLog.h"

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>
#include <mach-o/ranlib.h>
#include <mach-o/reloc.h>
#include <mach-o/stab.h>

uint32_t swapEndian(uint32_t x) {
    return (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
}

MDBMachFile::MDBMachFile(const char* fileName) : fileName(fileName) {
    // Nop.
}

bool
MDBMachFile::parse() {
    log.traceLn(MDBLog::SYM, "loading symbols from %s", fileName);
    FILE *file = fopen(fileName, "rb");
    RETURN_VALUE_ON_ERROR(file, "Cannot open file for reading.", false);
    fseek(file, 0, SEEK_END);
    bufferSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = (char *)malloc(bufferSize);
    fread(buffer, bufferSize, 1, file);
    fclose(file);
    
    char *curr = buffer;
    char *image = buffer;
    struct fat_header *fatHeader = (struct fat_header *)curr;
    if (fatHeader->magic == FAT_CIGAM) {
        bool foundX86Image = false;
        curr += sizeof(struct fat_header);
        log.traceLn("fat binary, looking for 32 bit X86 Mach-O image, %d images found", swapEndian(fatHeader->nfat_arch));
        for(uint32_t i = 0; i < swapEndian(fatHeader->nfat_arch); i++) {
            struct fat_arch *fatArch = (struct fat_arch *)curr;
            if (swapEndian(fatArch->cputype) == CPU_TYPE_X86) {
                foundX86Image = true;
                image = buffer + swapEndian(fatArch->offset);
                break;
            } else {
                curr += sizeof(struct fat_arch);
            }
        }
        if (foundX86Image == false) {
            error("Cannot find X86 Mach-O image.");
            return false;
        }
    }
    curr = image;
    struct mach_header *header = (struct mach_header *)curr;
    if (header->magic != MH_MAGIC) {
        error("Can only read 32 bit X86 Mach-O files.");
        return false;
    }
    
    if (!(header->filetype == MH_EXECUTE || header->filetype == MH_DYLIB || header->filetype == MH_DYLINKER)) {
        error("Can only process executable or library Mach-O files.");
        return false;
    }

    struct symtab_command *symtabCommand = NULL;
    curr += sizeof(mach_header);
    for (uint32_t i = 0; i < header->ncmds; i++) {
        struct load_command *loadCommand = (struct load_command *)curr;
        log.traceLn(MDBLog::SYM, "loading command 0x%X, %d of %d at 0x%X", loadCommand->cmd, i + 1, header->ncmds, (size_t) (curr - buffer));
        if (loadCommand->cmd == LC_SYMTAB) {
            symtabCommand = (struct symtab_command *)loadCommand;
        } else if(loadCommand->cmd == LC_SEGMENT) {
            // struct segment_command *segment_command = (struct segment_command *)curr;
            char *sectCurr = curr + sizeof(struct segment_command);
            struct section *section = (struct section *)sectCurr;
            if (strcmp(section->segname, "__TEXT") == 0 &&
                strcmp(section->sectname, "__text") == 0) {
                this->symbols.append(new MDBSymbol(ENTRY_POINT_SYMBOL_NAME, this, section->addr));
            }
        } else if (loadCommand->cmd == LC_LOAD_DYLIB) {
            struct dylib_command *dylibCommand = (struct dylib_command *)loadCommand;
            const char *dylibName = (char *) dylibCommand + dylibCommand->dylib.name.offset;
            log.traceLn(MDBLog::SYM, "loading dylib %s", dylibName);
            this->dylibs.append(new MDBDylib(dylibName));
        }
        curr += loadCommand->cmdsize;
    }
    
    RETURN_VALUE_ON_ERROR(symtabCommand, "Cannot find symbol table.", false);

    struct nlist *symbols = (struct nlist *)(image + symtabCommand->symoff);
    char *stringTable = image + symtabCommand->stroff;
    
    // log.traceLn(MDBLog::SYM, "loading 0x%X", (size_t) ((char *)&(symtabCommand->nsyms) - buffer));
    log.traceLn(MDBLog::SYM, "loading %d symbols", symtabCommand->nsyms);
    for (uint32_t i = 0; i < symtabCommand->nsyms; i++) {
        struct nlist &symbol = symbols[i];
        // log.traceLn(MDBLog::SYM, "loading 0x%X", (size_t) ((char *)(&symbol) - buffer));
        if (symbol.n_sect > 0 && symbol.n_un.n_strx > 1) {
            this->symbols.append(new MDBSymbol(stringTable + symbol.n_un.n_strx, this, symbol.n_value));
            // log.traceLn(MDBLog::SYM, "loaded symbol %s defined in section %d at 0x%X.", stringTable + symbol.n_un.n_strx, symbol.n_sect, symbol.n_value);
        }
    }
    return true;
}

MDBMachFile::~MDBMachFile() {
    free(buffer);
}

const char*
MDBMachFile::findImageName(uintptr_t image) {
    char *curr = (char *)image;
    struct mach_header *header = (struct mach_header *)curr;
    if (header->magic == MH_MAGIC) {
        curr += sizeof(struct mach_header);
        for (uint32_t i = 0; i < header->ncmds; i++) {
            struct load_command *loadCommand = (struct load_command *)curr;
            if (loadCommand->cmd == LC_ID_DYLIB) {
                struct dylib_command *dylibCommand = (struct dylib_command *)loadCommand;
                const char *dylibName = (char *) dylibCommand + dylibCommand->dylib.name.offset;
                return dylibName;
            } else if (loadCommand->cmd == LC_ID_DYLINKER) {
                struct dylinker_command *dylinkerCommand = (struct dylinker_command *)loadCommand;
                const char *dylinkerName = (char *) dylinkerCommand + dylinkerCommand->name.offset;
                return dylinkerName;
            }
            curr += loadCommand->cmdsize;
        }
    }
    return NULL;
}
