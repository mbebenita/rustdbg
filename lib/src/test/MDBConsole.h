#ifndef MDBCONSOLE_H
#define MDBCONSOLE_H

class MDBCommand {
public:
    const char *name;
    const char *shortName;
    MBCallback<void, char *> *callback;
    MDBCommand(const char *name, const char *shortName, MBCallback<void, char *> *callback);
    bool matches(char *line);
};

class MDBConsole {
    char *processFileName;
    MBList<MDBCommand> commands;
    MDBDebugger debugger;
public:
    static MDBConsole *instance;
    MDBConsole(MBList<char*> &arguments);
    virtual ~MDBConsole();
    int run();
    void logVersion();

    void commandQuit(char *line);
    void commandOpen(char *line);
    void commandInfo(char *line);

    void commandResume(char *line);

    static char *
    readlineCommandGenerator (const char *text, int state);

    static char **
    readlineCommandCompletion(const char *text, int start, int end);

    static void initializeReadline ();

protected:
    bool runCommand(char *line);
};

#endif /* MDBCONSOLE_H */
