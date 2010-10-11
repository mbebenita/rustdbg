#include "MBShared.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"

#include "MBList.h"

MBShared::MBShared() {
    // TODO Auto-generated constructor stub
}

MBShared::~MBShared() {
    // TODO Auto-generated destructor stub
}

MBThread::MBThread() :
    _is_running(false), thread(0) {
    // Nop.
}


static void *
MBThreadStart(void *ptr) {
    MBThread *thread = (MBThread *) ptr;
    thread->run();
    return 0;
}

void MBThread::start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, MBThreadStart, (void *) this);
    _is_running = true;
}

void MBThread::join() {
    if (thread) {
        pthread_join(thread, NULL);
    }
    thread = 0;
    _is_running = false;
}

bool MBThread::isRunning() {
    return _is_running;
}

char *
MBString::trimLeft(char *str) {
    while(isspace(*str)) {
        str += 1;
    }
    return str;
}

char *
MBString::trimRight(char *str) {
    if (strlen(str) > 0) {
        char* ptr = str + strlen(str);
        while (isspace(*--ptr));
        *(ptr + 1) = '\0';
    }
    return str;
}

char *
MBString::clone(const char *str) {
    char *buf = (char *)malloc(strlen(str) + 1);
    strcpy(buf, str);
    return buf;
}

bool
MBString::startsWith(const char *str, const char *with) {
    return strstr(str, with) == str;
}

char *
MBString::scanReverse(const char *str, char *ptr, const char chr) {
    while (str != ptr && *ptr != chr) {
        ptr -= 1;
    }
    return ptr;
}

MBList<char *> *
MBString::split(char *str, const char *delimiters) {
    MBList<char *> *tokens = new MBList<char *>();
    str = MBString::clone(str);
    char *ptr = strtok(str, delimiters);
    while (ptr != NULL) {
         tokens->add(MBString::clone(ptr));
         ptr = strtok(NULL, delimiters);
    }
    free(str);
    return tokens;
}
