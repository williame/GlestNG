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

struct quat_t;

struct matrix_t {
	float f[16];
	inline matrix_t operator*(const matrix_t& o) const;
	inline matrix_t& operator*=(const matrix_t& o);
	inline float operator()(int r,int c) const;
};

struct quat_t {
	float x, y, z, w;
	inline float dot(const quat_t& q) const;
	inline quat_t operator-() const;
	inline quat_t operator*(float f) const;
	inline quat_t operator+(const quat_t& q) const;
	quat_t slerp(const quat_t& d,float t) const;
};

struct ray_t;

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
	vec_t nearest(const ray_t& r) const;
	float distance(const vec_t& v) const;
	vec_t& normalise();
	static vec_t normalise(const vec_t& v);
	inline static vec_t midpoint(const vec_t& a,const vec_t& b);
	vec_t rotate(float rad,const vec_t& axis1,const vec_t& axis2) const;
};

struct ray_t { // a line segment
	ray_t(const vec_t& o_,const vec_t& d_): o(o_), d(d_), ddot(d_.dot(d_)) {}
	vec_t nearest(const vec_t& pt) const;
	float nearest_inf(const vec_t& pt) const;
	vec_t o, d;
	float ddot;
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
	inline vec_t corner(int corner) const;
	bool contains(const vec_t& p) const;
	vec_t n(const vec_t& normal) const;
	vec_t p(const vec_t& normal) const;
};

struct bounds_t: public sphere_t, public aabb_t {
	bounds_t();
	bounds_t(const vec_t& a,const vec_t& b);
	virtual void bounds_reset();
	virtual void bounds_include(const vec_t& v);
	virtual void bounds_fix();
	bool intersects(const ray_t& r) const;
	intersection_t intersects(const bounds_t& a) const;
	inline bounds_t operator+(const vec_t& pos) const;
	bounds_t centred(const vec_t& p) const;
};

struct cone_t: public ray_t { // with a radius at both extents
	cone_t(const vec_t& o,float o_radius_,const vec_t& d,float d_radius_):
		ray_t(o,d), o_radius(o_radius_), d_radius(d_radius_),
		od(o_radius_/d_radius_) {}
	float o_radius, d_radius, od;
	intersection_t contains(const sphere_t& sphere) const;
};

struct triangle_t {
	triangle_t(const vec_t a_,const vec_t b_,const vec_t c_): a(a_), b(b_), c(c_) {}
	vec_t a, b, c;
	bool intersection(const ray_t& r,vec_t& I) const;
};

struct plane_t {
	plane_t() {}
	plane_t(float a,float b,float c,float d); 
	float distance(const vec_t& pt) const;
	vec_t normal;
	float d;
};

struct frustum_t {
	frustum_t() {}
	frustum_t(const vec_t& e,const matrix_t& m);
	intersection_t contains(const sphere_t& sphere) const;
	intersection_t contains(const aabb_t& box) const;
	intersection_t contains(const bounds_t& bounds) const;
	plane_t pl[6];
	vec_t eye;
};

inline float sqrd(float x) { return x*x; }

inline matrix_t matrix_t::operator*(const matrix_t& o) const {
	matrix_t m = {{
		f[ 0] * o.f[ 0] + f[ 1] * o.f[ 4] + f[ 2] * o.f[ 8] + f[ 3] * o.f[12],
		f[ 0] * o.f[ 1] + f[ 1] * o.f[ 5] + f[ 2] * o.f[ 9] + f[ 3] * o.f[13],
		f[ 0] * o.f[ 2] + f[ 1] * o.f[ 6] + f[ 2] * o.f[10] + f[ 3] * o.f[14],
		f[ 0] * o.f[ 3] + f[ 1] * o.f[ 7] + f[ 2] * o.f[11] + f[ 3] * o.f[15],
		f[ 4] * o.f[ 0] + f[ 5] * o.f[ 4] + f[ 6] * o.f[ 8] + f[ 7] * o.f[12],
		f[ 4] * o.f[ 1] + f[ 5] * o.f[ 5] + f[ 6] * o.f[ 9] + f[ 7] * o.f[13],
		f[ 4] * o.f[ 2] + f[ 5] * o.f[ 6] + f[ 6] * o.f[10] + f[ 7] * o.f[14],
		f[ 4] * o.f[ 3] + f[ 5] * o.f[ 7] + f[ 6] * o.f[11] + f[ 7] * o.f[15],
		f[ 8] * o.f[ 0] + f[ 9] * o.f[ 4] + f[10] * o.f[ 8] + f[11] * o.f[12],
		f[ 8] * o.f[ 1] + f[ 9] * o.f[ 5] + f[10] * o.f[ 9] + f[11] * o.f[13],
		f[ 8] * o.f[ 2] + f[ 9] * o.f[ 6] + f[10] * o.f[10] + f[11] * o.f[14],
		f[ 8] * o.f[ 3] + f[ 9] * o.f[ 7] + f[10] * o.f[11] + f[11] * o.f[15],
		f[12] * o.f[ 0] + f[13] * o.f[ 4] + f[14] * o.f[ 8] + f[15] * o.f[12],
		f[12] * o.f[ 1] + f[13] * o.f[ 5] + f[14] * o.f[ 9] + f[15] * o.f[13],
		f[12] * o.f[ 2] + f[13] * o.f[ 6] + f[14] * o.f[10] + f[15] * o.f[14],
		f[12] * o.f[ 3] + f[13] * o.f[ 7] + f[14] * o.f[11] + f[15] * o.f[15] }};
	return m;
}

inline matrix_t& matrix_t::operator*=(const matrix_t& o) {
	return *this = (*this*o);
}

inline float matrix_t::operator()(int r,int c) const {
	assert(r>=0 && r<4);
	assert(c>=0 && c<4);
	return f[c*4+r]; // crazy swap
}

inline quat_t quat_t::operator-() const {
	quat_t ret = {-x,-y,-z,-w};
	return ret;
}

inline quat_t quat_t::operator*(float f) const {
	quat_t ret = {x*f,y*f,z*f,w*f};
	return ret;
}

inline quat_t quat_t::operator+(const quat_t& q) const {
	quat_t ret = {x+q.x,y+q.y,z+q.z,w+q.w};
	return ret;
}

inline float quat_t::dot(const quat_t& q) const {
	return x*q.x + y*q.y + z*q.z + w*q.w;
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

inline vec_t aabb_t::corner(int corner) const {
	switch(corner) {
	case 0: return a;
	case 1: return vec_t(a.x,a.y,b.z);
	case 2: return vec_t(a.x,b.y,b.z);
	case 3: return vec_t(a.x,b.y,a.z);
	case 4: return vec_t(b.x,a.y,a.z);
	case 5: return vec_t(b.x,b.y,b.z);
	case 6: return b;
	case 7: return vec_t(b.x,b.y,a.z);
	default: panic("bad index "<<corner);
	}
}

inline bounds_t bounds_t::operator+(const vec_t& pos) const {
	bounds_t ret = *this;
	ret.a += pos;
	ret.b += pos;
	ret.centre += pos;
	return ret;
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

