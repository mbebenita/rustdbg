#ifndef MBSHARED_H
#define MBSHARED_H

#include "stdio.h"
#include "pthread.h"

template<typename ReturnType, typename Parameter>
class MBCallback {
public:
    virtual ReturnType execute(Parameter parameter) = 0;
};

template<class Class, typename ReturnType, typename Parameter>
class MBFunction : public MBCallback<ReturnType, Parameter> {
public:
    typedef ReturnType (Class::*Method)(Parameter);

    MBFunction(Class *instance, Method method) {
        this->instance = instance;
        this->method = method;
    }

    ReturnType execute(Parameter parameter) {
        return (instance->*method)(parameter);
    }

private:
    Class* instance;
    Method method;
};

template<typename T> class MBPredicate {
public:
    virtual bool operator() (T value) const = 0;
};

class MBShared {
public:
    MBShared();
    virtual ~MBShared();
};

/**
* Thread utility class. Derive and implement your own run() method.
*/
class MBThread {
private:
    volatile bool _is_running;
public:
    pthread_t thread;
    MBThread();
    void start();
    virtual void run() {
        return;
    }
    void join();
    bool isRunning();
};

#endif /* MBSHARED_H */
