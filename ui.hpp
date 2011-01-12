/*
 ui.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UI_HPP__
#define __UI_HPP__

#include "2d.hpp"

class ui_mgr_t;

class ui_component_t {
public:
	inline const rect_t& rect() const { return r; } 
	void set_rect(const rect_t& r);
	void set_visible(bool visible);
	void show() { set_visible(true); }
	void hide() { set_visible(false); }
protected:
	ui_component_t();                 
	virtual ~ui_component_t();
	ui_mgr_t& mgr;
private:
	rect_t r;
	bool visible;
};

class ui_mgr_t {
public:
	static ui_mgr_t* get_ui_mgr();
	virtual ~ui_mgr_t() {}
	virtual void invalidate(const rect_t& r) = 0;
private:
	friend class ui_component_t;
	virtual void register_component(ui_component_t* comp) = 0;
	virtual void deregister_component(ui_component_t* comp) = 0;
	class impl_t;
	friend class impl_t;
	ui_mgr_t() {}
};

inline ui_mgr_t* ui_mgr() { return ui_mgr_t::get_ui_mgr(); }

#endif //__UI_HPP__

