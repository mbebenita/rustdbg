#ifndef MDBTHREAD_H
#define MDBTHREAD_H

#include "MDBStack.h"
#include "MDBDebugger.h"

class MDBThread {
public:
    MDBDebugger *debugger;
    thread_t thread;
    MDBStack stack;
    MDBThreadInfo info;
    MDBThreadState state;
    MDBThread(MDBDebugger *debugger, thread_t thread);
    bool step();
    bool stepOver();
    bool resume();
    bool updateState();
    void onStateUpdated();
    bool stepBack();
    bool commitState();
    void logState();
    const char* runStateName();
};

#endif
