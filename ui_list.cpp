/*
 ui_list.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <algorithm>
#include <iostream>

#include "font.hpp"
#include "ui_list.hpp"

struct ui_list_t::pimpl_t {
	static int line_height() {
		static int h = font_mgr()->measure(" ").y;
		return h;
	}
	pimpl_t(style_t s,const std::string& t,const strings_t& l):
		style(s),
		title(t),
		list(l),
		h(line_height()),
		line_spacing(s==MENU? line_height()+6: line_height()+2),
		margin(s==MENU? 3: 1,3),
		corner(line_height()/2,line_height()/2),
		mouse_grab(false), hover(0) {
		w = font_mgr()->measure(title).x;
		for(size_t i=0; i<list.size(); i++)
			w = std::max<int>(w,font_mgr()->measure(list[i]).x);
	}
	style_t style;
	const std::string title;
	const strings_t list;
	const int h, line_spacing;
	const vec2_t margin, corner;
	int w;
	bool mouse_grab;
	size_t hover;
};

ui_list_t::ui_list_t(style_t style,const std::string& title,const strings_t& list,ui_component_t* parent):
	ui_component_t(parent),
	pimpl(new pimpl_t(style,title,list)) {}

ui_list_t::~ui_list_t() { delete pimpl; }

bool ui_list_t::offer(const SDL_Event& event) {
	return false;
}

vec2_t ui_list_t::preferred_size() const {
	vec2_t sz(pimpl->w+pimpl->margin.x*6+pimpl->corner.x*2,
		pimpl->line_spacing*pimpl->list.size()+pimpl->margin.y);
	if(pimpl->title.size())
		sz.y += pimpl->line_spacing;
	return sz;
}

enum { BG_COL, BORDER_COL, OUTLINE_COL, ITEM_BG_COL, TEXT_COL, NUM_COLORS };

struct colour_t {
	uint8_t r,g,b,a;
	void set(float alpha=1) const { glColor4ub(r,g,b,a*alpha); }
} static const COL[NUM_COLORS] = {
	{0xc0,0xc0,0x40,0xc0}, //BG_COL
	{0x70,0x70,0x00,0xff}, //BORDER_COL
	{0x00,0x00,0xff,0xff}, //OUTLINE_COL
	{0xc0,0xc0,0x40,0xc0}, //ITEM_BG_COL
	{0x00,0xff,0xff,0xff}, //TEXT_COL
}, HOVER_COL[NUM_COLORS] = {
	{0xa0,0xa0,0x40,0xc0}, //BG_COL unused
	{0x00,0x00,0x00,0x00}, //BORDER_COL unused
	{0xf0,0xf0,0x40,0x00}, //OUTLINE_COL
	{0xf0,0xf0,0x40,0xc0}, //ITEM_BG_COL
	{0x00,0xff,0xff,0xff}, //TEXT_COL
}, TITLE_COL = {0xff,0xff,0xff,0xff};

void ui_list_t::draw() {
	const float alpha = 0.5;
	const vec2_t& corner = pimpl->corner;
	const rect_t outer = get_rect();
	COL[BG_COL].set(alpha);
	draw_cornered_box(outer,corner,true);
	COL[OUTLINE_COL].set(alpha);
	draw_cornered_box(outer,corner,false);
	rect_t inner = outer.inner(pimpl->margin);
	if(pimpl->title.size()) {
		TITLE_COL.set(alpha);
		font_mgr()->draw(inner.tl.x+corner.x,inner.tl.y,pimpl->title);
		inner.move(0,pimpl->h+pimpl->margin.y*2);
	}
	clip(inner);
	int y = inner.tl.y;
	for(size_t i=0; i<pimpl->list.size(); i++) {
		if(y > inner.br.y) break;
		const bool hover = (pimpl->hover == i);
		const colour_t* col = hover? HOVER_COL: COL;
		const std::string& s = pimpl->list[i];
		const rect_t item(inner.tl.x,y,inner.br.x,y+pimpl->line_spacing-pimpl->margin.y);
		col[ITEM_BG_COL].set(alpha);
		draw_cornered_box(item,corner,true);
		col[OUTLINE_COL].set(alpha);
		draw_cornered_box(item,corner,false);
		col[TEXT_COL].set(alpha);
		font_mgr()->draw(item.tl.x+corner.x,item.tl.y+(item.h()-pimpl->h)/2,s);
		y += pimpl->line_spacing;
	}
}

