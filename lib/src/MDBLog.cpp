/*
 * MDBLog.cpp
 *
 *  Created on: Sep 29, 2010
 *      Author: Michael
 */

#include "MDBShared.h"
#include "MDBLog.h"
#include <stdarg.h>

static uint32_t readTypeBitMask() {
    uint32_t bits = MDBLog::CALL | MDBLog::ERR;
    char *env_str = getenv("MDB_LOG");
    if (env_str) {
        bits = 0;
        bits |= strstr(env_str, "all") ? MDBLog::ALL : 0;
        bits = strstr(env_str, "none") ? 0 : bits;
    }
    return bits;
}

static const char * _foregroundColors[] = { "[37m", "[31m", "[1;31m", "[32m",
                                            "[1;32m", "[33m", "[1;33m",
                                            "[31m", "[1;31m", "[35m",
                                            "[1;35m", "[36m", "[1;36m" };

// static MDBLog log;

char *
appendString(char *buffer, MDBLog::AnsiColor color, const char *format, ...) {
    if (buffer != NULL && format) {
        appendString(buffer, "\x1b%s", _foregroundColors[color]);
        va_list args;
        va_start(args, format);
        vsprintf(buffer + strlen(buffer), format, args);
        va_end(args);
        appendString(buffer, "\x1b[0m");
    }
    return buffer;
}

MDBLog::MDBLog() :
    _indent(0), _typeBitMask(readTypeBitMask()) {

}

MDBLog::~MDBLog() {

}

void MDBLog::traceLn(uint32_t typeBits, const char *format, ...) {
    if (isTracing(typeBits)) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        fprintf(stdout, "\n");
        va_end(ap);
    }
}

void MDBLog::traceLn(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void MDBLog::traceLn(AnsiColor color, uint32_t typeBits, char *message) {
    if (isTracing(typeBits)) {
        if (true) {
            char buffer[512] = "";
            appendString(buffer, color, "%s", message);
            traceLn(typeBits, buffer);
        } else {
            traceLn(typeBits, message);
        }
    }
}

bool MDBLog::isTracing(uint32_t typeBits) {
    return typeBits & _typeBitMask;
}

void MDBLog::indent() {
    _indent++;
}

void MDBLog::outdent() {
    _indent--;
}

void MDBLog::resetIndent(uint32_t indent) {
    _indent = indent;
}
