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

#include <string>
#include <vector>
#include "memcheck.h"

#include "error.hpp"

struct tagged_string_t: public std::string {
	tagged_string_t(): tag(-1) {}
	tagged_string_t(const char* s): std::string(s), tag(-1) {}
	tagged_string_t(const std::string& s): std::string(s), tag(-1) {}
	tagged_string_t(const std::string& s,int t): std::string(s), tag(t) {}
	int tag;
};

struct strings_t: public std::vector<tagged_string_t> {
	using std::vector<tagged_string_t>::push_back;
	void push_back(const std::string& s,int tag) {
		push_back(tagged_string_t(s,tag));
	}
};

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

template<typename T> int binary_search(const std::vector<T>& vec,const T& key) {
	// painful that we write this ourselves; is it not standard somewhere?
	int start = 0, end = vec.size();
	while(start < end) {
		const unsigned middle = (start + ((end - start) / 2));
		if(vec[middle] == key)
			return middle;
		if(vec[middle] < key)
			start = middle + 1;
		else
			end = middle;
	}
	return -1; // not found
}

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

inline std::ostream& operator<<(std::ostream& out,const tagged_string_t& s) {
	out << "tag<" << static_cast<const std::string&>(s);
	if(s.tag != -1) out << ',' << s.tag;
	return out << '>';
}

#endif //__UTILS_HPP__

