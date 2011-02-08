/*
 font.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __FONT_HPP__
#define __FONT_HPP__

#include <string>
#include <string.h>
#include <memory>

#include "2d.hpp"

class font_t {
public:
	virtual vec2_t measure(char ch) = 0;
	inline vec2_t measure(const char* msg) { return measure(msg,strlen(msg)); }
	inline vec2_t measure(const std::string& s) { return measure(s.c_str(),s.size()); }
	virtual vec2_t measure(const char* msg,int count) = 0;
	virtual int draw(int x,int y,char ch) = 0;
	virtual int draw(int x,int y,const char* msg,int count) = 0;
	inline int draw(int x,int y,const char* msg) { return draw(x,y,msg,strlen(msg)); }
	inline int draw(int x,int y,const std::string& s) { return draw(x,y,s.c_str(),s.size()); }
protected:
	font_t() {}
	virtual ~font_t() {}
};

class fonts_t {
public:	
	enum logical_t {
		// generic families
		SANS,
		// mapping logical names
		UI_TITLE = SANS,
		UI_TEXT = SANS,
	};
	font_t* get(logical_t face);
	static fonts_t* create();
	static fonts_t* fonts();
	virtual ~fonts_t();
private:
	struct pimpl_t;
	pimpl_t* pimpl;
	fonts_t();
};

inline fonts_t* fonts() { return fonts_t::fonts(); } 

#endif //__FONT_HPP__

