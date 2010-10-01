#include "MDBDebuggerTest.h"
#include "../MDBShared.h"
#include "../MDBDebugger.h"
#include "../MDBThread.h"

bool 
MDBDebuggerTest0::run() {
    printf("HERE\n");
	MDBDebugger debugger;
	char *commandLineArguments[2];
    commandLineArguments[0] = (char *) "/Users/Michael/Rust/src/test/run-pass/argv.x86";
    commandLineArguments[1] = NULL;
	if (debugger.createProcess(commandLineArguments) == false)
		return false;
	for (int i = 0; i < 100000; i++) {
	    MDBThread *thread = debugger.threads[0];
	    thread->step();
	    printf("%d PC: 0x%X\n", i, thread->state.__eip);
	}
	debugger.resumeAllThreadsAndTask();
	return false;
};

int main() {
	MDBDebuggerTest0 a;
	a.run();
	return 0;
}
