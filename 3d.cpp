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

#include "error.hpp"
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
	const vec_t dst = r.o - centre;
	const float B = sqrd(dst.dot(r.d));
	const float C = dst.dot(dst) - sqrd(radius);
	return (B-C)>0;
}

bool aabb_t::intersects(const ray_t& r) const {
        const bool xsign = (r.d.x < 0.0);
        const float invx = 1.0 / r.d.x;
        float tmin = ((xsign?b.x:a.x) - r.o.x) * invx;
        float tmax = ((xsign?a.x:b.x) - r.o.x) * invx;
        const bool ysign = (r.d.y < 0.0);
        const float invy = 1.0 / r.d.y;
        const float tymin = ((ysign?b.y:a.y) - r.o.y) * invy;
        const float tymax = ((ysign?a.y:b.y) - r.o.y) * invy;
        if((tmin > tymax) || (tymin > tmax))
                return false;
        if(tymin > tmin) tmin = tymin;
        if(tymax < tmax) tmax = tymax;
        const bool zsign = (r.d.z < 0.0);
        const float invz = 1.0 / r.d.z;
        const float tzmin = ((zsign?b.z:a.z) - r.o.z) * invz;
        const float tzmax = ((zsign?a.z:b.z) - r.o.z) * invz;
        if((tmin > tzmax) || (tzmin > tmax))
                return false;
        if(tzmin > tmin) tmin = tzmin;
        if(tzmax < tmax) tmax = tzmax;
        return (tmin < 1.0) && (tmax > 0.0);
}

intersection_t aabb_t::intersects(const aabb_t& o) const {
	if((a.x>=o.b.x)||(b.x<o.a.x)||(a.y>=o.b.y)||(b.y<o.a.y)||(a.z>=o.b.z)||(b.z<o.a.z))
		return MISS;
	if((a.x>=o.a.x)&&(b.x<o.b.x)&&(a.y>=o.a.y)&&(b.y<o.b.y)&&(a.z>=o.a.z)&&(a.z<o.b.z))
		return ALL;
	return SOME;
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
	const vec_t sz = (b-a)/2.0f;
	centre = a+sz;
	radius = sz.magnitude();
}

bool triangle_t::intersection(const ray_t& r,vec_t& I) const {
    // http://softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm#intersect_RayTriangle%28%29
    // get triangle edge vectors and plane normal
    const vec_t u = b-a;
    const vec_t v = c-a;
    const vec_t n = u.cross(v);
    if(n.x==0 && n.y==0 && n.z==0) return false; // triangle is degenerate
    const vec_t w0 = r.o-a;
    const float j = n.dot(r.d);
    if(fabs(j) < 0.00000001) return false; // parallel, disjoint or on plane
    const float i = -n.dot(w0);
    // get intersect point of ray with triangle plane
    const float k = i / j;
    if(k < 0.0) return false; // ray goes away from triangle
    // for a segment, also test if (r > 1.0) => no intersect
    I = r.o + r.d * k; // intersect point of ray and plane
    // is I inside T?
    const float uu = u.dot(u);
    const float uv = u.dot(v);
    const float vv = v.dot(v);
    const vec_t w = I - a;
    const float wu = w.dot(u);
    const float wv = w.dot(v);
    const float D = uv * uv - uu * vv;
    const float s = (uv * wv - vv * wu) / D;
    if(s<0.0 || s>1.0) return false; // I is outside T
    const float t = (uv * wu - uu * wv) / D;
    if(t<0.0 || (s+t)>1.0) return false; // I is outside T
    return true; // I is in T
}

frustum_t::frustum_t(const matrix_t& projection,const matrix_t& modelview) {
	// http://www.crownandcutlass.com/features/technicaldetails/frustum.html
	const matrix_t clip = projection * modelview;
	/* Extract the numbers for the RIGHT plane */
	side[0][0] = clip.d[ 3] - clip.d[ 0];
	side[0][1] = clip.d[ 7] - clip.d[ 4];
	side[0][2] = clip.d[11] - clip.d[ 8];
	side[0][3] = clip.d[15] - clip.d[12];
	/* Normalize the result */
	double t = sqrt(sqrd(side[0][0])+sqrd(side[0][1])+sqrd(side[0][2]));
	side[0][0] /= t;
	side[0][1] /= t;
	side[0][2] /= t;
	side[0][3] /= t;
	/* Extract the numbers for the LEFT plane */
	side[1][0] = clip.d[ 3] + clip.d[ 0];
	side[1][1] = clip.d[ 7] + clip.d[ 4];
	side[1][2] = clip.d[11] + clip.d[ 8];
	side[1][3] = clip.d[15] + clip.d[12];
	/* Normalize the result */
	t = sqrt(sqrd(side[1][0])+sqrd(side[1][1])+sqrd(side[1][2]));
	side[1][0] /= t;
	side[1][1] /= t;
	side[1][2] /= t;
	side[1][3] /= t;	
	/* Extract the BOTTOM plane */
	side[2][0] = clip.d[ 3] + clip.d[ 1];
	side[2][1] = clip.d[ 7] + clip.d[ 5];
	side[2][2] = clip.d[11] + clip.d[ 9];
	side[2][3] = clip.d[15] + clip.d[13];
	/* Normalize the result */
	t = sqrt(sqrd(side[2][0])+sqrd(side[2][1])+sqrd(side[2][2]));
	side[2][0] /= t;
	side[2][1] /= t;
	side[2][2] /= t;
	side[2][3] /= t;
	/* Extract the TOP plane */
	side[3][0] = clip.d[ 3] - clip.d[ 1];
	side[3][1] = clip.d[ 7] - clip.d[ 5];
	side[3][2] = clip.d[11] - clip.d[ 9];
	side[3][3] = clip.d[15] - clip.d[13];
	/* Normalize the result */
	t = sqrt(sqrd(side[3][0])+sqrd(side[3][1])+sqrd(side[3][2]));
	side[3][0] /= t;
	side[3][1] /= t;
	side[3][2] /= t;
	side[3][3] /= t;
	/* Extract the FAR plane */
	side[4][0] = clip.d[ 3] - clip.d[ 2];
	side[4][1] = clip.d[ 7] - clip.d[ 6];
	side[4][2] = clip.d[11] - clip.d[10];
	side[4][3] = clip.d[15] - clip.d[14];
	/* Normalize the result */
	t = sqrt(sqrd(side[4][0])+sqrd(side[4][1])+sqrd(side[4][2]));
	side[4][0] /= t;
	side[4][1] /= t;
	side[4][2] /= t;
	side[4][3] /= t;
	/* Extract the NEAR plane */
	side[5][0] = clip.d[ 3] + clip.d[ 2];
	side[5][1] = clip.d[ 7] + clip.d[ 6];
	side[5][2] = clip.d[11] + clip.d[10];
	side[5][3] = clip.d[15] + clip.d[14];
	/* Normalize the result */
	t = sqrt(sqrd(side[5][0])+sqrd(side[5][1])+sqrd(side[5][2]));
	side[5][0] /= t;
	side[5][1] /= t;
	side[5][2] /= t;
	side[5][3] /= t;	
}

bool frustum_t::contains(const vec_t& pt) const {
	for(int p=0; p<6; p++ )
		if(side[p][0]*pt.x+side[p][1]*pt.y+side[p][2]*pt.z+side[p][3] <= 0)
			return false;
	return true;
}

intersection_t frustum_t::contains(const sphere_t& s,double& d) const {
	int c = 0;
	for(int p=0; p<6; p++ ) {
		d = side[p][0]*s.centre.x+side[p][1]*s.centre.y+side[p][2]*s.centre.z+side[p][3];
		if(d <= -s.radius)
			return MISS;
		if(d > s.radius)
			c++;
	}
   	d += s.radius;
   	return (c==6? ALL: SOME);
}

intersection_t frustum_t::contains(const aabb_t& box) const {
	// occasional false positives
	intersection_t ret = ALL;
	for(int p=0; p<6; p++ ) {
		int c = 0;
		if(side[p][0]*box.a.x+side[p][1]*box.a.y+side[p][2]*box.a.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.b.x+side[p][1]*box.a.y+side[p][2]*box.a.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.a.x+side[p][1]*box.b.y+side[p][2]*box.a.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.b.x+side[p][1]*box.b.y+side[p][2]*box.a.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.a.x+side[p][1]*box.a.y+side[p][2]*box.b.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.b.x+side[p][1]*box.a.y+side[p][2]*box.b.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.a.x+side[p][1]*box.b.y+side[p][2]*box.b.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(side[p][0]*box.b.x+side[p][1]*box.b.y+side[p][2]*box.b.z+side[p][3]>0) {
			if(ret == SOME) continue; else c++;
		}
		if(!c) return MISS;
		if(c!=8) ret = SOME;
	}
	return ret;
}

intersection_t frustum_t::contains(const bounds_t& bounds,double& d) const {
	switch(contains((const sphere_t&)bounds,d)) {
		case ALL:
			return ALL;
		case SOME:
			return contains((const aabb_t&)bounds);
		case MISS:
			return MISS;
		default: panic("wtf is the compiler warning me about?");
	}
}

