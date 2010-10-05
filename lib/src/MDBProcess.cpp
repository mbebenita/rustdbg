#include "MDBShared.h"
#include "MDBDebugger.h"
#include "MDBBreakpoint.h"
#include "MDBProcess.h"
#include "MDBLog.h"
#include "MDBMachReader.h"
#include "MDBCode.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>
#include <mach-o/ranlib.h>
#include <mach-o/reloc.h>
#include <mach-o/stab.h>
#include <mach-o/dyld_images.h>

MDBProcess::MDBProcess(MDBDebugger *debugger, mach_port_t task) : debugger(debugger), task(task) {
    // Nop.
}

MDBProcess::~MDBProcess() {
    // TODO Auto-generated destructor stub
}

bool
MDBProcess::read_overwrite(void *dst, vm_address_t src, vm_size_t size) {
    mach_vm_size_t read;
    kern_return_t kr = mach_vm_read_overwrite(task, src, size, (vm_address_t) dst, &read);
    REPORT_MACH_ERROR("mach_vm_read_overwrite", kr);
    if (kr != KERN_SUCCESS) {
        log.traceLn("Failed to read %d bytes from address 0x%llx.", size, src);
    }
    return kr == KERN_SUCCESS;
}

bool
MDBProcess::read(mach_vm_address_t *dst, mach_vm_address_t src, mach_vm_size_t size) {
    vm_offset_t data;
    mach_msg_type_number_t read;
    kern_return_t kr = mach_vm_read(task, src, size, &data, &read);
    REPORT_MACH_ERROR("mach_vm_read", kr);
    if (kr != KERN_SUCCESS) {
        log.traceLn("Failed to read %d bytes from address 0x%llx.", size, src);
    } else {
        // log.traceLn("Read %d bytes from address 0x%llx.", size, src);
    }
    *dst = data;
    return kr == KERN_SUCCESS;
}

bool
MDBProcess::free(mach_vm_address_t data, mach_vm_size_t size) {
    return mach_vm_deallocate(mach_task_self(), data, size);
}

const char *inheritanceName(vm_inherit_t i) {
    switch (i) {
    case VM_INHERIT_SHARE:
        return "share";
    case VM_INHERIT_COPY:
        return "copy";
    case VM_INHERIT_NONE:
        return "none";
    default:
        return "???";
    }
}

const char *behaviorName(vm_behavior_t i) {
    switch (i) {
    case VM_BEHAVIOR_CAN_REUSE:
        return "can reuse";
    case VM_BEHAVIOR_DEFAULT:
        return "default";
    case VM_BEHAVIOR_DONTNEED:
        return "don't need";
    case VM_BEHAVIOR_FREE:
        return "free";
    default:
        return "???";
    }
}

void
MDBProcess::logState() {
    kern_return_t kr;
    mach_vm_address_t address = 0;
    mach_vm_size_t size;
    vm_region_basic_info_data_64_t info;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;

    do {
        kr = mach_vm_region(task, &address, &size, flavor, (vm_region_info_t) &info, &count, &object);
        REPORT_MACH_ERROR("mach_vm_region", kr);

        log.traceLn(MDBLog::SYM, "%08X-%08X %8uK %c%c%c/%c%c%c %11s %6s %10s uwir=%hu sub=%u",
            (uint32_t) address, (uint32_t) (address + size), (uint32_t) (size >> 10),
            (info.protection & VM_PROT_READ) ? 'r' : '-',
            (info.protection & VM_PROT_WRITE) ? 'w' : '-',
            (info.protection & VM_PROT_EXECUTE) ? 'x' : '-',
            (info.max_protection & VM_PROT_READ) ? 'r' : '-',
            (info.max_protection & VM_PROT_WRITE) ? 'w' : '-',
            (info.max_protection & VM_PROT_EXECUTE) ? 'x' : '-',
            inheritanceName(info.inheritance),
            (info.shared) ? "shared" : "-",
            behaviorName(info.behavior),
            info.user_wired_count,
            info.reserved);

        // Make sure the region is not read protected and that it's large enough
        // to hold a mach header.

        if (!(info.protection & VM_PROT_READ) || size < sizeof (struct mach_header)) {
            address += size;
            continue;
        }

        mach_vm_address_t image;
        if (read(&image, address, size) == false) {
            address += size;
            continue;
        }

        struct mach_header *header = (struct mach_header *)image;
        if ((header->magic == MH_MAGIC || header->magic == MH_CIGAM)) {
            if (header->filetype == MH_DYLIB || header->filetype == MH_DYLINKER) {
                log.traceLn(MDBLog::SYM, "  imageName: %s", MDBMachFile::findImageName(image));
            }
        }

        free(image, size);
        address += size;
    } while (kr != KERN_INVALID_ADDRESS);
}

mach_vm_address_t
MDBProcess::findAddress(uint32_t fileType, const char *imageName) {
    kern_return_t kr;
    mach_vm_address_t address = 0;
    mach_vm_size_t size;
    vm_region_basic_info_data_64_t info;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;

    mach_vm_address_t foundAddress = 0;

    do {
        kr = mach_vm_region(task, &address, &size, flavor, (vm_region_info_t) &info, &count, &object);
        REPORT_MACH_ERROR("mach_vm_region", kr);
        if (!(info.protection & VM_PROT_READ) || size < sizeof(struct mach_header)) {
            address += size;
            continue;
        }
        mach_vm_address_t image;
        if (read(&image, address, size) == false) {
            address += size;
            continue;
        }
        struct mach_header *header = (struct mach_header *)image;
        if ((header->magic == MH_MAGIC || header->magic == MH_CIGAM)) {
            if (header->filetype == fileType) {
                if (imageName && strcmp(MDBMachFile::findImageName(image), imageName) != 0) {
                    break;
                }
                foundAddress = address;
                free(image, size);
                break;
            }
        }
        free(image, size);
        address += size;
    } while (kr != KERN_INVALID_ADDRESS);
    return foundAddress;
}

void
MDBProcess::onLoadedLibrary(MDBBreakpoint *breakpoint) {
    log.traceLn(MDBLog::SYM, "AHHHHHHHHHHHHHHHHHHHHHHHHHHAAS==========+++++");
}

bool
MDBProcess::initializeDylinkerHooks() {
    mach_vm_address_t address = findAddress(MH_DYLINKER, NULL);
    if (address == false) {
        log.traceLn(MDBLog::SYM, "Dynamic linker was not loaded, cannot initialize dylinker hooks.");
        return false;
    }
    uint32_t dyldAllImageInfosOffset = 0;
    if (read_overwrite(&dyldAllImageInfosOffset, address + DYLD_ALL_IMAGE_INFOS_OFFSET_OFFSET, sizeof(uint32_t)) == false) {
        log.traceLn(MDBLog::SYM, "Cannot read dyldAllImageInfosOffsetOffset.");
        return false;
    }
    mach_vm_address_t infosAddress = 0;
    mach_vm_address_t infosInferiorAddress = address + dyldAllImageInfosOffset;
    if (read(&infosAddress, infosInferiorAddress, sizeof (struct MDBDyldAllImageInfos32)) == false) {
        log.traceLn(MDBLog::SYM, "Cannot read infos from inferior.");
        return false;
    }
    struct MDBDyldAllImageInfos32 *infos = (struct MDBDyldAllImageInfos32 *) infosAddress;

    onLoadedLibraryBreakpoint =
        debugger->createBreakpoint(infos->notification, false, new MBFunction<MDBProcess, void, MDBBreakpoint*> (this, &MDBProcess::onLoadedLibrary));
    onLoadedLibraryBreakpoint->enable(true);

    log.traceLn(MDBLog::SYM, "DyldAllImageInfos32 version: %d, count: %d", infos->version, infos->infoArrayCount);
    for (uint32_t i = 0; i < infos->infoArrayCount; i++) {
        mach_vm_address_t infoAddress = 0;
        mach_vm_address_t infoInferiorAddress = infos->infoArray + sizeof(struct MDBDyldImageInfo32) * i;
        if (read(&infoAddress, infoInferiorAddress, sizeof (struct MDBDyldImageInfo32)) == false) {
            log.traceLn(MDBLog::SYM, "Cannot read info from inferior.");
            return false;
        }
        struct MDBDyldImageInfo32 *info = (struct MDBDyldImageInfo32 *) infoAddress;
        mach_vm_address_t infoNameAddress = 0;
        read(&infoNameAddress, info->imageFilePath, 64);
        char *infoName = (char *)infoNameAddress;
        log.traceLn(MDBLog::SYM, "DyldImageInfo32 name: %s, address: 0x%llX ", infoName, info->imageLoadAddress);
    }
    return false;
}
