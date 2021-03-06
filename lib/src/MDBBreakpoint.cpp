#include "MDBShared.h"
#include "MDBBreakpoint.h"
#include "MDBDebugger.h"
#include "MDBLog.h"

void
MDBBreakpoint::onBreakpointReached() {
    if (callback) {
        callback->execute(this);
    }
}

void
MDBBreakpoint::disable() {
    enable(false);
}

void
MDBBreakpoint::enable(bool enable) {
    if (this->enabled == false && enable) {
        char int_3 = 0xCC;
        this->enabled = debugger->read(&patch, address, 1) && debugger->write(address, &int_3, 1);
        if (this->enabled) {
            log.traceLn("enabled breakpoint at 0x%X, patch back with 0x%X", address, patch);
        } else {
            error("cannot enable breakpoint");
        }
        return;
    }
    if (this->enabled && enable == false) {
        this->enabled = debugger->write(address, &patch, 1) == false;
        if (this->enabled == false) {
            log.traceLn("breakpoint removed from 0x%X, patched with 0x%X", address, patch);
        } else {
            error("cannot remove breakpoint");
        }
    }
}

void MDBBreakpoint::logState() {
    log.traceLn("breakpoint address: 0x%llx, patch: %x, enabled: %s, temporarilyDisabled %s", address, patch, enabled ? "yes" : "no");
}
