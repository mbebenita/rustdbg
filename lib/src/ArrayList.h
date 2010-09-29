/*
 *  ArrayList.h
 *  RustDbg
 *
 *  Created by Michael Bebenita on 8/19/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ArrayList_H
#define ArrayList_H

/**
 * A simple, resizable array list.
 */
template<typename T> class ArrayList {
    static const size_t INITIAL_CAPACITY = 8;
    size_t _length;
    T * _data;
    size_t _capacity;
public:
    ArrayList();
    ~ArrayList();
    size_t length();
    T add(T value);
    int32_t append(T value);
    int32_t push(T value);
    T pop();
    bool replace(T oldValue, T newValue);
    int32_t indexOf(T value);
    bool contains(T value);
    void clear();
    bool isEmpty();
    T & operator[](size_t index);
};

template<typename T>
ArrayList<T>::ArrayList() {
    _length = 0;
    _capacity = INITIAL_CAPACITY;
    _data = (T *) malloc(sizeof(T) * _capacity);
}

template<typename T>
ArrayList<T>::~ArrayList() {
    free(_data);
}

template<typename T> size_t
ArrayList<T>::length() {
    return _length;
}

template<typename T> int32_t
ArrayList<T>::append(T value) {
    return push(value);
}

template<typename T> T
ArrayList<T>::add(T value) {
    if (_length == _capacity) {
        _capacity = _capacity * 2;
        _data = (T *) realloc(_data, _capacity * sizeof(T));
    }
    _data[_length ++] = value;
    return value;
}

template<typename T> int32_t
ArrayList<T>::push(T value) {
    if (_length == _capacity) {
        _capacity = _capacity * 2;
        _data = (T *) realloc(_data, _capacity * sizeof(T));
    }
    _data[_length ++] = value;
    return _length - 1;
}

template<typename T> T
ArrayList<T>::pop() {
    T value = _data[-- _length];
    return value;
}

/**
 * Replaces the oldValue in the list with the newValue.
 * Returns the true if the replacement succeeded, or false otherwise.
 */
template<typename T> bool
ArrayList<T>::replace(T oldValue, T newValue) {
    int index = indexOf(oldValue);
    if (index < 0) {
        return false;
    }
    _data[index] = newValue;
    return true;
}

template<typename T> bool
ArrayList<T>::contains(T value) {
    return indexOf(value) >= 0;
}

template<typename T> void
ArrayList<T>::clear() {
    _length = 0;
}

template<typename T> int32_t
ArrayList<T>::indexOf(T value) {
    for (size_t i = 0; i < _length; i++) {
        if (_data[i] == value) {
            return i;
        }
    }
    return -1;
}

template<typename T> T &
ArrayList<T>::operator[](size_t index) {
    return _data[index];
}

template<typename T> bool
ArrayList<T>::isEmpty() {
    return _length == 0;
}

#endif /* ArrayList_H */
