/*
 2d.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __2D_HPP__
#define __2D_HPP__

#include <ostream>

struct vec2_t {
	vec2_t() {}
	vec2_t(short x_,short y_): x(x_), y(y_) {}
	short x, y;
	inline vec2_t& operator+=(const vec2_t& a);
	inline vec2_t operator+(const vec2_t& a) const;
	inline vec2_t& operator-=(const vec2_t& a);
	inline vec2_t operator-(const vec2_t& a) const;
	inline static void normalise(vec2_t& tl,vec2_t& br);
};

struct rect_t {
	rect_t() {}
	rect_t(const vec2_t& tl_,const vec2_t& br_): tl(tl_), br(br_) {
		normalise();
	}
	rect_t(short x1,short y1,short x2,short y2): tl(x1,y1), br(x2,y2) {
		normalise();
	}
	vec2_t tl, br;
	inline short w() const { return br.x-tl.x; }
	inline short h() const { return br.y-tl.y; }
	inline vec2_t size() const { return vec2_t(w(),h()); }
	inline bool empty() const { return (br.x<=tl.x) || (br.y<=tl.y); }
	inline void normalise() { vec2_t::normalise(tl,br); }
};

inline vec2_t& vec2_t::operator+=(const vec2_t& a) {
	x += a.x;
	y += a.y;
	return *this;
}

inline vec2_t vec2_t::operator+(const vec2_t& a) const {
	vec2_t ret = *this;
	ret += a;
	return ret;
}

inline vec2_t& vec2_t::operator-=(const vec2_t& a) {
	x -= a.x;
	y -= a.y;
	return *this;
}

inline vec2_t vec2_t::operator-(const vec2_t& a) const {
	vec2_t ret = *this;
	ret -= a;
	return ret;
}

inline void vec2_t::normalise(vec2_t& tl,vec2_t& br) {
	if(br.x < tl.x) {
		const short tmp = br.x;
		br.x = tl.x;
		tl.x = tmp;
	}
	if(br.y < tl.y) {
		const short tmp = br.y;
		br.y = tl.y;
		tl.y = tmp;
	}
}

inline std::ostream& operator<<(std::ostream& out,const vec2_t& v) {
	return out << "vec2_t<"<<v.x<<','<<v.y<<'>';
}

inline std::ostream& operator<<(std::ostream& out,const rect_t& r) {
	return out << "rect_t<"<<r.tl<<','<<r.br<<'>';
}

#endif //__2D_HPP__

