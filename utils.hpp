/*
 utils.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <ostream>
#include <inttypes.h>
#include <assert.h>
#include <string>
#include "memcheck.h"

template<typename T> class fixed_array_t {
public:
	fixed_array_t(size_t capacity,bool filled=false);
	virtual ~fixed_array_t() { delete[] data; }
	T* ptr() const { return data; }
	size_t append(T t);
	T& operator[](size_t i);
	const T& operator[](size_t i) const;
	size_t size() const { return len; }
	bool full() const { return len==capacity; }
	void fill(const T& t);
	void clear();
	const size_t capacity;
private:
	size_t len;
	T* data;
};

float randf();

template<typename T> fixed_array_t<T>::fixed_array_t(size_t cap,bool filled):
	capacity(cap), len(0), data(new T[cap])
{
	if(filled) {
		len = capacity;
	} else {
		VALGRIND_MAKE_MEM_UNDEFINED(data,sizeof(T)*capacity);
	}
}

template<typename T> void fixed_array_t<T>::clear() {
	len = 0;
	VALGRIND_MAKE_MEM_UNDEFINED(data,sizeof(T)*capacity);
}

template<typename T> size_t fixed_array_t<T>::append(T t) {
	assert(len<capacity);
	data[len] = t;
	return len++;
}

template<typename T> T& fixed_array_t<T>::operator[](size_t i) {
	assert(i<len);
	return data[i];
}
	
template<typename T> const T& fixed_array_t<T>::operator[](size_t i) const {
	assert(i<len);
	return data[i];
}

template<typename T> void fixed_array_t<T>::fill(const T& t) {
	for(size_t i=0; i<capacity; i++)
		data[i] = t;
	len = capacity;
}

#endif //__UTILS_HPP__

