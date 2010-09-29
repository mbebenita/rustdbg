#ifndef MDBSTACK_H
#define MDBSTACK_H

#define K 1024
#define MAX_STACK_SIZE 64 * K

class MDBStackFrame {
public:
    MDBStack *stack;
    MDBStackFrame *parent;
    uintptr_t address;
    uintptr_t codeAddress;
    MDBCodeRegion *codeRegion;

    size_t size;
    MDBStackFrame (MDBStack *stack, MDBStackFrame *parent, uintptr_t address, uintptr_t codeAddress, MDBCodeRegion *codeRegion, size_t size);
    uint32_t readU32(int32_t frameOffset);
};

class MDBStack {
public:
    MDBThread *thread;
    MDBStackFrame *frame;
    ArrayList<MDBStackFrame *> frames;
    MDBStack(MDBThread *thread);
    void updateState();
    bool isInteriorPointer(uintptr_t address);
};


#endif
