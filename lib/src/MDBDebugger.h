#ifndef MDBDEBUGGER_H
#define MDBDEBUGGER_H

#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <mach/mach.h>

#include "MDBLog.h"
#include "MDBCode.h"

typedef bool (*ThreadVisitor)(MDBDebugger* debugger, thread_t thread, void *arg);

#define THREAD_STATE_REGISTER_COUNT x86_THREAD_STATE32_COUNT
#define THREAD_STATE_COUNT x86_THREAD_STATE32_COUNT
#define THREAD_STATE_FLAVOR x86_THREAD_STATE32
typedef x86_thread_state32_t MDBThreadState;
typedef thread_basic_info MDBThreadInfo;

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
    NONE = 0,
    TRAP = SIGTRAP
};

class MDBDebugger {
private:
    mach_port_t task;
    bool forallThreads(ThreadVisitor visitor, void *arg);

    
    void checkForBreakpointsAndStepBack();
    void deleteBreakpoints(MBList<MDBBreakpoint *> &breakpoints);
public:
    bool setSingleStep(thread_t thread, void *arg);
    bool enableSingleStep(MDBThread *thread, bool enable);
    
    char *fileName;

    MDBCodeRegionManager code;
    MBList<MDBThread *> threads;
    MBList<MDBBreakpoint *> breakpoints;
    MBList<MDBBreakpoint *> transientBreakpoints;
    
	MDBDebugger();
	~MDBDebugger();
    
	bool createProcess(char** commandLineArguments);
	bool killProcess();
    bool gatherThreads();
    
    bool updateThread(MDBThread *thread);
    bool commitThread(MDBThread *thread);

    void logState(const char *name);
    MDBThread *findThread(thread_t thread);
    bool suspendOrResumeNoncurrentThread(bool suspend, thread_t thread, thread_t current);

    bool resumeThread(MDBThread *thread, bool resumeTask);
    bool suspendAllThreads();
    bool suspendThread(MDBThread *thread);
    
    bool resumeAllThreadsAndTask();
    bool resumeTask();
    
    bool step(MDBThread *thread, bool resumeAll);
    bool stepOver(MDBThread *thread, bool resumeAll);
    
    MDBProcessRunState wait(MDBSignal signal);
    
    MDBBreakpoint *createBreakpoint(uint64_t address, bool transient = false);
    MDBBreakpoint *findBreakpoint(uint64_t address);
    
    int read(void *dst, vm_address_t src, size_t size, 
        bool ignoreBreakpoints = true);
    
    int write(vm_address_t dst, void *src, size_t size);
};

class MDBDisassembler {
public:
    static size_t disassembleMethod(uintptr_t address, char *buffer, size_t bufferSize);
};

#endif /* MDBDEBUGGER_H */
