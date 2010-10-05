#ifndef MDBBREAKPOINT_H
#define MDBBREAKPOINT_H

class MDBBreakpoint {
public:
    MDBDebugger *debugger;
    uint64_t address;
    char patch;
    bool enabled;
    MBCallback<void, MDBBreakpoint*> *callback;
    MDBBreakpoint(MDBDebugger *debugger, uint64_t address) :
        debugger(debugger), address(address), enabled(false), callback(NULL) {
        // Nop.
    }
    MDBBreakpoint(MDBDebugger *debugger, uint64_t address, MBCallback<void, MDBBreakpoint*> *callback) :
        debugger(debugger), address(address), enabled(false), callback(callback) {
        // Nop.
    }
    void enable(bool enable);
    void onBreakpointReached();
    void disable();
    void logState();
};

#endif
