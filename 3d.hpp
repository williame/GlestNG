/*
 3d.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __3D_HPP__
#define __3D_HPP__

#include <stddef.h>
#include <assert.h>
#include <iostream>

struct matrix_t {
	float f[16];
};

struct vec_t {
	vec_t() {}
	vec_t(float x_,float y_,float z_): x(x_), y(y_), z(z_) {}
	float x,y,z;
	inline vec_t& operator-=(const vec_t& v);
	inline vec_t operator-(const vec_t& v) const;
	inline vec_t& operator*=(const matrix_t& m);
	inline vec_t operator*(const matrix_t& m) const;
	inline vec_t& operator*=(float d);
	inline vec_t operator*(float d) const;
	inline vec_t& operator+=(const vec_t& v);
	inline vec_t operator+(const vec_t& v) const;
	inline vec_t& operator/=(float d);
	inline vec_t operator/(float d) const;
	inline vec_t cross(const vec_t& v) const;
	inline float dot(const vec_t& v) const;
	inline float magnitude_sqrd() const;
	float magnitude() const;
	inline float distance_sqrd(const vec_t& v) const;
	float distance(const vec_t& v) const;
	vec_t& normalise();
	static vec_t normalise(const vec_t& v);
	inline static vec_t midpoint(const vec_t& a,const vec_t& b);
	vec_t rotate(float rad,const vec_t& axis1,const vec_t& axis2) const;
};

struct ray_t {
	ray_t(const vec_t& o_,const vec_t& d_): o(o_), d(d_) {}
	vec_t o, d;
};

enum intersection_t {
	SOME,
	ALL,
	MISS,
};

struct sphere_t {
	sphere_t(const vec_t& c,float r): centre(c), radius(r) {}
	vec_t centre;
	float radius;
	inline intersection_t intersects(const sphere_t& s) const;
	bool intersects(const ray_t& r) const;
};

struct aabb_t { //axis-aligned bounding box
	aabb_t(const vec_t& a_,const vec_t& b_): a(a_), b(b_) {}
	vec_t a, b;
	bool intersects(const ray_t& r) const;
	intersection_t intersects(const aabb_t& a) const;
};

struct bounds_t: public sphere_t, public aabb_t {
	bounds_t();
	bounds_t(const vec_t& a,const vec_t& b);
	void bounds_reset();
	void bounds_include(const vec_t& v);
	bool intersects(const ray_t& r) const;
	intersection_t intersects(const bounds_t& a) const;
	void bounds_fix();
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
	const size_t capacity;
private:
	size_t len;
	T* data;
};

inline float sqrd(float x) { return x*x; }

inline vec_t& vec_t::operator-=(const vec_t& v) {
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

inline vec_t vec_t::operator-(const vec_t& v) const {
	vec_t ret = *this;
	ret -= v;
	return ret;
}

inline vec_t& vec_t::operator*=(const matrix_t& m) {
	const float x = this->x, y = this->y, z = this->z;
	this->x = x * m.f[0] + y * m.f[4] + z * m.f[8] + m.f[12];
	this->y = x * m.f[1] + y * m.f[5] + z * m.f[9] + m.f[13];
	this->z = x * m.f[2] + y * m.f[6] + z * m.f[10] + m.f[14];  	
	return *this;
}

inline vec_t vec_t::operator*(const matrix_t& m) const {
	vec_t ret = *this;
	ret *= m;
	return ret;
}

inline vec_t& vec_t::operator*=(float d) {
	x *= d;
	y *= d;
	z *= d;
	return *this;
}

inline vec_t vec_t::operator*(float d) const {
	vec_t ret = *this;
	ret *= d;
	return ret;
}

inline vec_t& vec_t::operator+=(const vec_t& v) {
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

inline vec_t vec_t::operator+(const vec_t& v) const {
	vec_t ret = *this;
	ret += v;
	return ret;
}

inline vec_t& vec_t::operator/=(float d) {
	x /= d;
	y /= d;
	z /= d;
	return *this;
}

inline vec_t vec_t::operator/(float d) const {
	vec_t ret = *this;
	ret /= d;
	return ret;
}

inline vec_t vec_t::cross(const vec_t& v) const {
	return vec_t(y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x);
}

inline float vec_t::dot(const vec_t& v) const {
	return x*v.x + y*v.y + z*v.z;
}

inline float vec_t::magnitude_sqrd() const {
	return sqrd(x) + sqrd(y) + sqrd(z);
}

inline float vec_t::distance_sqrd(const vec_t& v) const {
	const float xx = (x-v.x), yy = (y-v.y), zz = (z-v.z);
	return sqrd(xx) + sqrd(yy) + sqrd(zz);
}

inline vec_t vec_t::midpoint(const vec_t& a,const vec_t& b) {
	return vec_t((a.x+b.x)/2.0,(a.y+b.y)/2.0,(a.z+b.z)/2.0);
}

inline intersection_t sphere_t::intersects(const sphere_t& s) const {
	const float b = sqrd(radius+s.radius);
	const float d = centre.distance_sqrd(s.centre);
	if((radius<=s.radius) && (d<sqrd(s.radius-radius)))
		return ALL;
	if(d<b)
		return SOME;
	return MISS;
}

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

// pretty printers for logs and panics and things

inline std::ostream& operator<<(std::ostream& out,const vec_t& v) {
	out << "vec_t<" << v.x << "," << v.y << "," << v.z << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const ray_t& r) {
	out << "ray_t<" << r.o << "," << r.d << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const sphere_t& s) {
	out << "sphere_t<" << s.centre << "," << s.radius << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const aabb_t& a) {
	out << "aabb_t<" << a.a << "," << a.b << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const bounds_t& a) {
	out << "bounds_t<" << static_cast<const aabb_t&>(a) << "," << static_cast<const sphere_t&>(a) << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,intersection_t i) {
	out << "intersection<";
	switch(i) {
	case ALL: out << "ALL"; break;
	case SOME: out << "SOME"; break;
	case MISS: out << "MISS"; break;
	default: out << (int)i << "!";
	}
	out << ">";
	return out;
}

#endif //__3D_HPP__

