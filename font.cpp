/*
 font.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

//http://www.tdb.fi/ttf2png.shtml

#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>

#include "graphics.hpp"
#include "font.hpp"
#include "3d.hpp"

// 12th C. Fancy Caps
struct {
	int height, base;
} const font_meta = {24,19};

struct font_glyph_t {
	int code;
	short x,y,w,h,ofs_x,ofs_y,advance;
} const font_glyph[] = {
// convention is that [0] is -1 for unknown glyphs, others are in order
{-1,200,95,12,16,-1,4,10},{32,252,95,3,3,-1,18,5},{33,109,132,7,15,0,5,6},
{34,143,159,8,7,1,3,7},{35,9,133,13,15,-1,5,11},{36,88,133,12,15,-2,6,9},
{37,18,82,17,16,0,4,16},{38,210,61,18,16,-1,4,15},{39,159,156,5,7,1,3,4},
{40,117,0,10,21,0,3,7},{41,128,0,10,21,-2,3,7},{42,99,161,10,9,1,4,9},
{43,120,147,12,11,-1,9,11},{44,152,158,6,7,-2,15,5},{45,0,175,9,3,-1,13,7},
{46,221,154,5,4,-1,16,5},{47,245,21,10,18,-1,4,8},{48,159,144,12,11,-1,9,11},
{49,232,142,9,11,-2,9,6},{50,133,147,12,11,-2,9,9},{51,62,133,12,15,-2,9,9},
{52,143,115,12,16,-1,8,10},{53,49,133,12,15,-2,9,9},{54,161,96,12,16,0,4,11},
{55,174,96,12,16,-1,9,10},{56,147,96,13,16,-1,4,11},{57,36,133,12,15,-1,9,11},
{58,10,163,7,11,-1,9,5},{59,169,130,7,13,-1,9,5},{60,172,144,12,11,-1,9,11},
{61,171,156,12,6,-1,12,11},{62,94,149,12,11,-1,9,11},{63,214,112,9,16,1,4,9},
{64,166,42,18,18,-1,6,17},{65,0,82,17,16,-3,4,13},{66,38,64,14,17,-1,3,12},
{67,0,99,15,16,0,4,13},{68,20,64,17,17,-1,3,15},{69,16,99,14,16,-1,4,11},
{70,241,78,14,16,-1,4,11},{71,193,78,15,16,0,4,14},{72,190,61,19,16,-1,4,16},
{73,203,112,10,16,-1,4,7},{74,163,21,13,20,-4,4,6},{75,36,82,17,16,-1,4,13},
{76,133,98,13,16,-1,4,11},{77,167,61,22,16,-2,4,18},{78,0,64,19,17,-2,4,15},
{79,143,79,16,16,0,4,16},{80,76,99,14,16,-1,4,11},{81,85,22,16,20,0,4,16},
{82,31,99,14,16,-1,4,12},{83,119,98,13,16,-1,4,10},{84,177,78,15,16,0,4,12},
{85,126,81,16,16,1,4,15},{86,238,42,17,17,0,4,13},{87,142,43,23,18,0,3,20},
{88,229,61,18,16,-2,4,13},{89,209,78,15,16,0,4,12},{90,225,78,15,16,-2,4,11},
{91,189,21,11,20,-1,3,7},{92,230,42,7,18,1,4,8},{93,223,21,10,20,-2,3,7},
{94,31,162,11,10,0,4,11},{95,233,154,13,3,-2,20,10},{96,192,155,5,6,2,4,8},
{97,146,146,12,11,-1,9,10},{98,122,63,12,17,-1,3,10},{99,209,142,11,11,-1,9,9},
{100,53,64,13,17,-1,3,10},{101,185,143,11,11,-1,9,9},{102,18,0,16,22,-5,3,6},
{103,91,98,13,16,-2,9,10},{104,217,42,12,18,-1,3,11},{105,101,132,7,15,0,5,5},
{106,212,21,10,20,-3,5,5},{107,203,42,13,18,-1,3,10},{108,135,63,7,17,0,3,5},
{109,177,130,18,12,-1,9,17},{110,224,129,13,12,-1,9,11},{111,81,149,12,11,-1,9,10},
{112,61,99,14,16,-3,9,10},{113,187,95,12,16,-1,9,10},{114,38,149,10,12,-1,9,8},
{115,221,142,10,11,-1,9,8},{116,129,132,9,14,0,6,7},{117,238,129,12,12,0,8,11},
{118,26,149,11,12,-1,9,9},{119,139,132,16,13,-1,8,14},{120,67,149,13,11,-2,9,10},
{121,95,63,13,17,-2,8,10},{122,107,148,12,11,-1,9,10},{123,150,0,9,21,0,3,7},
{124,50,0,4,22,2,3,7},{125,139,0,10,21,-2,3,7},{126,198,154,12,4,-1,12,11},
{160,252,99,3,3,-1,18,5},{161,234,112,8,16,-2,9,5},{162,117,132,11,14,-1,7,9},
{163,210,129,13,12,-2,8,10},{164,196,129,13,12,-1,6,11},{165,0,150,12,12,0,8,11},
{166,251,0,4,18,2,3,7},{167,102,43,13,19,-1,4,11},{168,211,154,9,4,1,4,8},
{169,160,79,16,16,0,4,16},{170,0,163,9,11,-1,6,6},{171,86,161,12,9,-1,10,10},
{172,130,159,12,7,-1,13,11},{173,20,173,9,3,-1,13,7},{174,110,160,10,9,0,4,8},
{175,10,175,9,3,1,5,8},{176,121,159,8,8,0,4,7},{177,13,149,12,12,-1,8,11},
{178,43,162,10,10,-1,4,7},{179,54,161,9,10,0,4,7},{180,184,156,7,6,3,4,8},
{181,81,63,13,17,-2,8,11},{182,116,43,13,19,0,4,11},{183,227,154,5,4,0,12,5},
{184,165,156,5,7,0,18,8},{185,64,161,8,10,0,4,7},{186,242,142,9,11,-1,6,6},
{187,73,161,12,9,-1,10,10},{188,90,81,17,16,0,4,16},{189,72,82,17,16,0,4,16},
{190,54,82,17,16,0,4,16},{191,192,112,10,16,-2,9,9},{192,180,0,17,20,-3,0,13},
{193,216,0,17,20,-3,0,13},{194,198,0,17,20,-3,0,13},{195,0,44,17,19,-3,1,13},
{196,18,44,17,19,-3,1,13},{197,55,0,17,21,-3,-1,13},{198,143,62,23,16,-3,4,18},
{199,73,0,15,21,0,4,13},{200,118,22,14,20,-1,0,11},{201,133,22,14,20,-1,0,11},
{202,148,22,14,20,-1,0,11},{203,87,43,14,19,-1,1,11},{204,201,21,10,20,-1,0,7},
{205,234,21,10,20,-1,0,7},{206,177,21,11,20,-1,0,7},{207,130,43,11,19,-1,1,7},
{208,108,81,17,16,-1,4,15},{209,160,0,19,20,-2,1,15},{210,234,0,16,20,0,0,16},
{211,51,23,16,20,0,0,16},{212,68,22,16,20,0,0,16},{213,70,43,16,19,0,1,16},
{214,36,44,16,19,0,1,16},{215,197,142,11,11,0,9,11},{216,185,42,17,18,0,3,16},
{217,34,23,16,20,1,0,15},{218,17,23,16,20,1,0,15},{219,0,23,16,20,1,0,15},
{220,53,44,16,19,1,1,15},{221,102,22,15,20,0,0,12},{222,46,99,14,16,-1,4,12},
{223,0,0,17,22,-5,3,12},{224,213,95,12,16,-1,4,10},{225,226,95,12,16,-1,4,10},
{226,239,95,12,16,-1,4,10},{227,23,133,12,15,-1,5,10},{228,0,116,12,16,-1,4,10},
{229,109,63,12,17,-1,3,10},{230,49,149,17,11,-1,9,15},{231,156,113,11,16,-1,9,9},
{232,168,113,11,16,-1,4,9},{233,13,116,12,16,-1,4,9},{234,180,113,11,16,-1,4,9},
{235,26,116,12,16,-1,4,9},{236,248,60,6,16,0,4,5},{237,0,133,8,16,0,4,5},
{238,243,112,8,16,-1,4,5},{239,224,112,9,16,-1,4,5},{240,67,64,13,17,-1,3,11},
{241,105,98,13,16,-1,5,11},{242,39,116,12,16,-1,4,10},{243,52,116,12,16,-1,4,10},
{244,65,116,12,16,-1,4,10},{245,75,133,12,15,-1,5,10},{246,78,116,12,16,-1,4,10},
{247,18,162,12,10,-1,10,11},{248,156,130,12,13,-1,8,10},{249,91,115,12,16,0,4,11},
{250,104,115,12,16,0,4,11},{251,117,115,12,16,0,4,11},{252,130,115,12,16,0,4,11},
{253,89,0,13,21,-2,4,10},{254,35,0,14,22,-3,3,10},{255,103,0,13,21,-2,4,10}};

static const size_t num_glyphs = (sizeof(font_glyph)/sizeof(font_glyph_t));
	
class font_mgr_t::impl_t: public font_mgr_t {
public:
	impl_t();
	vec2_t measure(const char* msg);
	int draw(int x,int y,const char* msg);
private:
	const font_glyph_t* get_glyph(int code) const;
	GLuint texture;
	vec2_t bmp_sz;
};

font_mgr_t::impl_t::impl_t() {
	SDL_Surface* bmp = SDL_LoadBMP("font.bmp");
	if(!bmp) {
		fprintf(stderr,"SDL_Load: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	texture = graphics_mgr()->alloc_texture();
	graphics_mgr()->load_texture_2D(texture,bmp);
	bmp_sz = vec2_t(bmp->w,bmp->h);
	printf("loaded embedded bitmap font: %dx%d\n",bmp->w,bmp->h);
	SDL_FreeSurface(bmp);
}

vec2_t font_mgr_t::impl_t::measure(const char* msg) {
	vec2_t sz(0,font_meta.height);
	for(; *msg; msg++) {
		const font_glyph_t* g = get_glyph(*msg);
		if(!g) continue;
		sz.x += g->advance;
	}
	return sz;
}

int font_mgr_t::impl_t::draw(int x,int y,const char* msg) {
	const int start = x;
#if 0
	// show guide
	glBindTexture(GL_TEXTURE_2D,0);
	glBegin(GL_LINES);
	const vec2_t sz = measure(msg);
	glVertex2f(x,y);
	glVertex2f(x+sz.x,y);
	glVertex2f(x+sz.x,y);
	glVertex2f(x+sz.x,y+sz.y);
	glEnd();
#endif
	glBindTexture(GL_TEXTURE_2D,texture);
	glBegin(GL_QUADS);
	for(; *msg; msg++) {
		const font_glyph_t* g = get_glyph(*msg);
		if(!g) continue;
		const float
			tx0 = (float)g->x/bmp_sz.x,
			ty0 = (float)g->y/bmp_sz.y,
			tx1 = (float)(g->x+g->w)/bmp_sz.x,
			ty1 = (float)(g->y+g->h)/bmp_sz.y;
		const float
			x0 = x+g->ofs_x,
			y0 = y+g->ofs_y, //font_meta.height-(g->h-g->ofs_y),
			x1 = x0+g->w,
			y1 = y0+g->h;
		glTexCoord2f(tx0,ty0);
		glVertex2f(x0,y0);
		glTexCoord2f(tx1,ty0);
		glVertex2f(x1,y0);
		glTexCoord2f(tx1,ty1);
		glVertex2f(x1,y1);
		glTexCoord2f(tx0,ty1);
		glVertex2f(x0,y1);
		x += g->advance;
	}
	glEnd();
	glBindTexture(GL_TEXTURE_2D,0);
	return x-start;
}

static int _cmp_int(const void *a, const void *b) {
	return *(int*)a - *(int*)b;
}

const font_glyph_t* font_mgr_t::impl_t::get_glyph(int code) const {
	const font_glyph_t* ret = (font_glyph_t*)
		bsearch(&code,
		font_glyph,num_glyphs,sizeof(font_glyph_t),
		_cmp_int);
	if(!ret)
		return font_glyph; //[0] is reserved for unknown glyths
	return ret;
}
	
font_mgr_t* font_mgr_t::get_font_mgr() {
	static font_mgr_t* singleton = NULL;
	if(!singleton)
		singleton = new font_mgr_t::impl_t();
	return singleton;
}

