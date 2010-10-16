#ifndef MDBTHREAD_H
#define MDBTHREAD_H

#include "MDBStack.h"

class MDBProcess;

class MDBThread {
public:
    MDBProcess *process;
    thread_t thread;
    MDBStack stack;
    MDBThreadInfo info;
    MDBThreadState32 state32;
    MDBThreadState64 state64;
    MDBThread(MDBProcess *process, thread_t thread);
    void onStateUpdated();
    void logState();
    const char* runStateName();
    void updateInstructionPointerAndCommitState(int32_t offset);
};

#endif
