#ifndef MDBBREAKPOINT_H
#define MDBBREAKPOINT_H

class MDBBreakpoint {
public:
    MDBDebugger *debugger;
    uint64_t address;
    char patch;
    bool enabled;
    MDBBreakpoint(MDBDebugger *debugger, uint64_t address) : 
    debugger(debugger), address(address), enabled(false) {
        // Nop.
    }
    void enable(bool enable);
    void disable();
};

#endif
