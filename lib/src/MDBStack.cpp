#include "MDBShared.h"
#include "MDBStack.h"
#include "MDBThread.h"

MDBStackFrame::MDBStackFrame (MDBStack *stack,
                              MDBStackFrame *parent,
                              uintptr_t address,
                              uintptr_t codeAddress,
                              MDBCodeRegion *codeRegion,
                              size_t size)
    : stack(stack), parent(parent), address(address), codeAddress(codeAddress), codeRegion(codeRegion), size(size) {
};

uint32_t 
MDBStackFrame::readU32(int32_t offset) {
    uint32_t value;
    stack->thread->debugger->read(&value, address + offset, sizeof(uint32_t));
    return value;
}

MDBStack::MDBStack(MDBThread *thread) 
    : thread(thread), frame(NULL) {
    // Nop.
}

void
MDBStack::updateState() {
    // log("walking stack for thread: %d", thread->thread);
    uintptr_t codeAddress = thread->state.__eip;
    uintptr_t frameAddress = thread->state.__ebp;
    size_t frameSize = 128;
    frames.clear();
    MDBStackFrame *frame = NULL;
    while (frameAddress) {
        uint32_t previousFrameAddress = 0;
        uint32_t previousFrameAddressOffset = 0;
        thread->debugger->read(&previousFrameAddress, frameAddress + previousFrameAddressOffset, sizeof(uint32_t));
        if (isInteriorPointer(previousFrameAddress) == false) {
            previousFrameAddressOffset = 12;
            thread->debugger->read(&previousFrameAddress, frameAddress + previousFrameAddressOffset, sizeof(uint32_t));
        }
        MDBCodeRegion *region = thread->debugger->code.getRegion(codeAddress);
        frame = new MDBStackFrame(this, NULL, frameAddress, codeAddress, region, frameSize);
        frames.push(frame);
        // log("frameAddress %x, previousFrameAddress %x, codeAddress %x", frameAddress, previousFrameAddress, codeAddress);
        thread->debugger->read(&codeAddress, frameAddress + previousFrameAddressOffset + 4, sizeof(uint32_t));
        frameSize = previousFrameAddress - frameAddress;
        frameSize = frameSize > 256 ? 256 : frameSize;
        frameAddress = previousFrameAddress;
        if (frames.length() > 64) {
            log("too much recursion");
            break;
        }
    }
    // Link frames.
    for (uint32_t i = 0; frames.length() && i < frames.length() - 1; i++) {
        frames[i]->parent = frames[i + 1];
    }
    this->frame = frames.length() > 0 ? frames[0] : NULL;
}

bool 
MDBStack::isInteriorPointer(uintptr_t address) {
    uintptr_t frameAddress = thread->state.__ebp;
    uintptr_t baseAddress = frameAddress + MAX_STACK_SIZE;
    return address >= baseAddress && address <= frameAddress;
}
