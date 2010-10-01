#include "MDBShared.h"
#include "MDBMachReader.h"
#include "MDBLog.h"

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>
#include <mach-o/ranlib.h>
#include <mach-o/reloc.h>

bool
MDBMachReader::read(const char* fileName) {
    log.traceLn("loading symbols from %s", fileName);
    FILE *file = fopen(fileName, "rb");
    RETURN_VALUE_ON_ERROR(file, "Cannot open file for reading.", false);
    fseek(file, 0, SEEK_END);
    bufferSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = (char *)malloc(bufferSize);
    fread(buffer, bufferSize, 1, file);
    fclose(file);
    
    char *curr = buffer;    
    struct mach_header *header = (struct mach_header *)curr;
    if (header->magic != MH_MAGIC) {
        error("Can only read 32 bit Mach-O files.");
        return false;
    }
    
    if (!(header->filetype == MH_EXECUTE ||
          header->filetype == MH_DYLIB)) {
        error("Can only process executable or library Mach-O files.");
        return false;
    }

    struct symtab_command *symtabCommand = NULL;
    curr += sizeof(mach_header);
    for (uint32_t i = 0; i < header->ncmds; i++) {
        struct load_command *loadCommand = (struct load_command *)curr;
        log.traceLn("loading command 0x%X, %d of %d", loadCommand->cmd, i + 1, header->ncmds);
        if (loadCommand->cmd == LC_SYMTAB) {
            symtabCommand = (struct symtab_command *)loadCommand;
        } else if(loadCommand->cmd == LC_SEGMENT) {
            // struct segment_command *segment_command = (struct segment_command *)curr;
            char *sectCurr = curr + sizeof(struct segment_command);
            struct section *section = (struct section *)sectCurr;
            if (strcmp(section->segname, "__TEXT") == 0 &&
                strcmp(section->sectname, "__text") == 0) {
                this->symbols.append(new MDBMachSymbol(ENTRY_POINT_SYMBOL_NAME, section->addr));
            }
        }
        curr += loadCommand->cmdsize;
    }
    
    RETURN_VALUE_ON_ERROR(symtabCommand, "Cannot find symbol table.", false);

    struct nlist *symbols = (struct nlist *)(buffer + symtabCommand->symoff);
    char *stringTable = buffer + symtabCommand->stroff;
    
    for (uint32_t i = 0; i < symtabCommand->nsyms; i++) {
        struct nlist &symbol = symbols[i];
        log.traceLn("symbol name %s", stringTable + symbol.n_un.n_strx);
        if ((symbol.n_type & N_TYPE) == N_SECT) {
            if (symbol.n_sect == 1) {
                // TODO: We only support code symbols for now.
                this->symbols.append(new MDBMachSymbol(stringTable + symbol.n_un.n_strx, symbol.n_value));
            }
            log.traceLn("symbol defined in section %d at 0x%X.", symbol.n_sect, symbol.n_value);
        }
    }
    return true;
}

MDBMachReader::~MDBMachReader() {
    free(buffer);
}
