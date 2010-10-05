#include "MDBDebuggerTest.h"
#include "../MBShared.h"
#include "../MDBShared.h"
#include "../MDBDebugger.h"
#include "../MDBBreakpoint.h"
#include "../MDBThread.h"
#include "../MDBProcess.h"

void
MDBDebuggerTest0::test() {
    printf("COOL STUFF");
}

void
MDBDebuggerTest0::test1(int i) {
    printf("COOL STUFF %d", i);
}

bool 
MDBDebuggerTest0::run() {
	MDBDebugger debugger;
	const char *fileName = "/Users/Michael/Rust/src/test/run-pass/argv.x86";
	char *commandLineArguments[2];
    commandLineArguments[0] = (char *) fileName;
    commandLineArguments[1] = NULL;

	if (debugger.createProcess(commandLineArguments) == false) {
		return false;
	}

	debugger.process->logExecutionState("STARTED");

	debugger.process->initializeDylinkerHooks();
	debugger.process->logExecutionState("about to resume");
	while (debugger.resume()) {
	    debugger.process->logExecutionState("STOPPED");
	    debugger.logState("HERE");
	};

//	MBDelegate<void, int> a;

//	a.execute(0);

	// MBFunctionOne<void, void, int> *fun;

//	        new MBFunctionOne<void*, void, int>(this, &MDBDebuggerTest0::test1);
//	        // static_cast<MBFunctionOne<void*, void, int>>

	// fun->execute(42);

//	debugger.code.loadSymbols("/usr/lib/dyld");
//	debugger.code.loadSymbols(fileName);
//	debugger.createBreakpoint(0x000014A8, false)->enable(true);
//	debugger.resumeAllThreadsAndTask();
//	debugger.wait(SIG_TRAP);

	// debugger.code.logState();
//	debugger.process->logState();
//	debugger.process->initializeDylinkerHooks();

//	debugger.process->initializeDylinkerHooks();

//	for (int i = 0; i < 10; i++) {
//	    MDBThread *thread = debugger.threads[0];
//	    thread->step();
//	    printf("%d PC: 0x%X\n", i, thread->state.__eip);
//	}
//	debugger.resumeAllThreadsAndTask();
	return false;
};

int main() {
	MDBDebuggerTest0 a;
	a.run();
	return 0;
}
