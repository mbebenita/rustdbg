#include "../MDBShared.h"
#include "../MBList.h"
#include "../MBShared.h"
#include "../MDBShared.h"
#include "../MDBDebugger.h"
#include "../MDBBreakpoint.h"
#include "../MDBThread.h"
#include "../MDBProcess.h"
#include "MDBConsole.h"

#include "readline/readline.h"

#include <stdio.h>

MDBCommand::MDBCommand(const char *name, const char *shortName, MBCallback<void, char *> *callback) :
    name(name), shortName(shortName), callback(callback) {
}

bool MDBCommand::matches(char *line) {
    return strcmp(line, shortName) == 0 || strcmp(line, name) == 0;
}

MDBConsole* MDBConsole::instance = NULL;

MDBConsole::MDBConsole(MBList<char*> &arguments) : processFileName(NULL) {
    if (arguments.length() > 1) {
        processFileName = arguments[1];
    }

    instance = this;

    commands.add(MDBCommand("quit", "q", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandQuit)));
    commands.add(MDBCommand("info", "i", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandInfo)));
    commands.add(MDBCommand("resume", "r", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandResume)));

    logVersion();
}

MDBConsole::~MDBConsole() {
}

void
MDBConsole::logVersion() {
    log.traceLn("Rust Debugger - Version: 0.1 - Copyright (C) 2010 Mozilla Foundation");
}

void
MDBConsole::commandQuit(char *line) {
    exit(0);
}

void
MDBConsole::commandOpen(char *line) {
    printf("Opening ..\n");
}

void
MDBConsole::commandInfo(char *line) {
    debugger.process->logExecutionState("INFO");
}

void
MDBConsole::commandResume(char *line) {
    debugger.resume();
}

bool
MDBConsole::runCommand(char *line) {
    bool commandFound = false;
    for(uint32_t i = 0;i < commands.length();i++){
        if(commandFound = commands[i].matches(line)){
            commands[i].callback->execute(line);
            break;
        }
    }

    return commandFound;
}

int
MDBConsole::run() {
    initializeReadline();

    char *commandLineArguments[2];
    commandLineArguments[0] = processFileName;
    commandLineArguments[1] = NULL;

    debugger.createProcess(commandLineArguments);
    char lastCommand [1024];

    while (true) {
        char *line = readline("mdb: ");
        if (strlen(line) == 0) {
            strcpy(line, lastCommand);
        }
        strcpy(lastCommand, line);
        if (runCommand(line) == false) {
            printf("command %s not found\n", line);
        }
        if (debugger.process->running == false) {
            printf("process terminated, exiting ...\n");
            break;
        }
    }
    return 1;
}

char *
MDBConsole::readlineCommandGenerator (const char *text, int state) {
    static int readlineCommandGeneratorIndex;
    if (state == 0) {
        readlineCommandGeneratorIndex = instance->commands.length() - 1;
    }
    while (readlineCommandGeneratorIndex >= 0) {
        if (strcmp(text, "") == 0 ||
            strstr(instance->commands[readlineCommandGeneratorIndex].name, text) != NULL) {
            char *str = new char[1024];
            strcpy(str, instance->commands[readlineCommandGeneratorIndex].name);
            readlineCommandGeneratorIndex -= 1;
            return (char *) str;
        } else {
            readlineCommandGeneratorIndex -= 1;
        }
    }
    return NULL;
}

char **
MDBConsole::readlineCommandCompletion(const char *text, int start, int end) {
    char **matches;
    matches = (char **) NULL;
    if (start >= 0) {
        matches = rl_completion_matches(text, MDBConsole::readlineCommandGenerator);
    } else {
        rl_bind_key('\t', rl_complete);
    }
    return matches;
}

void
MDBConsole::initializeReadline () {
    rl_attempted_completion_function = MDBConsole::readlineCommandCompletion;
}

int main(int argc, char **argv) {
    MBList<char *> arguments;
    for (int i = 0; i < argc; i++) {
        arguments.add(argv[i]);
    }
    MDBConsole console (arguments);
    return console.run();
}
