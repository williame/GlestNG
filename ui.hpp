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

struct ui_fader_t {
	ui_fader_t(float a_,unsigned d,bool o): a(a_), duration(d), end(0), on(o) {}
	bool is_transitioning() const;
	const float a;
	const unsigned duration;
	unsigned end;
	bool on;
	void set(bool on);
	bool calc(float& a) const;
};

class ui_component_t {
public:
	enum { // FLAGS for use to adjust the behaviour of the base and subclasses
		FADE_VISIBLE =	0x00000001, // transition changes in visiblity?
		CANCEL_BUTTON =	0x00000100,
		DESTROYED = 	0x01000000,
	};
	inline const rect_t& rect() const { return r; } 
	rect_t get_rect() const { return r; }
	vec2_t get_size() const { return r.size(); }
	void set_rect(const rect_t& r);
	void set_pos(const vec2_t& pt);
	vec2_t get_pos() const { return r.tl; }
	void set_visible(bool visible);
	bool is_visible() const;
	bool is_drawable() const; // may be visible, or transitioning to be shown/hidden
	void show() { set_visible(true); }
	void hide() { set_visible(false); }
	void invalidate();
	virtual void destroy(); // triggers the (eventual) deletion of this; don't deference after calling!
	enum { BG_COL, BORDER_COL, OUTLINE_COL, TITLE_COL, ITEM_COL, ITEM_ACTIVE_COL, TEXT_COL, TEXT_ACTIVE_COL, NUM_COLOURS };
	struct colour_t {
		uint8_t r,g,b,a;
		bool operator==(const colour_t& o) const { return r==o.r && b==o.b && g==o.g && a==o.a; }
		void set(float alpha=1) const;
	} static const col[NUM_COLOURS];
protected:
	ui_component_t(unsigned flags,ui_component_t* parent = NULL);                 
	virtual ~ui_component_t();
	virtual void reshaped() {}
	virtual void visibility_changed(bool visible) {}
	ui_mgr_t& mgr;
	void draw_box(const rect_t& r) const;
	void draw_box(short x,short y,short w,short h) const;
	void draw_filled_box(const rect_t& r) const;
	void draw_filled_box(short x,short y,short w,short h) const;
	enum corner_t { INNER, OUTER, LINE };
	void draw_corner(const rect_t& r,bool left,bool top,corner_t type) const;
	void draw_corner(const rect_t& r,bool left,bool top,short line_width) const;
	void draw_cornered_box(const rect_t& r,const vec2_t& corner,short line_width) const;
	void draw_cornered_box(const rect_t& r,const vec2_t& corner) const;
	void draw_filled_cornered_box(const rect_t& r,const vec2_t& corner) const;
	void draw_circle(const rect_t& r,bool filled) const; 
	void draw_circle(const rect_t& r,short line_width) const;
	void draw_line(const vec2_t& a,const vec2_t& b) const;
	void draw_hline(const vec2_t& p,short l) const;
	void draw_vline(const vec2_t& p,short h) const;
	void draw_hline(const vec2_t& p,short l,short line_width) const;
	void draw_vline(const vec2_t& p,short h,short line_width) const;
	rect_t draw_border(float alpha,const rect_t& r,const std::string& title,const colour_t& fill = col[BG_COL]) const;
	rect_t calc_border(const rect_t& r,const std::string& title) const;	
	bool offer_children(const SDL_Event& event);
	rect_t clip() const;
	rect_t clip(const rect_t& c) const;
	float base_alpha() const;
	static int line_height();
	static const vec2_t& corner();
	static const vec2_t& margin();
	unsigned flags;
private:
	friend class ui_mgr_t;
	virtual void draw() = 0;
	void add_child(ui_component_t* child);
	void remove_child(ui_component_t* child);
	virtual 	bool offer(const SDL_Event& event) { return false; }
	rect_t r;
	ui_fader_t visible;
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

class ui_cancel_button_t: public ui_component_t {
public:
	struct handler_t {
		virtual void on_cancel(ui_cancel_button_t* btn) = 0;
	};
	ui_cancel_button_t(unsigned flags,handler_t& h,ui_component_t* parent);
private:
	void reshaped();
	bool offer(const SDL_Event& event);
	void draw();
	handler_t& handler;
	int radius_sqrd;
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

