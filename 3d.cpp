/*
 3d.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <math.h>
#include <assert.h>
#include "3d.hpp"

void vec_t::normalise() {
	const float d  = sqrt(x*x + y*y + z*z);
	if(d > 0) {
		x /= d;
		y /= d;
		z /= d;
	}
}

float vec_t::distance(const vec_t& v) const {
	return sqrt(distance_sqrd(v));
}

bool sphere_t::intersects(const ray_t& r) const {
	const float a = r.d.mag_sqrd();
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

