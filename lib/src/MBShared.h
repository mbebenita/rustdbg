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

template<typename T> class MBList;

class MBString {
public:
    static char *trimLeft(char *str);
    static char *trimRight(char *str);
    static char *clone(const char *str);
    static MBList<char *> *split(char *str, const char *delimiters);

    static bool startsWith(const char *str, const char *with);
    static char *scanReverse(const char *str, char *ptr, const char chr);
};

#endif /* MBSHARED_H */
