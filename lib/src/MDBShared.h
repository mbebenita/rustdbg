#ifndef MDBSHARED_H
#define MDBSHARED_H

#include <execinfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <mach/mach_types.h>

#include "MBList.h"

class MDBDebugger;
class MDBBreakpoint;
class MDBThread;
class MDBCode;
class MDBStack;
class MDBCodeRegion;

#define PS_UNKNOWN 1
#define PS_TERMINATED 1
#define PS_STOPPED 2

#define ENTRY_POINT_SYMBOL_NAME "__ENTRY_POINT"

#define THREAD_STATE32_REGISTER_COUNT x86_THREAD_STATE32_COUNT
#define THREAD_STATE64_REGISTER_COUNT x86_THREAD_STATE64_COUNT

#define THREAD_STATE32_COUNT x86_THREAD_STATE32_COUNT
#define THREAD_STATE64_COUNT x86_THREAD_STATE64_COUNT

#define THREAD_STATE32_FLAVOR x86_THREAD_STATE32
#define THREAD_STATE64_FLAVOR x86_THREAD_STATE64

typedef x86_thread_state32_t MDBThreadState32;
typedef x86_thread_state64_t MDBThreadState64;
typedef thread_basic_info MDBThreadInfo;


#define REPORT_MACH_ERROR(msg, kr) do { \
    if (kr != KERN_SUCCESS) {\
        void *frames[1024]; \
        int frameCount = backtrace(frames, 1024); \
        char **frameSymbols = backtrace_symbols(frames, frameCount); \
        for (int i = 0; i < frameCount; i++) { \
            log.traceLn("%s", frameSymbols[i]); \
        } \
        char *machErrorMessage = mach_error_string(kr); \
        if (machErrorMessage != NULL && strlen(machErrorMessage) != 0) { \
            log.traceLn("%s:%d: %s: %s", __FILE__, __LINE__, msg, machErrorMessage); \
        } else { \
            log.traceLn("%s:%d: %s: [errno: %d]", __FILE__, __LINE__, msg, kr); \
        } \
    } \
} while (0)

#define RETURN_ON_MACH_ERROR(msg, kr, retval) do { \
    if (kr != KERN_SUCCESS) { \
        REPORT_MACH_ERROR(msg, kr); \
        return retval; \
    } \
} while (0)

#define RETURN_VALUE_ON_ERROR(ret, msg, retval) do { \
    if (!ret) { \
        error(msg); \
        return retval; \
    } \
} while (0)

#define RETURN_ON_ERROR(ret, msg) do { \
    if (!ret) { \
        error(msg); \
        return; \
    } \
} while (0)

#define WRAP_MACH(call, retval) do { \
    kern_return_t _kr; \
    while((_kr = call) == KERN_ABORTED); \
    RETURN_ON_MACH_ERROR ("CALL", _kr, retval); \
} while (0)

int acquireTaskportRight();
void log2(const char* format, ...);
void error(const char* format, ...);
char *appendString(char *buffer, const char *format, ...);
char *allocateFormattedString(const char *format, ...);
void deallocateString(char *address);

#define LOG_CALL log("> CALL %s %s %d", __FUNCTION__, __FILE__, __LINE__)

#endif
