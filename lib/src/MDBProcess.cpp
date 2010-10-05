#include "MDBShared.h"
#include "MDBDebugger.h"
#include "MDBThread.h"
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
MDBProcess::logDylinkerState() {
    mach_vm_address_t address = findAddress(MH_DYLINKER, NULL);
    if (address == false) {
        log.traceLn(MDBLog::SYM, "Dynamic linker was not loaded, cannot initialize dylinker hooks.");
        return;
    }
    uint32_t dyldAllImageInfosOffset = 0;
    if (read_overwrite(&dyldAllImageInfosOffset, address + DYLD_ALL_IMAGE_INFOS_OFFSET_OFFSET, sizeof(uint32_t)) == false) {
        log.traceLn(MDBLog::SYM, "Cannot read dyldAllImageInfosOffsetOffset.");
        return;
    }
    mach_vm_address_t infosAddress = 0;
    mach_vm_address_t infosInferiorAddress = address + dyldAllImageInfosOffset;
    if (read(&infosAddress, infosInferiorAddress, sizeof (struct MDBDyldAllImageInfos32)) == false) {
        log.traceLn(MDBLog::SYM, "Cannot read infos from inferior.");
        return;
    }
    struct MDBDyldAllImageInfos32 *infos = (struct MDBDyldAllImageInfos32 *) infosAddress;
    log.traceLn(MDBLog::SYM, "DyldAllImageInfos32 version: %d, count: %d", infos->version, infos->infoArrayCount);
    for (uint32_t i = 0; i < infos->infoArrayCount; i++) {
        mach_vm_address_t infoAddress = 0;
        mach_vm_address_t infoInferiorAddress = infos->infoArray + sizeof(struct MDBDyldImageInfo32) * i;
        if (read(&infoAddress, infoInferiorAddress, sizeof (struct MDBDyldImageInfo32)) == false) {
            log.traceLn(MDBLog::SYM, "Cannot read info from inferior.");
            return;
        }
        struct MDBDyldImageInfo32 *info = (struct MDBDyldImageInfo32 *) infoAddress;
        mach_vm_address_t infoNameAddress = 0;
        read(&infoNameAddress, info->imageFilePath, 64);
        char *infoName = (char *)infoNameAddress;
        log.traceLn(MDBLog::SYM, "DyldImageInfo32 name: %s, address: 0x%llX ", infoName, info->imageLoadAddress);
    }
    return;
}

void
MDBProcess::logMemoryState() {
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
MDBProcess::onLibraryLoaded(MDBBreakpoint *breakpoint) {
    logDylinkerState();
    logMemoryState();
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
    // Set a breakpoint that is triggered whenever the dylinker loads a new library.
    onLibraryLoadedBreakpoint =
        debugger->createBreakpoint(infos->notification, new MBFunction<MDBProcess, void, MDBBreakpoint*> (this, &MDBProcess::onLibraryLoaded));
    onLibraryLoadedBreakpoint->enable(true);
    return false;
}

MDBThread *
MDBProcess::findThread(thread_t thread) {
    for (uint32_t i = 0; i < threads.length(); i++) {
        if (threads[i]->thread == thread) {
            return threads[i];
        }
    }
    return NULL;
}

bool
MDBProcess::gatherThreads() {
    thread_array_t threadList = NULL;
    unsigned int threadCount = 0;
    kern_return_t kr = task_threads(task, &threadList, &threadCount);
    RETURN_ON_MACH_ERROR("task_threads", kr, false);
    for (uint32_t i = 0; i < threadCount; i++) {
        MDBThread *thread = findThread(threadList[i]);
        if (thread == NULL) {
            thread = threads.add(new MDBThread(this, threadList[i]));
        }
        machUpdateThread(thread);
    }
    kr = vm_deallocate(mach_task_self(), (vm_address_t) threadList, (threadCount * sizeof(int)));
    RETURN_ON_MACH_ERROR("vm_deallocate", kr, false);
    return true;
}

bool
MDBProcess::machCommitThread(MDBThread *thread) {
    mach_msg_type_number_t stateCount = THREAD_STATE_REGISTER_COUNT;
    WRAP_MACH(thread_set_state(thread->thread, THREAD_STATE_FLAVOR, (natural_t *) &thread->state, stateCount), false);
    return true;
}

bool
MDBProcess::machUpdateThread(MDBThread *thread) {
    unsigned int infoCount = THREAD_BASIC_INFO_COUNT;
    thread_info(thread->thread, THREAD_BASIC_INFO, (thread_info_t) &thread->info, &infoCount);
    mach_msg_type_number_t stateCount = THREAD_STATE_REGISTER_COUNT;
    WRAP_MACH(thread_get_state(thread->thread, THREAD_STATE_FLAVOR, (natural_t *) &thread->state, &stateCount), false);
    thread->onStateUpdated();
    return true;
}

void
MDBProcess::logExecutionState(const char *name) {
    struct task_basic_info info;
    unsigned int info_count = TASK_BASIC_INFO_COUNT;
    task_info(task, TASK_BASIC_INFO, (task_info_t) &info, &info_count);
    log.traceDividerLn("execution state: %s", name);
    log.traceLn("  task: %d, sc: %d", task, info.suspend_count);
    for (uint32_t i = 0; i < threads.length(); i++) {
        threads[i]->logState();
    }
    log.traceDividerLn();
}

bool
MDBProcess::machResumeThreadsAndTask() {
    for (uint32_t i = 0; i < threads.length(); i++) {
        MDBThread *thread = threads[i];
        if (machResumeThread(thread) == false) {
            return false;
        }
    }
    return machResumeTask();
}

bool
MDBProcess::machResumeThread(MDBThread *thread) {
    struct thread_basic_info info;
    unsigned int infoCount = THREAD_BASIC_INFO_COUNT;
    kern_return_t kr = thread_info(thread->thread, THREAD_BASIC_INFO, (thread_info_t) &info, &infoCount);
    RETURN_ON_MACH_ERROR("thread_info()", kr, false);

    // The thread will not resume unless we abort it first if it's WAITING. It is usually
    // in the WAITING state if it stopped because of a trap.
    if (info.run_state == TH_STATE_WAITING) {
        thread_abort(thread->thread);
    }

    // Resume the thread.
    for (uint32_t i = 0; i < (unsigned) info.suspend_count; i++) {
        thread_resume(thread->thread);
    }

    log.traceLn(MDBLog::THREAD, "resumed thread: %d", thread->thread);
    return true;
}

bool
MDBProcess::machResumeTask() {
    while (true) {
        struct task_basic_info info;
        unsigned int info_count = TASK_BASIC_INFO_COUNT;
        kern_return_t kr = task_info(task, TASK_BASIC_INFO, (task_info_t) &info, &info_count);
        RETURN_ON_MACH_ERROR("task_info()", kr, false);
        if (info.suspend_count > 0) {
            task_resume((task_t) task);
        } else {
            break;
        }
    }
    log.traceLn(MDBLog::DEBUG, "resumed task: %d", task);
    return true;
}

bool
MDBProcess::machSuspendThread(MDBThread *thread) {
    struct thread_basic_info info;
    unsigned int info_count = THREAD_BASIC_INFO_COUNT;
    kern_return_t kr = thread_info(thread->thread, THREAD_BASIC_INFO, (thread_info_t) &info, &info_count);
    RETURN_ON_MACH_ERROR("thread_info()", kr, false);
    if (info.suspend_count == 0) {
        kr = thread_suspend(thread->thread);
        if (kr != KERN_SUCCESS) {
            log.traceLn(MDBLog::THREAD, "thread_suspend() failed when suspending thread %d", thread);
        }
    }
    log.traceLn(MDBLog::THREAD, "suspended thread %d", thread->thread);
    return true;
}

bool
MDBProcess::machSuspendAllThreads() {
    for (uint32_t i = 0; i < threads.length(); i++) {
        machSuspendThread(threads[i]);
    }
    return true;
}
