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
#include "error.hpp"

struct matrix_t {
	double d[16];
	inline matrix_t operator*(const matrix_t& o) const;
	inline matrix_t& operator*=(const matrix_t& o);
	inline double operator()(int r,int c) const { return d[r*4+c]; }
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
	inline float operator[](int i) const;
	inline float& operator[](int i);
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

struct face_t {
	face_t() {}
	face_t(int a_,int b_,int c_): a(a_), b(b_), c(c_) {}
	int a,b,c;
};

enum intersection_t {
	MISS, // 0 so you can do !intersection to simply test if it misses
	SOME,
	ALL,
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
	intersection_t intersects(const aabb_t& o) const;
};

struct bounds_t: public sphere_t, public aabb_t {
	bounds_t();
	bounds_t(const vec_t& a,const vec_t& b);
	void bounds_reset();
	void bounds_include(const vec_t& v);
	bool intersects(const ray_t& r) const;
	intersection_t intersects(const bounds_t& a) const;
	void bounds_fix();
	inline bounds_t operator+(const vec_t& pos) const;
};

struct triangle_t {
	triangle_t(const vec_t a_,const vec_t b_,const vec_t c_): a(a_), b(b_), c(c_) {}
	vec_t a, b, c;
	bool intersection(const ray_t& r,vec_t& I) const;
};

struct plane_t {
	plane_t() {}
	plane_t(float a_,float b_,float c_,float d_): a(a_), b(b_), c(c_), d(d_) {}
	float a, b, c, d;
	inline plane_t operator+(const plane_t& o) const;
	inline plane_t operator-(const plane_t& o) const;
	inline float dot(const vec_t& v) const;
	inline float operator[](int i) const;
	void normalise();
};

struct frustum_t {
	frustum_t() {}
	frustum_t(const vec_t& eye,const matrix_t& proj_modelview);
	intersection_t contains(const aabb_t& box) const;
	vec_t eye;
	plane_t side[6];
	bool sign[6][3];
};

inline float sqrd(float x) { return x*x; }

inline matrix_t matrix_t::operator*(const matrix_t& o) const {
	matrix_t m = {{
		d[ 0] * o.d[ 0] + d[ 1] * o.d[ 4] + d[ 2] * o.d[ 8] + d[ 3] * o.d[12],
		d[ 0] * o.d[ 1] + d[ 1] * o.d[ 5] + d[ 2] * o.d[ 9] + d[ 3] * o.d[13],
		d[ 0] * o.d[ 2] + d[ 1] * o.d[ 6] + d[ 2] * o.d[10] + d[ 3] * o.d[14],
		d[ 0] * o.d[ 3] + d[ 1] * o.d[ 7] + d[ 2] * o.d[11] + d[ 3] * o.d[15],
		d[ 4] * o.d[ 0] + d[ 5] * o.d[ 4] + d[ 6] * o.d[ 8] + d[ 7] * o.d[12],
		d[ 4] * o.d[ 1] + d[ 5] * o.d[ 5] + d[ 6] * o.d[ 9] + d[ 7] * o.d[13],
		d[ 4] * o.d[ 2] + d[ 5] * o.d[ 6] + d[ 6] * o.d[10] + d[ 7] * o.d[14],
		d[ 4] * o.d[ 3] + d[ 5] * o.d[ 7] + d[ 6] * o.d[11] + d[ 7] * o.d[15],
		d[ 8] * o.d[ 0] + d[ 9] * o.d[ 4] + d[10] * o.d[ 8] + d[11] * o.d[12],
		d[ 8] * o.d[ 1] + d[ 9] * o.d[ 5] + d[10] * o.d[ 9] + d[11] * o.d[13],
		d[ 8] * o.d[ 2] + d[ 9] * o.d[ 6] + d[10] * o.d[10] + d[11] * o.d[14],
		d[ 8] * o.d[ 3] + d[ 9] * o.d[ 7] + d[10] * o.d[11] + d[11] * o.d[15],
		d[12] * o.d[ 0] + d[13] * o.d[ 4] + d[14] * o.d[ 8] + d[15] * o.d[12],
		d[12] * o.d[ 1] + d[13] * o.d[ 5] + d[14] * o.d[ 9] + d[15] * o.d[13],
		d[12] * o.d[ 2] + d[13] * o.d[ 6] + d[14] * o.d[10] + d[15] * o.d[14],
		d[12] * o.d[ 3] + d[13] * o.d[ 7] + d[14] * o.d[11] + d[15] * o.d[15] }};
	return m;
}

inline matrix_t& matrix_t::operator*=(const matrix_t& o) {
	return *this = (*this*o);
}

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
	this->x = x * m.d[0] + y * m.d[4] + z * m.d[8] + m.d[12];
	this->y = x * m.d[1] + y * m.d[5] + z * m.d[9] + m.d[13];
	this->z = x * m.d[2] + y * m.d[6] + z * m.d[10] + m.d[14];  	
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

inline float vec_t::operator[](int i) const {
	switch(i) {
	case 0: return x;
	case 1: return y;
	case 2: return z;
	default: panic("bad index "<<i);
	}
}

inline float& vec_t::operator[](int i) {
	switch(i) {
	case 0: return x;
	case 1: return y;
	case 2: return z;
	default: panic("bad index "<<i);
	}
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

inline bounds_t bounds_t::operator+(const vec_t& pos) const {
	bounds_t ret = *this;
	ret.a += pos;
	ret.b += pos;
	ret.centre += pos;
	return ret;
}

inline plane_t plane_t::operator+(const plane_t& o) const {
	return plane_t(a+o.a,b+o.b,c+o.c,d+o.d);
}

inline plane_t plane_t::operator-(const plane_t& o) const {
	return plane_t(a-o.a,b-o.b,c-o.c,d-o.d);
}

inline float plane_t::dot(const vec_t& v) const {
	return a*v.x+b*v.y+c*v.z+d;
}

inline float plane_t::operator[](int i) const {
	switch(i) {
	case 0: return a;
	case 1: return b;
	case 2: return c;
	case 3: return d;
	default: panic("bad index "<<i);
	}
}

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

