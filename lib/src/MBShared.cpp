#include "MBShared.h"

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
