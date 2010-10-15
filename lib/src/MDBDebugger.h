#ifndef MDBDEBUGGER_H
#define MDBDEBUGGER_H

#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <mach/mach.h>

#include "MDBLog.h"
#include "MDBCode.h"
#include "MDBProcess.h"

typedef bool (*ThreadVisitor)(MDBDebugger* debugger, thread_t thread, void *arg);



enum MDBThreadRunState {
    TRS_RUNNING,
    TRS_STOPPED,
    TRS_WAITING,
    TRS_UNINTERRUPTIBLE,
    TRS_HALTED
};

enum MDBProcessRunState {
    PRS_UNKNOWN,
    PRS_STOPPED = 1, 
    PRS_RUNNING = 2,
    PRS_TERMINATED = 3
};

enum MDBSignal {
    SIG_NONE = 0,
    SIG_TRAP = SIGTRAP
};

class MDBDebugger {
private:
    mach_port_t task;
    bool forallThreads(ThreadVisitor visitor, void *arg);
    void checkForBreakpointsAndStepBack();
    void checkForBreakpointsAndStep();
    void deleteBreakpoints(MBList<MDBBreakpoint *> &breakpoints);


public:
    bool setSingleStep(thread_t thread, void *arg);
    bool enableSingleStep(MDBThread *thread, bool enable);
    
    char *fileName;
    MDBCode code;
    MBList<MDBThread *> threads;
    MBList<MDBBreakpoint *> breakpoints;
    MBList<MDBBreakpoint *> transientBreakpoints;
    MDBProcess *process;
    
	MDBDebugger();
	~MDBDebugger();
    
	bool createProcess(char** commandLineArguments);
	bool killProcess();

    void logState(const char *name);
    bool suspendOrResumeNoncurrentThread(bool suspend, thread_t thread, thread_t current);

    bool resumeThread(MDBThread *thread, bool resumeTask);
    bool suspendAllThreads();
    bool suspendThread(MDBThread *thread);
    
    bool resumeAllThreadsAndTask();
    bool resumeTask();
    
    bool step(MDBThread *thread, bool resumeAll);
    bool stepOver(MDBThread *thread, bool resumeAll);
    
    MDBProcessRunState internalWait(MDBSignal signal);
    
    MDBBreakpoint *createBreakpoint(uint64_t address, MBCallback<void, MDBBreakpoint*> *callback = NULL);
    MDBBreakpoint *findBreakpoint(uint64_t address);
    
    int read(void *dst, vm_address_t src, size_t size, bool ignoreBreakpoints = true);
    
    int write(vm_address_t dst, void *src, size_t size);

    bool resume();
};

class MDBDisassembler {
public:
    static size_t disassembleMethod(uintptr_t address, char *buffer, size_t bufferSize);
};

#endif /* MDBDEBUGGER_H */
