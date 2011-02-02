/*
 ui.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UI_HPP__
#define __UI_HPP__

#include <string>
#include <inttypes.h>
#include <ostream>

#include "2d.hpp"
#include "graphics.hpp"

class ui_mgr_t;

class ui_component_t {
public:
	inline const rect_t& rect() const { return r; } 
	rect_t get_rect() const { return r; }
	vec2_t get_size() const { return r.size(); }
	void set_rect(const rect_t& r);
	void set_pos(const vec2_t& pt);
	vec2_t get_pos() const { return r.tl; }
	virtual void set_visible(bool visible);
	virtual bool is_visible() const { return visible; }
	bool is_shown() const;
	void show() { set_visible(true); }
	void hide() { set_visible(false); }
	void invalidate();
	virtual void destroy(); // triggers the (eventual) deletion of this; don't deference after calling!
protected:
	ui_component_t(ui_component_t* parent = NULL);                 
	virtual ~ui_component_t();
	virtual void reshaped() {}
	ui_mgr_t& mgr;
	void draw_box(const rect_t& r) const;
	void draw_box(short x,short y,short w,short h) const;
	void draw_filled_box(const rect_t& r) const;
	void draw_filled_box(short x,short y,short w,short h) const;
	void draw_corner(const rect_t& r,bool left,bool top,bool filled) const;
	void draw_cornered_box(const rect_t& r,const vec2_t& corner,bool filled) const;
	void draw_circle(const rect_t& r,bool filled) const; 
	void draw_line(const vec2_t& a,const vec2_t& b) const;
	void draw_hline(const vec2_t& p,short l) const;
	void draw_vline(const vec2_t& p,short h) const;
	bool offer_children(const SDL_Event& event);
	rect_t clip() const;
	rect_t clip(const rect_t& c) const;
private:
	friend class ui_mgr_t;
	virtual void draw() = 0;
	void add_child(ui_component_t* child);
	void remove_child(ui_component_t* child);
	virtual 	bool offer(const SDL_Event& event) { return false; }
	rect_t r;
	bool visible;
	ui_component_t *parent, *first_child, *next_peer;
};

class ui_label_t: public ui_component_t {
public:
	ui_label_t(const std::string& str,ui_component_t* parent = NULL);
	void set_color(uint8_t r_,uint8_t g_,uint8_t b_) { r = r_; g = g_; b = b_; }
	void set_text(const std::string& str);
	std::string get_text() const { return s; }
private:
	void draw();
	std::string s;
	vec2_t sz;
	uint8_t r,g,b;
};

class ui_mgr_t {
public:
	static ui_mgr_t* get_ui_mgr();
	virtual ~ui_mgr_t(); 
	void draw();
	rect_t get_screen_bounds() const;
	void invalidate(const rect_t& r);
	bool offer(const SDL_Event& event);
private:
	friend class ui_component_t;
	virtual void register_component(ui_component_t* comp);
	virtual void deregister_component(ui_component_t* comp);
	struct pimpl_t;
	pimpl_t* pimpl;
	ui_mgr_t();
};

inline std::ostream& operator<<(std::ostream& out,const ui_component_t& comp) {
	return out << "ui<" << comp.get_rect() << '>';
}

inline std::ostream& operator<<(std::ostream& out,const ui_component_t* comp) {
	return out << *comp;
}

inline ui_mgr_t* ui_mgr() { return ui_mgr_t::get_ui_mgr(); }

#endif //__UI_HPP__

