/*
 2d.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __2D_HPP__
#define __2D_HPP__

struct vec2_t {
	vec2_t() {}
	vec2_t(short x_,short y_): x(x_), y(y_) {}
	short x, y;
	inline static void normalise(vec2_t& tl,vec2_t& br);
};

struct rect_t {
	rect_t(short x1,short y1,short x2,short y2): tl(x1,y1), br(x2,y2) {
		normalise();
	}
	vec2_t tl, br;
	inline bool empty() const { return (br.x<=tl.x) || (br.y<=tl.y); }
	inline void normalise() { vec2_t::normalise(tl,br); }
};

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

#endif //__2D_HPP__

