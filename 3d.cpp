/*
 3d.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <math.h>
#include <assert.h>
#include <float.h>
#include "memcheck.h"

#include "3d.hpp"

vec_t& vec_t::normalise() {
	const float d  = sqrt(x*x + y*y + z*z);
	if(d > 0) {
		x /= d;
		y /= d;
		z /= d;
	}
	return *this;
}

vec_t vec_t::normalise(const vec_t& v) {
	vec_t ret = v;
	return ret.normalise();
}

float vec_t::distance(const vec_t& v) const {
	return sqrt(distance_sqrd(v));
}

float vec_t::magnitude() const {
	return sqrt(magnitude_sqrd());
}

vec_t vec_t::rotate(float rad,const vec_t& axis1,const vec_t& axis2) const {
	// http://local.wasp.uwa.edu.au/~pbourke/geometry/rotate/example.c
	vec_t q1 = *this - axis1, q2;
	vec_t u = axis2 - axis1;
	u.normalise();
	const double
		d = sqrt(u.y*u.y + u.z*u.z),
		cosrad = cos(rad),
		sinrad = sin(rad);
	if(d != 0) {
		q2.x = q1.x;
		q2.y = q1.y * u.z / d - q1.z * u.y / d;
		q2.z = q1.y * u.y / d + q1.z * u.z / d;
	} else
		q2 = q1;
	q1.x = q2.x * d - q2.z * u.x;
	q1.y = q2.y;
	q1.z = q2.x * u.x + q2.z * d;
	q2.x = q1.x * cosrad - q1.y * sinrad;
	q2.y = q1.x * sinrad + q1.y * cosrad;
	q2.z = q1.z;
	q1.x =   q2.x * d + q2.z * u.x;
	q1.y =   q2.y;
	q1.z = - q2.x * u.x + q2.z * d;
	if (d != 0) {
		q2.x =   q1.x;
		q2.y =   q1.y * u.z / d + q1.z * u.y / d;
		q2.z = - q1.y * u.y / d + q1.z * u.z / d;
	} else
		q2 = q1;
	q1 = q2 + axis1;
	return q1;
}

bool sphere_t::intersects(const ray_t& r) const {
	const float a = r.d.magnitude_sqrd();
	assert(a>0);
	const float b = 2.0f * r.d.dot(r.o) - centre.dot(r.d);
	const float c = centre.distance_sqrd(r.o)-(radius*radius);
	return (b*b-4*a*c) >= 0.0f;
}

static bool _aabb_intersects_r(bool& found,float D,float d,float e,float& t0,float& t1) {
	const float es = D>0? e: -e;
	const float invDi = 1. / D;
	if(!found) {
		t0 = (d - es) * invDi;
		t1 = (d + es) * invDi;
		found = true;
	} else {
		float s = (d - es) * invDi;
		if (s > t0) t0 = s;
		s = (d + es) * invDi;
		if (s < t1) t1 = s;
		if (t0 > t1) return false;
	}
	return true;
}

bool aabb_t::intersects(const ray_t& r) const {
	unsigned parallel = 0;
	bool found = false;
	const vec_t d = a - r.o;
	float t0, t1;
	if(fabs(a.x) < 0.000001f)
		parallel |= 1 << 0;
	else if(!_aabb_intersects_r(found,r.d.x,d.x,b.x,t0,t1))
		return false;
	if(fabs(a.y) < 0.000001f)
		parallel |= 1 << 1;
	else if(!_aabb_intersects_r(found,r.d.y,d.y,b.y,t0,t1))
		return false;
	if(fabs(a.z) < 0.000001f)
		parallel |= 1 << 2;
	else if(!_aabb_intersects_r(found,r.d.z,d.z,b.z,t0,t1))
		return false;
	if(!parallel)
		return true;
	else if((parallel & (1<<0)) &&
		((fabs(d.x-t0*r.d.x)>b.x) || (fabs(d.x-t1*r.d.x)>b.x)))
		return false;
	else if((parallel & (1<<1)) &&
		((fabs(d.y-t0*r.d.y)>b.y) || (fabs(d.y-t1*r.d.y)>b.y)))
		return false;
	else if((parallel & (1<<2)) &&
		((fabs(d.z-t0*r.d.z)>b.z) || (fabs(d.z-t1*r.d.z)>b.z)))
		return false;
	return true;
}

intersection_t aabb_t::intersects(const aabb_t& o) const {
	const bool a_after = (a.x>=o.a.x && a.y>=o.a.y && a.z>=o.a.z),
		b_before = (b.x<=o.b.x && b.y<=o.b.y && b.z<=o.b.z);
	if(a_after && b_before)
		return ALL;
	const bool a_before = (a.x<=o.b.x && a.y<=o.b.y && a.z<=o.b.z);
	if(a_after&&a_before)
		return SOME;
	const bool b_after = (b.x>=o.a.x && b.y>=o.a.y && b.z>=o.a.z);
	if(b_before&&b_after)
		return SOME;
	if(!a_after&&!b_before)
		return SOME;
	return MISS;
}

static const vec_t
	BOUNDS_MIN = vec_t(FLT_MAX,FLT_MAX,FLT_MAX),
	BOUNDS_MAX = vec_t(-FLT_MAX,-FLT_MAX,-FLT_MAX);

bounds_t::bounds_t():
	sphere_t(vec_t(0,0,0),0),
	aabb_t(BOUNDS_MIN,BOUNDS_MAX)
{}

bounds_t::bounds_t(const vec_t& a,const vec_t& b):
	sphere_t(vec_t(0,0,0),0),
	aabb_t(a,b)
{
	bounds_fix();
}

void bounds_t::bounds_reset() {
	a = BOUNDS_MIN;
	b = BOUNDS_MAX;
	VALGRIND_MAKE_MEM_UNDEFINED(&centre,sizeof(centre));
	VALGRIND_MAKE_MEM_UNDEFINED(&radius,sizeof(radius));
}

void bounds_t::bounds_include(const vec_t& v) {
	if(v.x < a.x) a.x = v.x;
	if(v.y < a.y) a.y = v.y;
	if(v.z < a.z) a.z = v.z;
	if(v.x > b.x) b.x = v.x;
	if(v.y > b.y) b.y = v.y;
	if(v.z > b.z) b.z = v.z;
}

intersection_t bounds_t::intersects(const bounds_t& a) const {
	return aabb_t::intersects(a);
}

bool bounds_t::intersects(const ray_t& r) const {
	return sphere_t::intersects(r) && aabb_t::intersects(r);
}

void bounds_t::bounds_fix() {
	const vec_t sz = b-a;
	centre = vec_t(sz.x/2.0f,sz.y/2.0f,sz.z/2.0f);
	radius = sz.magnitude()/2.0f;
}

