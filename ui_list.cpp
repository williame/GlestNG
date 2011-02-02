/*
 ui_list.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "font.hpp"
#include "ui_list.hpp"

struct ui_list_t::pimpl_t {
	pimpl_t(style_t s,const strings_t& l):
		style(s),
		list(l),
		h(font_mgr()->measure(' ').y),
		mouse_grab(false), hover(0) {}
	style_t style;
	const strings_t list;
	const int h;
	bool mouse_grab;
	size_t hover;
};

ui_list_t::ui_list_t(style_t style,const strings_t& list,ui_component_t* parent):
	ui_component_t(parent),
	pimpl(new pimpl_t(style,list)) {}

ui_list_t::~ui_list_t() { delete pimpl; }

bool ui_list_t::offer(const SDL_Event& event) {
	return false;
}

enum { BG_COL, OUTLINE_COL, TEXT_COL, NUM_COLORS };

struct colour_t {
	uint8_t r,g,b,a;
	void set() const { glColor4ub(r,g,b,a); }
} static const COL[NUM_COLORS] = {
	{0x40,0x40,0x40,0xc0}, //BG_COL
	{0x00,0x00,0xff,0xff}, //OUTLINE_COL
	{0x00,0xff,0xff,0xff}, //TEXT_COL
}, HOVER_COL[NUM_COLORS] = {
	{0xa0,0xa0,0x40,0xc0}, //BG_COL
	{0xf0,0xf0,0x40,0xff}, //OUTLINE_COL
	{0x00,0x00,0xff,0xff}, //TEXT_COL
};

void ui_list_t::draw() {
	const rect_t r = get_rect();
	const int line_spacing = (pimpl->style==MENU? pimpl->h*2: pimpl->h);
	int y = r.tl.y;
	for(size_t i=0; i<pimpl->list.size(); i++) {
		if(y > r.br.y) break;
		const colour_t* col = (pimpl->hover == i)? HOVER_COL: COL;
		const std::string& s = pimpl->list[i];
		int x = r.tl.x;
		const int w = font_mgr()->measure(s).x;
		col[BG_COL].set();
		draw_filled_box(x,y,w,pimpl->h);
		col[OUTLINE_COL].set();
		draw_box(x,y,w,pimpl->h);
		col[TEXT_COL].set();
		font_mgr()->draw(x,y,s);
		y += line_spacing;
	}
}

