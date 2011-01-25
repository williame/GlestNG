/*
 font.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __FONT_HPP__
#define __FONT_HPP__

#include "2d.hpp"

class font_mgr_t {
public:	
	static font_mgr_t* get_font_mgr();
	virtual ~font_mgr_t() {}
	virtual vec2_t measure(char ch) = 0;
	virtual vec2_t measure(const char* msg) = 0;
	virtual vec2_t measure(const char* msg,int count) = 0;
	virtual int draw(int x,int y,char ch) = 0;
	virtual int draw(int x,int y,const char* msg) = 0;
private:
	class impl_t;
	friend class impl_t;
	font_mgr_t() {}
};

inline font_mgr_t* font_mgr() { return font_mgr_t::get_font_mgr(); } 

#endif //__FONT_HPP__

