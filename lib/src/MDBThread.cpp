#include "MDBShared.h"
#include "MDBProcess.h"
#include "MDBThread.h"
#include "MDBLog.h"

MDBThread::MDBThread(MDBProcess *process, thread_t thread)
    : process(process), thread(thread), stack(this) {
    // Nop.
}

void
MDBThread::onStateUpdated() {
    // Nop.
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
        thread, info.suspend_count, state32.__esp, state32.__ebp, state32.__eip,
        state32.__eflags & 0x100UL ? "yes" : "no");
}

void
MDBThread::updateInstructionPointerAndCommitState(int32_t offset) {
    state32.__eip += offset;
    process->machCommitThread(this);
}
