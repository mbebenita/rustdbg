#ifndef MBList_H
#define MBList_H

#include "MBShared.h"

/**
 * A simple, resizable array list.
 */
template<typename T> class MBList {
    static const size_t INITIAL_CAPACITY = 8;
    size_t _length;
    T * _data;
    size_t _capacity;
public:
    MBList();
    ~MBList();
    size_t length();
    T add(T value);
    int32_t append(T value);
    int32_t push(T value);
    T pop();
    T get(size_t index);
    bool replace(T oldValue, T newValue);
    int32_t indexOf(T value);
    int32_t indexOf(MBPredicate<T> &predicate);
    bool contains(T value);
    void clear();
    bool isEmpty();
    T & operator[](size_t index);
};

template<typename T>
MBList<T>::MBList() {
    _length = 0;
    _capacity = INITIAL_CAPACITY;
    _data = (T *) malloc(sizeof(T) * _capacity);
}

template<typename T>
MBList<T>::~MBList() {
    free(_data);
}

template<typename T> size_t
MBList<T>::length() {
    return _length;
}

template<typename T> int32_t
MBList<T>::append(T value) {
    return push(value);
}

template<typename T> T
MBList<T>::add(T value) {
    if (_length == _capacity) {
        _capacity = _capacity * 2;
        _data = (T *) realloc(_data, _capacity * sizeof(T));
    }
    _data[_length ++] = value;
    return value;
}

template<typename T> int32_t
MBList<T>::push(T value) {
    if (_length == _capacity) {
        _capacity = _capacity * 2;
        _data = (T *) realloc(_data, _capacity * sizeof(T));
    }
    _data[_length ++] = value;
    return _length - 1;
}

template<typename T> T
MBList<T>::pop() {
    T value = _data[-- _length];
    return value;
}

template<typename T> T
MBList<T>::get(size_t index) {
    return _data[index];
}

/**
 * Replaces the oldValue in the list with the newValue.
 * Returns the true if the replacement succeeded, or false otherwise.
 */
template<typename T> bool
MBList<T>::replace(T oldValue, T newValue) {
    int index = indexOf(oldValue);
    if (index < 0) {
        return false;
    }
    _data[index] = newValue;
    return true;
}

template<typename T> bool
MBList<T>::contains(T value) {
    return indexOf(value) >= 0;
}

template<typename T> void
MBList<T>::clear() {
    _length = 0;
}

template<typename T> int32_t
MBList<T>::indexOf(T value) {
    for (size_t i = 0; i < _length; i++) {
        if (_data[i] == value) {
            return i;
        }
    }
    return -1;
}

template<typename T> int32_t
MBList<T>::indexOf(MBPredicate<T> &predicate) {
    for (size_t i = 0; i < _length; i++) {
        if (predicate(_data[i])) {
            return i;
        }
    }
    return -1;
}

template<typename T> T &
MBList<T>::operator[](size_t index) {
    return _data[index];
}

template<typename T> bool
MBList<T>::isEmpty() {
    return _length == 0;
}

#endif /* MBList_h */
