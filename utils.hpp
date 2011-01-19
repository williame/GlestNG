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
	const size_t capacity;
private:
	size_t len;
	T* data;
};

class file_stream_t {
public:
	virtual ~file_stream_t() {}
	inline uint8_t r_byte();
	inline uint16_t r_uint16();
	inline uint32_t r_uint32();
	inline float r_float32();
	inline void r_skip(size_t n);
	static file_stream_t* open_file(const char* filename,const char* mode);
	virtual void read(void* dest,size_t bytes) = 0;
	virtual void write(const void* src,size_t bytes) = 0;
	template<int N> std::string r_fixed_str();
	virtual std::ostream& repr(std::ostream& out) const = 0;
private:
	template<typename T> T r_();
};

template<typename T> fixed_array_t<T>::fixed_array_t(size_t cap,bool filled):
	capacity(cap), len(0), data(new T[cap])
{
	if(filled) {
		len = capacity;
	} else {
		VALGRIND_MAKE_MEM_UNDEFINED(data,sizeof(T)*capacity);
	}
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

inline std::ostream& operator<<(std::ostream& out,const file_stream_t& repr) {
	return repr.repr(out);
}

template<int N> std::string file_stream_t::r_fixed_str() {
	char v[N+1];
	read(v,N);
	v[N] = 0;
	return std::string(v);
}

template<typename T> T file_stream_t::r_() {
	T v;
	read(&v,sizeof(T));
	return v;
}

// all source data and target platforms are little-endian
inline uint8_t file_stream_t::r_byte() { return r_<uint8_t>(); }
inline uint16_t file_stream_t::r_uint16() { return r_<uint16_t>(); }
inline uint32_t file_stream_t::r_uint32() { return r_<uint32_t>(); }
inline float file_stream_t::r_float32() { return r_<float>(); }

inline void file_stream_t::r_skip(size_t n) {
	while(n--) r_byte();
}

#endif //__UTILS_HPP__

