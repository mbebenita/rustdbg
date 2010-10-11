#include "MDBShared.h"
#include "MDBDebugger.h"
#include "MDBThread.h"
#include "MDBBreakpoint.h"

#include <errno.h>
#include <unistd.h>
#include <mach/mach_vm.h>

#define ERROR -1;

MDBDebugger::MDBDebugger() : code(this), process(NULL) {

}

MDBDebugger::~MDBDebugger() {

}

bool
MDBDebugger::forallThreads(ThreadVisitor visitor, void *arg) {
    thread_array_t thread_list = NULL;
    unsigned int nthreads = 0;
    unsigned i;    
    kern_return_t kr = task_threads(task, &thread_list, &nthreads);
    RETURN_ON_MACH_ERROR("task_threads", kr, false);
    for (i = 0; i < nthreads; i++) {
        thread_t thread = thread_list[i];
        if (!(*visitor)(this, thread, arg)) {
            break;
        }
    }
    // deallocate thread list
    kr = vm_deallocate(mach_task_self(), (vm_address_t) thread_list, (nthreads * sizeof(int)));
    RETURN_ON_MACH_ERROR("vm_deallocate", kr, false);
    return true;
}

bool setSingleStepDelegate(MDBDebugger *debugger, thread_t thread, void *arg) {
    return debugger->setSingleStep(thread, arg);
}

void
MDBDebugger::logState(const char *name) {
    log.traceDividerLn("debugger state: %s", name);
    for (uint32_t i = 0; i < breakpoints.length(); i++) {
        breakpoints[i]->logState();
    }
    log.traceDividerLn();
}

bool 
MDBDebugger::suspendOrResumeNoncurrentThread(bool suspend, thread_t thread, thread_t current) {
    kern_return_t kr;
    if (current == thread) {
        return true;
    }
    
    struct thread_basic_info info;
    unsigned int info_count = THREAD_BASIC_INFO_COUNT;
    
    kr = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t) &info, &info_count);
    if (kr != KERN_SUCCESS) {
        log.traceLn(MDBLog::DEBUG, "thread_info() failed when suspending thread %d", thread);
    } else {
        if (suspend) {
            if (info.suspend_count == 0) {
                kr = thread_suspend(thread);
                if (kr != KERN_SUCCESS) {
                    log.traceLn(MDBLog::DEBUG, "thread_suspend() failed when suspending thread %d", thread);
                }
            }
        } else {
            for (uint32_t i = 0; i < (unsigned) info.suspend_count; i++) {
                kr = thread_resume(thread);
                if (kr != KERN_SUCCESS) {
                    log.traceLn(MDBLog::DEBUG, "thread_resume() failed when resuming thread %d", thread);
                    break;
                }
            }            
        }
    }
    return true;
}

bool 
suspendNoncurrentThreadDelegate(MDBDebugger *debugger, thread_t thread, void *arg) {
    return debugger->suspendOrResumeNoncurrentThread(true, thread, (thread_t) (uintptr_t) arg);
}

bool 
resumeNoncurrentThreadDelegate(MDBDebugger *debugger, thread_t thread, void *arg) {
    return debugger->suspendOrResumeNoncurrentThread(false, thread, (thread_t) (uintptr_t) arg);
}

bool
MDBDebugger::suspendAllThreads() {
    log.traceLn("suspending all threads");
    for (uint32_t i = 0; i < threads.length(); i++) {
        suspendThread(threads[i]);
    }
    return true;
}

bool
MDBDebugger::resumeAllThreadsAndTask() {
    for (uint32_t i = 0; i < threads.length(); i++) {
        MDBThread *thread = threads[i];
        resumeThread(thread, false);
    }
    resumeTask();
    return true;
}

bool
MDBDebugger::resume() {
    checkForBreakpointsAndStep();
    process->machResumeThreadsAndTask();
    if (internalWait(SIG_TRAP) != PRS_STOPPED) {
        return false;
    }
    checkForBreakpointsAndStepBack();
    return true;
}
    
bool
MDBDebugger::resumeThread(MDBThread *thread, bool shouldResumeTask) {
    kern_return_t kr;
    struct thread_basic_info info;
    unsigned int info_count = THREAD_BASIC_INFO_COUNT;
    unsigned int j;
    
    // get info for the current thread
    kr = thread_info(thread->thread, THREAD_BASIC_INFO, (thread_info_t) &info, &info_count);
    RETURN_ON_MACH_ERROR("thread_info()", kr, false);
    
    // if the thread is WAITING it will not resume unless we abort it first
    // the thread is WAITING if if stopped because of a trap
    if (info.run_state == TH_STATE_WAITING) {
        thread_abort(thread->thread);
    }
    
    // resume the thread
    for (j = 0; j < (unsigned) info.suspend_count; j++) {
        thread_resume(thread->thread);
    }       
    
    // the thread will not resume unless the task is also resumed
    if (shouldResumeTask) {
        resumeTask();
    }
    
    log.traceLn(MDBLog::DEBUG, "resumed thread: %d", thread->thread);
    return true;
}

bool
MDBDebugger::suspendThread(MDBThread *thread) {
    struct thread_basic_info info;
    unsigned int info_count = THREAD_BASIC_INFO_COUNT;
    kern_return_t kr = thread_info(thread->thread, THREAD_BASIC_INFO, (thread_info_t) &info, &info_count);
    RETURN_ON_MACH_ERROR("thread_info()", kr, false);    
    if (info.suspend_count == 0) {
        kr = thread_suspend(thread->thread);
        if (kr != KERN_SUCCESS) {
            log.traceLn(MDBLog::DEBUG, "thread_suspend() failed when suspending thread %d", thread);
        }
    }
    log.traceLn(MDBLog::DEBUG, "suspended thread %d", thread->thread);
    return true;    
}

bool
MDBDebugger::resumeTask() {
    bool warningPrinted = false;
    while (true) {
        struct task_basic_info info;
        unsigned int info_count = TASK_BASIC_INFO_COUNT;
        kern_return_t kr = task_info(task, TASK_BASIC_INFO, (task_info_t) &info, &info_count);
        RETURN_ON_MACH_ERROR("task_info()", kr, false);
        if (info.suspend_count > 0) {
            if (info.suspend_count > 1 && !warningPrinted) {
                warningPrinted = true;
                log.traceLn("here");
            }
            task_resume((task_t) task);
        } else {
            break;
        }
    }
    log.traceLn(MDBLog::DEBUG, "resumed task: %d", task);
    return true;
}

int
MDBDebugger::read(void *dst, vm_address_t src, size_t size, bool ignoreBreakpoints) {
    mach_vm_size_t bytesRead;
    kern_return_t kr = mach_vm_read_overwrite(task, src, size, (vm_address_t) dst, &bytesRead);
    // Patch parse buffer with breakpoint patches.
    for (uint32_t i = 0; i < breakpoints.length(); i++) {
        MDBBreakpoint *breakpoint = breakpoints[i];
        if (breakpoint->enabled) {
            int64_t offset = breakpoint->address - src;
            if (offset >= 0 && offset < (int) size) {
                ((char *)dst)[offset] = breakpoint->patch;
            }
        }
    }
    RETURN_ON_MACH_ERROR("mach_vm_read_overwrite", kr, -1);
    return (size_t) bytesRead;
}

int
MDBDebugger::write(vm_address_t dst, void *src, size_t size) {
    kern_return_t kr;
    kr = mach_vm_write(task, dst, (vm_offset_t) src, size);
    if (kr != KERN_SUCCESS) {
        kr = mach_vm_protect(task, dst, size, FALSE, VM_PROT_COPY | VM_PROT_ALL);
        REPORT_MACH_ERROR("mach_vm_protect", kr);
        kr = mach_vm_write(task, dst, (vm_offset_t) src, size);
    }
    REPORT_MACH_ERROR("mach_vm_write", kr);
    return size;
}

MDBBreakpoint *
MDBDebugger::createBreakpoint(uint64_t address, MBCallback<void, MDBBreakpoint*> *callback) {
    MDBBreakpoint *breakpoint = new MDBBreakpoint(this, address, callback);
    breakpoints.add(breakpoint);
    return breakpoint;
}

MDBBreakpoint *
MDBDebugger::findBreakpoint(uint64_t address) {
    for (uint32_t i = 0; i < breakpoints.length(); i++) {
        if (breakpoints[i]->address == address) {
            return breakpoints[i];
        }
    }
    for (uint32_t i = 0; i < transientBreakpoints.length(); i++) {
        if (transientBreakpoints[i]->address == address) {
            return transientBreakpoints[i];
        }
    }
    return NULL;
}

void
MDBDebugger::deleteBreakpoints(MBList<MDBBreakpoint *> &list) {
    for (uint32_t i = 0; i < list.length(); i++) {
        assert (list[i]->enabled == false);
        delete list[i];
    }
    list.clear();
}

/**
 * If we stopped at a breakpoint we are actually one byte after the breakpoint 
 * address, so here we check if any breakpoints were set one byte before and
 * if so, we disable the breakpoint and step back.
 */
void
MDBDebugger::checkForBreakpointsAndStepBack() {
    for (uint32_t i = 0; i < process->threads.length(); i++) {
        MDBThread *thread = process->threads[i];
        MDBBreakpoint *breakpoint = findBreakpoint(thread->state.__eip - 1);

        // Check if we've stopped at a breakpoint.
        if (breakpoint == false) {
            continue;
        }

        // We stopped at a breakpoint, step back.
        if (breakpoint->enabled) {
            breakpoint->onBreakpointReached();
            // Step the tread back by 1.
            thread->updateInstructionPointerAndCommitState(-1);
        }
    }
}

void
MDBDebugger::checkForBreakpointsAndStep() {
    for (uint32_t i = 0; i < process->threads.length(); i++) {
        MDBThread *thread = process->threads[i];
        MDBBreakpoint *breakpoint = findBreakpoint(thread->state.__eip);
        if (breakpoint && breakpoint->enabled) {
            breakpoint->disable();
            enableSingleStep(thread, true);
            process->machSuspendAllThreads();
            process->machResumeThread(thread);
            process->machResumeTask();
            if (internalWait(SIG_TRAP) == PRS_STOPPED) {
                enableSingleStep(thread, false);
                breakpoint->enable(true);
            }
        }
    }
}

MDBProcessRunState
MDBDebugger::internalWait(MDBSignal waitSignal) {
    log.traceLn(MDBLog::DEBUG, "waiting for task ..");
    int debuggeePid;
    WRAP_MACH(pid_for_task(task, &debuggeePid), PRS_UNKNOWN);
    while (true) {
        int status;
        int error;
        while ((error = waitpid(debuggeePid, &status, 0)) != debuggeePid && 
               errno == EINTR) {
            log.traceLn(MDBLog::DEBUG, "waitpid failed with error: %d (%s)", errno, strerror(error));
        }
        if (error != debuggeePid) {
            log.traceLn(MDBLog::DEBUG, "waitpid failed with error: %d (%s)", errno, strerror(error));
            process->gatherThreads();
            return PRS_UNKNOWN;
        }
        if (WIFEXITED(status)) {
            log.traceLn(MDBLog::DEBUG, "process: %d exited with exit code: %d", debuggeePid, WEXITSTATUS(status));
            process->machSignalTerminated();
            return PRS_TERMINATED;
        }
        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            log.traceLn(MDBLog::DEBUG, "process: %d terminated due to signal number: %d (%s)", debuggeePid, signal, strsignal(signal));
            process->gatherThreads();
            return PRS_TERMINATED;
        }
        if (WIFSTOPPED(status)) {
            process->gatherThreads();
            // check whether the process received a signal, and continue with it if so.
            int signal = WSTOPSIG(status);
            log.traceLn(MDBLog::DEBUG, "process: %d stopped due to the signal number: %d (%s)", debuggeePid, signal, strsignal(signal));
            if (signal == waitSignal) {
                return PRS_STOPPED;
            } else {
                ptrace(PT_CONTINUE, debuggeePid, (char*) 1, signal);
                error = errno;
                if (error != 0) {
                    log.traceLn(MDBLog::DEBUG, "continuing process %d failed: %d [%s]", error, strerror(error));
                    process->gatherThreads();
                    return PRS_UNKNOWN;
                }
            }
        }
    }
}

/**
 * Suspends all other running threads while single stepping the specified
 * thread.
 */
bool
MDBDebugger::step(MDBThread *thread, bool resumeAll) {
//    log.traceLn(MDBLog::CALL, "step thread: %d, resumeAll: %d", thread->thread, resumeAll);
//    MDBBreakpoint *breakpoint = findBreakpoint(thread->state.__eip);
//    bool reEnableBreakpoint = false;
//    if (breakpoint && breakpoint->enabled) {
//        breakpoint->disable();
//        reEnableBreakpoint = true;
//    }
//    if (resumeAll == false) {
//        suspendAllThreads();
//    }
//    enableSingleStep(thread, true);
//    if (resumeAll) {
//        resumeAllThreadsAndTask();
//    } else {
//        resumeThread(thread, false);
//        resumeTask();
//    }
//    MDBProcessRunState state = internalWait(SIG_TRAP);
//    if (state == PRS_STOPPED) {
//        if (reEnableBreakpoint) {
//            breakpoint->enable(true);
//        }
//        return true;
//    }
//    return false;

    MDBBreakpoint *breakpoint = findBreakpoint(thread->state.__eip);
    bool reEnableBreakpoint = false;
    if (breakpoint && breakpoint->enabled) {
        breakpoint->disable();
        reEnableBreakpoint = true;
    }
    enableSingleStep(thread, true);
    process->machSuspendAllThreads();
    process->machResumeThread(thread);
    process->machResumeTask();
    if (internalWait(SIG_TRAP) == PRS_STOPPED) {
        enableSingleStep(thread, false);
    }
    if (reEnableBreakpoint) {
        breakpoint->enable(true);
    }
    return true;
}

/**
 * Steps over the current instruction. Set a breakpoint right after the 
 * currenet instruction and resume.
 */
bool
MDBDebugger::stepOver(MDBThread *thread, bool resumeAll) {
    if (code.isCallInstruction(thread->state.__eip) == false) {
        return step(thread, resumeAll);
    }
    MDBBreakpoint *breakpoint = findBreakpoint(thread->state.__eip);
    bool reEnableBreakpoint = false;
    if (breakpoint && breakpoint->enabled) {
        breakpoint->disable();
        reEnableBreakpoint = true;
    }
    size_t instructionLength = code.getInstructionLength(thread->state.__eip);
    MDBBreakpoint *transientBreakpoint =
        createBreakpoint(thread->state.__eip + instructionLength);
    transientBreakpoint->enable(true);
    resumeAllThreadsAndTask();
    MDBProcessRunState state = internalWait(SIG_TRAP);
    if (state == PRS_STOPPED) {
        if (reEnableBreakpoint) {
            breakpoint->enable(true);
        }
        deleteBreakpoints(transientBreakpoints);
        return true;
    }
    return false;
}

bool
MDBDebugger::killProcess() {
    int debuggeePid = 0;
    kern_return_t kr = pid_for_task(task, &debuggeePid);
    REPORT_MACH_ERROR("pid_for_task", kr); 
    return ptrace(PT_KILL, debuggeePid, 0, 0) == 0;
}

bool
MDBDebugger::setSingleStep(thread_t thread, void *arg) {
    const bool isEnabled = arg != NULL;
    MDBThreadState state;
    mach_msg_type_number_t state_count = THREAD_STATE_COUNT;
    WRAP_MACH(thread_get_state(thread, THREAD_STATE_FLAVOR, (natural_t *) &state, &state_count), false);
    if (isEnabled) {
        state.__eflags |= 0x100UL;
    } else {
        state.__eflags &= ~0x100UL;
    }
    WRAP_MACH(thread_set_state(thread, THREAD_STATE_FLAVOR, (natural_t *) &state, state_count), false);
    log.traceLn("%s single step flag for thread %d.", isEnabled ? "Set" : "Removed", thread);
    return true;
}

bool
MDBDebugger::enableSingleStep(MDBThread *thread, bool enable) {
    if (enable) {
        thread->state.__eflags |= 0x100UL;
    } else {
        thread->state.__eflags &= ~0x100UL;
    }
    process->machCommitThread(thread);
    log.traceLn(MDBLog::DEBUG, "%s single step flag for thread %d", enable ? "set" : "removed", thread->thread);
    return true;
}

bool
MDBDebugger::createProcess(char** commandLineArguments) {
	if (acquireTaskportRight() != 0) {
		error("Failed to acquire taskport right.");
		return false;
	}
	const char* fileName = commandLineArguments[0];
    log.traceLn("creating process %s", fileName);
	int childPid = fork();
	if (childPid == 0) {        
        // We're in the child process now.
		if (ptrace(PT_TRACE_ME, 0, 0, 0) != 0) {
            error("Failed to initialize ptrace in forked process.");
            return ERROR;
        }
		log.traceLn("executing target process");
        execv(commandLineArguments[0], commandLineArguments);
        error("Failed to execte (execv) target process.");
        // TODO: Notify parent task that we failed to execute target process.
    } else if (childPid < 0) {
        error("Cannot fork process.");
        return false;
	} else {
        // We're in the parent process now.
        int status = 0;
        log.traceLn("waiting for pid %d", childPid);
        
        // Wait for the process to stop, waitpid() can fail if it is interrupted
        // by a signal, so poll until we've succeeded the system call.
        int waitPid = 0;
        while ((waitPid = waitpid(childPid, &status, 0)) == childPid && errno == EINTR);
        // while ((waitPid = waitpid(childPid, &status, 0)) <= 0);
        if (WIFSTOPPED(status)) {
			// log.traceLn("sig: %d", WSTOPSIG(status));
            kern_return_t kr;
            // Get the mach task for the target process.
            kr = task_for_pid(mach_task_self(), childPid, &task);
			RETURN_ON_MACH_ERROR("task_for_pid", kr, -1);
            log.traceLn(MDBLog::INFO, "Created inferior process name: %s, pid: %d.", fileName, childPid);
            process = new MDBProcess(this, task, childPid);
            process->initializeDylinkerHooks();
            process->gatherThreads();
            return true;
        } else if (WIFEXITED(status)) {
            error("Process terminated normally.");
        } else if (WIFSIGNALED(status)) {
            error("Process terminated due to receipt of an unhandled signal.");
        }
	}
	sleep(1000);
    return false;
}
