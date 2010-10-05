#ifndef MBSHARED_H
#define MBSHARED_H

#include "stdio.h"

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

#endif /* MBSHARED_H */
