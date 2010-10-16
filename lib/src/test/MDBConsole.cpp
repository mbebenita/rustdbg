#include "../MDBShared.h"
#include "../MBList.h"
#include "../MBShared.h"
#include "../MDBShared.h"
#include "../MDBDebugger.h"
#include "../MDBBreakpoint.h"
#include "../MDBThread.h"
#include "../MDBProcess.h"
#include "../MDBCode.h"
#include "../MDBMachReader.h"
#include "readline/readline.h"
#include "readline/history.h"
#include "MDBConsole.h"
#include <stdio.h>

MDBCommand::MDBCommand(const char *name, const char *shortName, MBCallback<void, char *> *callback, rl_compentry_func_t *completionFunction) :
    name(name), shortName(shortName), callback(callback), completionFunction(completionFunction) {
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

    commands.add(MDBCommand("quit", "q", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandQuit), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("info", "i", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandInfo), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("info-symbols", "is", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandInfo), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("print-back-trace", "is", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandPrintBackTrace), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("resume", "r", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandResume), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("step", "si", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandStep), MDBConsole::readlineCommandGenerator));
    commands.add(MDBCommand("break", "b", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandStep), MDBConsole::readlineSymbolGenerator));

    commands.add(MDBCommand("profile", "p", new MBFunction<MDBConsole, void, char*> (this, &MDBConsole::commandProfile), MDBConsole::readlineCommandGenerator));

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
MDBConsole::commandPrintBackTrace(char *line) {

}

void
MDBConsole::commandResume(char *line) {
    debugger.resume();
}

void
MDBConsole::commandStep(char *line) {
    debugger.step(debugger.process->threads[0], false);
    debugger.process->logExecutionState("INFO");
}

void
MDBConsole::commandProfile(char *line) {
    debugger.process->machSample();
}

bool
MDBConsole::runCommand(char *line) {
    bool commandFound = false;
    for(uint32_t i = 0;i < commands.length();i++) {
        if(commandFound = commands[i].matches(line)) {
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
    debugger.code.loadSymbols(processFileName);
    char lastCommand [1024];

    while (true) {
        char *line = readline("mdb: ");
        if (strlen(line) == 0) {
            strcpy(line, lastCommand);
        }
        strcpy(lastCommand, line);
        add_history(line);
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
duplicate(const char *string) {
    char *str = (char *) malloc(strlen(string) + 1);
    strcpy(str, string);
    return str;
}

char **
MDBConsole::readlineCommandCompletion(const char *text, int start, int end) {
    // MBString::trimRight(rl_line_buffer);
    rl_attempted_completion_over = true;

    // printf("Command Completion text: [%s] start: %d end: %d\n", text, start, end);
    char **matches = NULL;
    if (start >= 0) {
        MBList<char *> *tokens = MBString::split(rl_line_buffer, " ");
        MDBCommand *command = tokens->isEmpty() ? NULL : findCommand(tokens->get(0));
        if (command == NULL) {
            matches = rl_completion_matches(text, MDBConsole::readlineCommandGenerator);
        } else {
            matches = rl_completion_matches(text, command->completionFunction);
        }
        delete tokens;
    }
    return matches;
}

char *
MDBConsole::readlineCommandGenerator (const char *text, int state) {
    // printf("Command Generator [%s] [%s], state: %d\n", rl_line_buffer, text, state);
    static MBList<MDBCommand*> commands;
    if (state == 0) {
        commands.clear();
        for (uint32_t i = 0; i < instance->commands.length(); i++) {
            if (MBString::startsWith(instance->commands[i].name, text)) {
                commands.add(&instance->commands[i]);
            }
        }
    }

    while (commands.isEmpty() == false) {
        const char *name = commands.pop()->name;
        // char *suff = MBString::scanReverse(name, (char *) name + strlen(rl_line_buffer), ' ');
        return MBString::clone(name);
    }

    return NULL;
}

char *
MDBConsole::readlineSymbolGenerator (const char *text, int state) {
    static MBList<MDBSymbol*> symbols;
    if (state == 0) {
        symbols.clear();
        for (uint32_t i = 0; i < instance->debugger.code.files.length(); i++) {
            MDBMachFile *file = instance->debugger.code.files[i];
            for (uint32_t j = 0; j < file->symbols.length(); j++) {
                MDBSymbol *symbol = file->symbols[j];
                if (MBString::startsWith(symbol->name, text)) {
                    symbols.add(symbol);
                }
            }
        }
    }
    while (symbols.isEmpty() == false) {
        return MBString::clone(symbols.pop()->name);
    }
    return NULL;
}

MDBCommand *
MDBConsole::findCommand(char *name) {
    for (size_t i = 0; i < instance->commands.length(); i++) {
        if (strcmp(instance->commands[i].name, name) == 0) {
            return &instance->commands[i];
        }
    }
    return NULL;
}

void
MDBConsole::initializeReadline () {
    rl_attempted_completion_function = MDBConsole::readlineCommandCompletion;
    rl_basic_word_break_characters = (char *) " ";
    rl_bind_key('\t', rl_complete);
}

int main(int argc, char **argv) {
    MBList<char *> arguments;
    for (int i = 0; i < argc; i++) {
        arguments.add(argv[i]);
    }
    MDBConsole console (arguments);
    return console.run();
}
