/*
 3d.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <math.h>

#include <float.h>
#include "memcheck.h"

#include "error.hpp"
#include "3d.hpp"

#define ANG2RAD 3.14159265358979323846/180.0


matrix_t matrix_t::inverse() const {
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
	matrix_t out;
	const matrix_t& in(*this);
   float wtmp[4][8];
   float m0, m1, m2, m3, s;
   float *r0, *r1, *r2, *r3;
   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
   r0[0] = in( 0, 0), r0[1] = in( 0, 1),
      r0[2] = in( 0, 2), r0[3] = in( 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = in( 1, 0), r1[1] = in( 1, 1),
      r1[2] = in( 1, 2), r1[3] = in( 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = in( 2, 0), r2[1] = in( 2, 1),
      r2[2] = in( 2, 2), r2[3] = in( 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = in( 3, 0), r3[1] = in( 3, 1),
      r3[2] = in( 3, 2), r3[3] = in( 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
   /* choose pivot - or die */
   if (fabsf(r3[0]) > fabsf(r2[0]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (fabsf(r2[0]) > fabsf(r1[0]))
      SWAP_ROWS_FLOAT(r2, r1);
   if (fabsf(r1[0]) > fabsf(r0[0]))
      SWAP_ROWS_FLOAT(r1, r0);
   if (0.0 == r0[0])
      data_error("cannot inverse "<<this);
   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }
   /* choose pivot - or die */
   if (fabsf(r3[1]) > fabsf(r2[1]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (fabsf(r2[1]) > fabsf(r1[1]))
      SWAP_ROWS_FLOAT(r2, r1);
   if (0.0 == r1[1])
      data_error("cannot inverse "<<this);
   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }
   /* choose pivot - or die */
   if (fabsf(r3[2]) > fabsf(r2[2]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (0.0 == r2[2])
      data_error("cannot inverse "<<this);
   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
   /* last check */
   if (0.0 == r3[3])
      data_error("cannot inverse "<<this);
   s = 1.0 / r3[3];		/* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;
   m2 = r2[3];			/* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
   m1 = r1[2];			/* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
   m0 = r0[1];			/* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
   matrix_t m = {
   	r0[4],
   	r0[5], r0[6],
   	r0[7], r1[4],
   	r1[5], r1[6],
   	r1[7], r2[4],
   	r2[5], r2[6],
   	r2[7], r3[4],
   	r3[5], r3[6],
      r3[7] };
   return m;
   #undef SWAP_ROWS_FLOAT
}

quat_t quat_t::slerp(const quat_t& d,float t) const {
	if(t<=0) return *this;
	if(t>=1) return d;
	quat_t l(d);
	float cosOmega = dot(d);
	if(cosOmega<0) {
		l = -l;
		cosOmega = -cosOmega;
	}
	assert(cosOmega < 1.1f);
	if(cosOmega > 0.9999f) // very close
		return (*this*(1.0f-t))+(d*t);
	else {
		const float sinOmega = sqrt(1.0f - sqrd(cosOmega));
		const float omega = atan2(sinOmega,cosOmega);
		const float oneOverSinOmega = 1.0f / sinOmega;
		const float k0 = sin((1.0f-t)*omega)*oneOverSinOmega;
		const float k1 = sin(t*omega)*oneOverSinOmega;
		return (*this*k0)+(l*k1);
	}
}

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

vec_t vec_t::nearest(const ray_t& r) const {
	return r.nearest(*this);
}

vec_t ray_t::nearest(const vec_t& pt) const {
	const float b = nearest_inf(pt);
	if(b <= 0) return o;
	if(b >= 1) return o+d;
	return o + d*b;
}

float ray_t::nearest_inf(const vec_t& pt) const {	
	const vec_t w = pt-o;
	const float c = w.dot(d);
	if(c == 0) return 0;
	return c/ddot;
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
	if((a.x>=o.b.x)||(b.x<=o.a.x)||(a.y>=o.b.y)||(b.y<=o.a.y)||(a.z>=o.b.z)||(b.z<=o.a.z))
		return MISS;
	if((a.x>=o.a.x)&&(b.x<=o.b.x)&&(a.y>=o.a.y)&&(b.y<=o.b.y)&&(a.z>=o.a.z)&&(a.z<=o.b.z))
		return ALL;
	return SOME;
}

bool aabb_t::contains(const vec_t& p) const {
	return ((p.x>=a.x) && (p.x<b.x) && (p.y>=a.y) && (p.y<b.y) && (p.z>=a.z) && (p.z<b.z));
}

vec_t aabb_t::n(const vec_t& normal) const {
	return vec_t(
		normal.x < 0? a.x: b.x,
		normal.y < 0? a.y: b.y,
		normal.z < 0? a.z: b.z);
}

vec_t aabb_t::p(const vec_t& normal) const {
	return vec_t(
		normal.x > 0? a.x: b.x,
		normal.y > 0? a.y: b.y,
		normal.z > 0? a.z: b.z);
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
	if(a.x > b.x) panic(this<<" is infinite");
	const vec_t sz = (b-a)/2.0f;
	centre = a+sz;
	radius = sz.magnitude();
}

bounds_t bounds_t::centred(const vec_t& p) const {
	if(a.x > b.x) panic(this<<" is infinite");
	bounds_t ret(p-(centre-a),p+(b-centre));
	ret.bounds_fix();
	if(!ret.contains(p)) panic(this<<" is not anchored in the right place");
	return ret;
}

intersection_t cone_t::contains(const sphere_t& s) const {
	const float i = nearest_inf(s.centre);
	const float r = s.radius / d.magnitude(); 
	if((i<-r) || (i>r)) return MISS;
	if((i<r) || (i>(1-r))) return SOME;
	if(r > od) return SOME;
	//### ok head explodes, not finished
	return MISS; 
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

plane_t::plane_t(float a,float b,float c,float d) {
	normal = vec_t(a,b,c);
	const float l = normal.magnitude();
	normal /= l;
	this->d = d/l;
}

float plane_t::distance(const vec_t& pt) const {
	return (d + normal.dot(pt));
}

static float _mat(const matrix_t& m,int row,int col) {
	const float hmm = m(row-1,col-1);
	const float ret = m.f[col*4+row-5]; // this is NOT the same as m(row,col)
	if(hmm != ret) panic("wtf? ("<<row<<","<<col<<") "<<hmm<<", "<<ret);
	return ret;
}

frustum_t::frustum_t(const vec_t& e,const matrix_t& m): eye(e) {
	pl[0] = plane_t( // near
				 _mat(m,3,1) + _mat(m,4,1),
				 _mat(m,3,2) + _mat(m,4,2),
				 _mat(m,3,3) + _mat(m,4,3),
				 _mat(m,3,4) + _mat(m,4,4));
	pl[1] = plane_t( // far
				-_mat(m,3,1) + _mat(m,4,1),
				-_mat(m,3,2) + _mat(m,4,2),
				-_mat(m,3,3) + _mat(m,4,3),
				-_mat(m,3,4) + _mat(m,4,4));
	pl[2] = plane_t( // bottom
				 _mat(m,2,1) + _mat(m,4,1),
				 _mat(m,2,2) + _mat(m,4,2),
				 _mat(m,2,3) + _mat(m,4,3),
				 _mat(m,2,4) + _mat(m,4,4));
	pl[3] = plane_t(  // top
				-_mat(m,2,1) + _mat(m,4,1),
				-_mat(m,2,2) + _mat(m,4,2),
				-_mat(m,2,3) + _mat(m,4,3),
				-_mat(m,2,4) + _mat(m,4,4));
	pl[4] = plane_t(  // left
				 _mat(m,1,1) + _mat(m,4,1),
				 _mat(m,1,2) + _mat(m,4,2),
				 _mat(m,1,3) + _mat(m,4,3),
				 _mat(m,1,4) + _mat(m,4,4));
	pl[5] = plane_t( // right
				-_mat(m,1,1) + _mat(m,4,1),
				-_mat(m,1,2) + _mat(m,4,2),
				-_mat(m,1,3) + _mat(m,4,3),
				-_mat(m,1,4) + _mat(m,4,4));
}

intersection_t frustum_t::contains(const sphere_t& sphere) const {
	intersection_t result = ALL;
	const float radius_sqrd = sphere.radius;
	for(int i=0; i<6; i++) {
		const float d = pl[i].distance(sphere.centre);
		if (d < -radius_sqrd)
			return MISS;
		else if (d < radius_sqrd)
			result = SOME;
	}
	return result;
}

intersection_t frustum_t::contains(const aabb_t& box) const {
	intersection_t result = ALL;
	for(int i=0; i < 6; i++) {
		int out=0, in=0;
		for(int k=0; k<8 && (!in||!out); k++) {
			if (pl[i].distance(box.corner(k))<0)
				out++;
			else
				in++;
		}
		if(!in)
			return MISS;
		if(out)
			result = SOME;
	}
	return result;
}

intersection_t frustum_t::contains(const bounds_t& bounds) const {
	intersection_t ret = contains(static_cast<const sphere_t&>(bounds));
	if(SOME == ret)
		ret = contains(static_cast<const aabb_t&>(bounds));
	return ret;
}

