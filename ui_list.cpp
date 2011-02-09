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

struct ui_list_t::pimpl_t: public ui_cancel_button_t::handler_t {
	pimpl_t(ui_list_t* ui_,const std::string& t,const strings_t& l):
		ui(ui_), handler(NULL),
		title(t),list(l),
		line_spacing(line_height()+margin().y*2),
		enabled(0.5,2000,true),
		selected(NO_SELECTION),
		cancel(NULL) {
		w = fonts()->get(fonts_t::UI_TITLE)->measure(title).x;
		for(size_t i=0; i<list.size(); i++)
			w = std::max<int>(w,fonts()->get(fonts_t::UI_TITLE)->measure(list[i]).x);
		if(CANCEL_BUTTON&ui->flags)
			cancel = new ui_cancel_button_t(FADE_VISIBLE&ui->flags,*this,ui);
	}
	ui_list_t* const ui;
	ui_list_t::handler_t* handler;
	unsigned flags;
	const std::string title;
	const strings_t list;
	const int line_spacing;
	int w;
	ui_fader_t enabled;
	size_t selected;
	ui_cancel_button_t* cancel;
	void on_cancel(ui_cancel_button_t*) {
		if(handler)
			handler->on_cancelled(ui);
		else {
			std::cerr << ui << ": no handler set, and we cancel" << std::endl;
			ui->hide();
		}
	}
	static const size_t NO_SELECTION = ~(size_t)0;
};

const unsigned ui_list_t::default_flags = FADE_VISIBLE|CANCEL_BUTTON;

ui_list_t::ui_list_t(unsigned flags,const std::string& title,const strings_t& list,ui_component_t* parent):
	ui_component_t(flags,parent),
	pimpl(new pimpl_t(this,title,list)) {}

ui_list_t::~ui_list_t() { delete pimpl; }

bool ui_list_t::offer(const SDL_Event& event) {
	if(offer_children(event)) return true;
	if(!pimpl->enabled.on) return false;
	if(event.type == SDL_MOUSEBUTTONDOWN) {
		rect_t r(calc_border(get_rect(),pimpl->title));
		vec2_t m(event.button.x,event.button.y);
		if(!r.contains(m)) return false;
		m -= r.tl;
		r.move(-r.tl);
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
		const vec2_t sz(corner()+corner());
		const vec2_t pos(r.br.x-sz.x-margin().x,r.tl.y+margin().y);
		pimpl->cancel->set_rect(rect_t(pos,pos+sz));
	}
}

vec2_t ui_list_t::preferred_size() const {
	vec2_t sz(pimpl->w+margin().x*6+corner().x*2,
		pimpl->line_spacing*pimpl->list.size()+margin().y);
	if(pimpl->title.size())
		sz.y += pimpl->line_spacing;
	return sz;
}

const strings_t& ui_list_t::get_list() const { return pimpl->list; }

const std::string& ui_list_t::get_title() const { return pimpl->title; }

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

void ui_list_t::visibility_changed(bool visible) {
	if(pimpl->cancel)
		pimpl->cancel->set_visible(is_enabled() && visible);
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

void ui_list_t::draw() {
	font_t* font = fonts()->get(fonts_t::UI_TITLE);
	float alpha = base_alpha();
	pimpl->enabled.calc(alpha);
	const rect_t inner = draw_border(alpha,get_rect(),pimpl->title);
	const int h = line_height();
	const vec2_t& corner = this->corner(), margin = this->margin();
	clip(inner);
	int y = inner.tl.y;
	for(size_t i=0; i<pimpl->list.size(); i++) {
		if(y > inner.br.y) break;
		const bool selected = (pimpl->selected == i);
		const std::string& s = pimpl->list[i];
		const rect_t item(inner.tl.x,y,inner.br.x,y+pimpl->line_spacing-margin.y);
		col[selected? ITEM_ACTIVE_COL: ITEM_COL].set(alpha);
		draw_filled_cornered_box(item,corner);
		col[OUTLINE_COL].set(alpha);
		draw_cornered_box(item,corner);
		col[selected? TEXT_ACTIVE_COL: TEXT_COL].set(alpha);
		font->draw(item.tl.x+corner.x,item.tl.y+(item.h()-h)/2,s);
		y += pimpl->line_spacing;
	}
}

