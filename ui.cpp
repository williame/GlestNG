/*
 ui.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <vector>
#include <assert.h>

#include "ui.hpp"

ui_component_t::ui_component_t(): mgr(*ui_mgr()), r(0,0,0,0), visible(false) {
	mgr.register_component(this);
}

ui_component_t::~ui_component_t() {
	mgr.deregister_component(this);
}

void ui_component_t::set_rect(const rect_t& r_) {
	if(visible)
		mgr.invalidate(r);
	r = r_;
	if(visible)
		mgr.invalidate(r);
}

void ui_component_t::set_visible(bool v) {
	if(v == visible) return;
	visible = v;
	mgr.invalidate(r);
}

class ui_mgr_t::impl_t: public ui_mgr_t {
public:
	void invalidate(const rect_t& r);
private:
	void register_component(ui_component_t* comp);
	void deregister_component(ui_component_t* comp);
	typedef std::vector<ui_component_t*> components_t;
	components_t components;
};

void ui_mgr_t::impl_t::invalidate(const rect_t& r) {
	if(r.empty()) return;
}

void ui_mgr_t::impl_t::register_component(ui_component_t* comp) {
#ifndef NDEBUG
	for(components_t::const_iterator i=components.begin(); i!=components.end(); i++)
		assert(*i != comp);
#endif
	components.push_back(comp);
}

void ui_mgr_t::impl_t::deregister_component(ui_component_t* comp) {
	//TODO###
}

ui_mgr_t* ui_mgr_t::get_ui_mgr() {
	static ui_mgr_t* singleton = 0;
	if(!singleton)
		singleton = new ui_mgr_t::impl_t();
	return singleton;
}

