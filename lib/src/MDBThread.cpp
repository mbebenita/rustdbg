#include "MDBShared.h"
#include "MDBThread.h"

MDBThread::MDBThread(MDBDebugger *debugger, thread_t thread) 
    : debugger(debugger), thread(thread), stack(this) {
    // Nop.
}

bool
MDBThread::resume() {
    return debugger->resumeThread(this, false);
}

bool
MDBThread::step() {
    return debugger->step(this, true);
}

bool
MDBThread::stepOver() {
    return debugger->stepOver(this, true);
}

bool
MDBThread::stepBack() {
    state.__eip -= 1;
    commitState();
    return true;
}

bool
MDBThread::updateState() {
    return debugger->updateThread(this);
}

void
MDBThread::onStateUpdated() {
    // stack.updateState();
}

bool
MDBThread::commitState() {
    return debugger->commitThread(this);
}

const char* 
MDBThread::runStateName() {
    switch (info.run_state) {
        case TH_STATE_RUNNING: return "RUNNING";
        case TH_STATE_STOPPED: return "STOPPED";
        case TH_STATE_WAITING: return "WAITING";
        case TH_STATE_UNINTERRUPTIBLE: return "UNINTERRUPTIBLE";
        case TH_STATE_HALTED: return "HALTED";
        default: return "NULL";
    }
}

void
MDBThread::logState() {
    log.traceLn("thread: %d, sc: %d sp: 0x%X, fp: 0x%X, pc: 0x%X, step: %s",
        thread, info.suspend_count, state.__esp, state.__ebp, state.__eip,
        state.__eflags & 0x100UL ? "yes" : "no");
}
