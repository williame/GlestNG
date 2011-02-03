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
#include "error.hpp"

unsigned now(); // in world.cpp

class cancel_button_t: public ui_component_t {
public:
	struct handler_t {
		virtual void on_cancel(cancel_button_t* btn) = 0;
	};
	cancel_button_t(handler_t& h,ui_component_t* parent): 
		ui_component_t(parent), handler(h), radius_sqrd(0) {}
private:
	void reshaped() {
		const rect_t r(get_rect());
		radius_sqrd = std::min(r.w(),r.h())/2;
		radius_sqrd *= radius_sqrd;
	}
	bool offer(const SDL_Event& event) {
		if((event.type == SDL_KEYDOWN) &&
			(event.key.keysym.sym == SDLK_ESCAPE)) {
			handler.on_cancel(this);
			return true;
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			const rect_t r(get_rect());
			const vec2_t m(event.button.x,event.button.y);
			if(r.contains(m) && (radius_sqrd >= m.distance_sqrd(r.centre()))) {
				handler.on_cancel(this);
				return true;
			}
		}
		return false;
	}
	void draw() {
		glColor3ub(0x90,0,0);
		draw_circle(get_rect(),true);
	}
	handler_t& handler;
	int radius_sqrd;
};

struct fader_t {
	fader_t(float a_,unsigned d,bool o): a(a_), duration(d), end(0), on(o) {}
	const float a;
	const unsigned duration;
	unsigned end;
	bool on;
	void set(bool on);
	bool calc(float& a) const;
};

void fader_t::set(bool on) {
	if(on == this->on) return;
	this->on = on;
	if(end < now())
		end = now()+duration;
	else 
		end = now()+duration - (end-now());
}

bool fader_t::calc(float& alpha) const {
	if(end > now()) {
		float f = ((float)(end-now())/duration);
		if(on) f = 1.0 - f;
		f = (1.0-a) + (a*f);
		alpha *= f;
		return true;
	}
	alpha *= on? 1.0: a;
	return on;
}

struct ui_list_t::pimpl_t: public cancel_button_t::handler_t {
	static int line_height() {
		static int h = font_mgr()->measure(" ").y;
		return h;
	}
	pimpl_t(ui_list_t* ui_,unsigned f,const std::string& t,const strings_t& l):
		ui(ui_), handler(NULL),
		flags(f),title(t),list(l),
		h(line_height()),
		line_spacing(line_height()+6),
		margin(3,3),
		corner(line_height()/2,line_height()/2),
		enabled(0.5,2000,true), visible(1.0,2000,false), destroyed(false),
		selected(NO_SELECTION),
		cancel(NULL) {
		w = font_mgr()->measure(title).x;
		for(size_t i=0; i<list.size(); i++)
			w = std::max<int>(w,font_mgr()->measure(list[i]).x);
		if(CANCEL_BUTTON&flags)
			cancel = new cancel_button_t(*this,ui);
		visible.set(true);
	}
	ui_list_t* ui;
	ui_list_t::handler_t* handler;
	unsigned flags;
	const std::string title;
	const strings_t list;
	const int h, line_spacing;
	const vec2_t margin, corner;
	int w;
	fader_t enabled, visible;
	bool destroyed;
	size_t selected;
	cancel_button_t* cancel;
	void on_cancel(cancel_button_t*) {
		if(handler)
			handler->on_cancelled(ui);
		else
			ui->hide();
	}
	static const size_t NO_SELECTION = ~(size_t)0;
};

ui_list_t::ui_list_t(unsigned flags,const std::string& title,const strings_t& list,ui_component_t* parent):
	ui_component_t(parent),
	pimpl(new pimpl_t(this,flags,title,list)) {}

ui_list_t::~ui_list_t() { delete pimpl; }

bool ui_list_t::offer(const SDL_Event& event) {
	if(!pimpl->visible.on) return false;
	if(offer_children(event)) return true;
	if(!pimpl->enabled.on) return false;
	if(event.type == SDL_MOUSEBUTTONDOWN) {
		rect_t r(get_rect().inner(pimpl->margin));
		vec2_t m(event.button.x,event.button.y);
		if(!r.contains(m)) return false;
		m -= r.tl;
		r.move(-r.tl);
		if(pimpl->title.size()) {
			if(m.y < pimpl->line_spacing) return true;
			m.y -= pimpl->line_spacing;
		}
		pimpl->selected = m.y / pimpl->line_spacing;
		if(pimpl->handler)
			pimpl->handler->on_selected(this,pimpl->selected,vec2_t(event.button.x,event.button.y));
		return true;
	}
	return false;
}

void ui_list_t::reshaped() {
	if(pimpl->cancel) {
		const rect_t r = get_rect();
		const vec2_t sz(pimpl->corner+pimpl->corner);
		const vec2_t pos(r.br.x-sz.x-pimpl->margin.x,r.tl.y+pimpl->margin.y);
		pimpl->cancel->set_rect(rect_t(pos,pos+sz));
	}
}

vec2_t ui_list_t::preferred_size() const {
	vec2_t sz(pimpl->w+pimpl->margin.x*6+pimpl->corner.x*2,
		pimpl->line_spacing*pimpl->list.size()+pimpl->margin.y);
	if(pimpl->title.size())
		sz.y += pimpl->line_spacing;
	return sz;
}

const strings_t& ui_list_t::get_list() const { return pimpl->list; }

bool ui_list_t::has_selection() const { return (pimpl->selected != pimpl_t::NO_SELECTION); }

size_t ui_list_t::get_selection() const {
	if(!has_selection()) panic(this << " has no selection");
	return pimpl->selected;
}

void ui_list_t::set_selection(size_t idx) {
	if(idx >= pimpl->list.size()) panic(this << " selection is out of bounds: "<<idx);
	pimpl->selected = idx;
}

void ui_list_t::clear_selection() { pimpl->selected = ~0; }

ui_list_t::handler_t* ui_list_t::set_handler(handler_t* handler) {
	handler_t* old = pimpl->handler;
	pimpl->handler = handler;
	return old;
}

enum { BG_COL, BORDER_COL, OUTLINE_COL, ITEM_BG_COL, TEXT_COL, NUM_COLORS };

struct colour_t {
	uint8_t r,g,b,a;
	void set(float alpha=1) const { glColor4ub(r,g,b,a*alpha); }
} static const COL[NUM_COLORS] = {
	{0xc0,0xc0,0x40,0xc0}, //BG_COL
	{0x50,0x50,0x00,0xff}, //BORDER_COL
	{0xd0,0xd0,0x60,0x80}, //OUTLINE_COL
	{0xc0,0xc0,0x40,0xc0}, //ITEM_BG_COL
	{0x00,0x80,0xa0,0xff}, //TEXT_COL
}, HOVER_COL[NUM_COLORS] = {
	{0xa0,0xa0,0x40,0xc0}, //BG_COL unused
	{0x00,0x00,0x00,0x00}, //BORDER_COL unused
	{0xf0,0xf0,0x40,0x00}, //OUTLINE_COL
	{0xf0,0xf0,0x40,0xc0}, //ITEM_BG_COL
	{0x00,0x40,0xa0,0xff}, //TEXT_COL
}, TITLE_COL = {0xff,0xff,0xff,0xff};

void ui_list_t::set_visible(bool visible) {
	if(visible) {
		if(is_visible())
			return;
		if(!ui_component_t::is_visible())
			ui_component_t::set_visible(true);
		pimpl->visible.set(true);
		if(is_enabled() && pimpl->cancel)
			pimpl->cancel->show();
	} else if(is_visible()) {
		pimpl->visible.set(false);
		if(pimpl->cancel)
			pimpl->cancel->hide();
	}
}

bool ui_list_t::is_visible() const {
	return pimpl->visible.on;
}

bool ui_list_t::is_enabled() const {
	return pimpl->enabled.on;
}

void ui_list_t::disable() {
	if(!is_enabled())
		return;
	pimpl->enabled.set(false);
	if(pimpl->cancel)
		pimpl->cancel->hide();
}

void ui_list_t::enable() {
	if(is_enabled())
		return;
	pimpl->enabled.set(true);
	if(!is_visible())
		return;
	if(pimpl->cancel)
		pimpl->cancel->show();
}

void ui_list_t::destroy() {
	pimpl->destroyed = true;
	if(is_visible())
		hide();
	else
		ui_component_t::destroy();
}

void ui_list_t::draw() {
	float alpha = 1.0;
	if(!pimpl->visible.calc(alpha)) {
		if(pimpl->destroyed)
			ui_component_t::destroy();
		else
			ui_component_t::set_visible(false);
		return;
	}
	pimpl->enabled.calc(alpha);
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
		const bool selected = (pimpl->selected == i);
		const colour_t* col = selected? HOVER_COL: COL;
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

