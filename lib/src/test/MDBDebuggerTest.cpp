#include "MDBDebuggerTest.h"
#include "../MDBShared.h"
#include "../MDBDebugger.h"

bool 
MDBDebuggerTest0::run() {
	MDBDebugger debugger;
	char *commandLineArguments[2];
    commandLineArguments[0] = (char *) "/Users/Maxine/Projects/Play/a.out";
    commandLineArguments[1] = NULL;
	if (debugger.createProcess(commandLineArguments) == false)
		return false;
	debugger.logState();
	return false;
};

int main() {
	MDBDebuggerTest0 a;
	a.run();
	printf("Hello World\n");
	return 0;
}
