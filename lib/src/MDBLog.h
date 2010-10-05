#ifndef MDBLOG_H
#define MDBLOG_H

class MDBLog {
private:
    uint32_t _indent;
    uint32_t _typeBitMask;
public:
    enum AnsiColor {
        WHITE,
        RED,
        LIGHTRED,
        GREEN,
        LIGHTGREEN,
        YELLOW,
        LIGHTYELLOW,
        BLUE,
        LIGHTBLUE,
        MAGENTA,
        LIGHTMAGENTA,
        TEAL,
        LIGHTTEAL
    };

    enum LogType {
        ERR = 0x1,
        DEBUG = 0x2,
        CALL = 0x4,
        SYM = 0x8,
        INFO = 0x10,
        THREAD = 0x20,
        ALL = 0xffffffff
    };

    MDBLog();
    virtual ~MDBLog();
    void indent();
    void outdent();
    void resetIndent(uint32_t indent);
    MDBLog::AnsiColor getColorForType(uint32_t typeBits);
    void traceLn(AnsiColor color, uint32_t typeBits, char *message);
    void traceLn(uint32_t typeBits, const char *format, ...);
    void traceLn(const char *format, ...);
    void traceDividerLn();
    void traceDividerLn(const char *format, ...);
    bool isTracing(uint32_t typeBits);
};

static MDBLog log;

#endif /* MDBLOG_H */
