/*
 ui.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <vector>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <math.h>

#include "ui.hpp"
#include "font.hpp"
#include "graphics.hpp"

unsigned now(); // world.hpp

bool ui_fader_t::is_transitioning() const {
	return (end > now());
}

void ui_fader_t::set(bool on) {
	if(on == this->on) return;
	this->on = on;
	if(is_transitioning())
		end = now()+duration - (end-now());
	else 
		end = now()+duration;
}

bool ui_fader_t::calc(float& alpha) const {
	if(is_transitioning()) {
		float f = ((float)(end-now())/duration);
		if(on) f = 1.0 - f;
		f = (1.0-a) + (a*f);
		alpha *= f;
		return true;
	}
	alpha *= on? 1.0: a;
	return on;
}

void ui_component_t::colour_t::set(float alpha) const { glColor4ub(r,g,b,a*alpha); }

const ui_component_t::colour_t ui_component_t::col[ui_component_t::NUM_COLOURS] = {
	{0xc0,0xc0,0x40,0xc0}, //BG_COL
	{0x50,0x50,0x00,0xff}, //BORDER_COL
	{0xd0,0xd0,0x60,0x80}, //OUTLINE_COL
	{0x00,0xff,0xff,0xff}, //TITLE_COL
	{0xc0,0xc0,0x40,0xc0}, //ITEM_COL
	{0xf0,0xf0,0x40,0xc0}, //ITEM_ACTIVE_COL
	{0x00,0x80,0xa0,0xff}, //TEXT_COL
	{0x00,0x40,0xa0,0xff}, //TEXT_ACTIVE_COL
};

ui_component_t::ui_component_t(unsigned f,ui_component_t* p):
	mgr(*ui_mgr()), flags(f), r(0,0,0,0),
	visible(1.0,f&FADE_VISIBLE?500:0,!(f&FADE_VISIBLE)),
	parent(p), first_child(NULL), next_peer(NULL) {
	if(parent)
		parent->add_child(this);
	else
		mgr.register_component(this);
	show();
}

ui_component_t::~ui_component_t() {
	if(parent)
		parent->remove_child(this);
	else
		mgr.deregister_component(this);
	for(ui_component_t* child = first_child; child; ) {
		ui_component_t* t = child;
		child = child->next_peer;
		delete t;
	}
		
}

void ui_component_t::add_child(ui_component_t* child) {
	if(first_child) {
		ui_component_t* peer = first_child;
		while(peer->next_peer)
			peer = peer->next_peer;
		peer->next_peer = child;
	} else
		first_child = child;
}
		
void ui_component_t::remove_child(ui_component_t* child) {
	if(child == first_child)
		first_child = child->next_peer;
	else { // defensively programmed
		ui_component_t* peer = first_child;
		while(peer && (peer->next_peer != child))
			peer = peer->next_peer;
		if(peer)
			peer->next_peer = child->next_peer;
	}
}

void ui_component_t::destroy() {
	flags |= DESTROYED;
	if(is_visible() && (FADE_VISIBLE&flags))
		hide();
	else
		delete this;
}

void ui_component_t::invalidate() {
	if(is_visible())
		mgr.invalidate(r);
}

void ui_component_t::set_rect(const rect_t& r_) {
	invalidate();
	r = r_;
	invalidate();
	reshaped();
}

void ui_component_t::set_pos(const vec2_t& pt) {
	invalidate();
	r.br = pt+r.size();
	r.tl = pt;
	invalidate();
	reshaped();
}

float ui_component_t::base_alpha() const {
	float alpha = 1.0;
	visible.calc(alpha);
	return alpha;
}

bool ui_component_t::is_visible() const {
	return visible.on;
}

bool ui_component_t::is_drawable() const {
	return visible.on || visible.is_transitioning();
}

void ui_component_t::set_visible(bool v) {
	if(v == visible.on) return;
	visible.set(v);
	mgr.invalidate(r);
	visibility_changed(v);
}

static void _draw_box(GLenum type,const rect_t& r) {
	glBegin(type);
		glVertex2i(r.tl.x,r.tl.y);
		glVertex2i(r.br.x,r.tl.y);
		glVertex2i(r.br.x,r.br.y);
		glVertex2i(r.tl.x,r.br.y);
	glEnd();
}

void ui_component_t::draw_box(const rect_t& r) const { _draw_box(GL_LINE_LOOP,r); }
void ui_component_t::draw_box(short x,short y,short w,short h) const { _draw_box(GL_LINE_LOOP,rect_t(x,y,x+w,y+h)); }
void ui_component_t::draw_filled_box(const rect_t& r) const { _draw_box(GL_QUADS,r); }
void ui_component_t::draw_filled_box(short x,short y,short w,short h) const { _draw_box(GL_QUADS,rect_t(x,y,x+w,y+h)); }

static void _draw_arc(float r,float cx,float cy,int quadrant,int quadrants,bool open) {
	const int num_segments = 4*4,
		start = quadrant * (num_segments/4),
		stop = (quadrant+quadrants) * (num_segments/4) + (open?1:0);
	const float theta = 2.0 * 3.1415926 / float(num_segments); 
	const float c = cos(theta);//precalculate the sine and cosine
	const float s = sin(theta);
	float x = r, y = 0;
	for(int i = 0; i < stop; i++) {
		if(i>=start)
			glVertex2f(x + cx, y + cy);//output vertex 
		//apply the rotation matrix
		const float t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
}

void ui_component_t::draw_corner(const rect_t& r,bool left,bool top,corner_t type) const {
	const float cx = (left?r.br.x:r.tl.x),
		cy = (top?r.br.y:r.tl.y);
	const int quadrant = (!left&&!top? 0: left&&!top? 1: left&&top? 2: 3);
	glBegin(LINE == type? GL_LINE_STRIP: GL_POLYGON);
	if(INNER == type) glVertex2f((left?r.tl.x:r.br.x),(top?r.tl.y:r.br.y));
	_draw_arc(std::min(r.h(),r.w()),cx,cy,quadrant,1,true); 
	if(OUTER == type) glVertex2f(cx,cy);
	glEnd();
}

void ui_component_t::draw_circle(const rect_t& r,bool filled) const {
	glBegin(filled? GL_POLYGON: GL_LINE_LOOP);
	_draw_arc(std::min(r.h(),r.w())/2,r.tl.x+r.w()/2,r.tl.y+r.h()/2,0,4,false);
	glEnd();
}

void ui_component_t::draw_line(const vec2_t& a,const vec2_t& b) const {
	glBegin(GL_LINES);
	glVertex2s(a.x,a.y);
	glVertex2s(b.x,b.y);
	glEnd();
}

void ui_component_t::draw_hline(const vec2_t& p,short l) const {
	draw_line(p,vec2_t(p.x+l,p.y));
}

void ui_component_t::draw_vline(const vec2_t& p,short h) const {
	draw_line(p,vec2_t(p.x,p.y+h));
}

void ui_component_t::draw_cornered_box(const rect_t& r,const vec2_t& corner,bool filled) const {
	draw_corner(rect_t(r.tl,r.tl+corner),true,true,filled? OUTER: LINE);
	draw_corner(rect_t(r.br.x-corner.x,r.tl.y,r.br.x,r.tl.y+corner.y),false,true,filled? OUTER: LINE);
	draw_corner(rect_t(r.br-corner,r.br),false,false,filled? OUTER: LINE);
	draw_corner(rect_t(r.tl.x,r.br.y-corner.y,r.tl.x+corner.x,r.br.y),true,false,filled? OUTER: LINE);
	if(filled) {
		draw_filled_box(rect_t(r.tl.x+corner.x,r.tl.y,r.br.x-corner.x,r.tl.y+corner.y));
		draw_filled_box(r.inner(vec2_t(0,corner.y)));
		draw_filled_box(rect_t(r.tl.x+corner.x,r.br.y-corner.y,r.br.x-corner.x,r.br.y));
	} else {
		draw_vline(vec2_t(r.tl.x,r.tl.y+corner.y),r.h()-corner.y*2);
		draw_vline(vec2_t(r.br.x,r.tl.y+corner.y),r.h()-corner.y*2);
		draw_hline(vec2_t(r.tl.x+corner.x,r.tl.y),r.w()-corner.x*2);
		draw_hline(vec2_t(r.tl.x+corner.x,r.br.y),r.w()-corner.x*2);
	}
}

rect_t ui_component_t::draw_border(float alpha,const rect_t& r,const std::string& title,const colour_t& fill) const {
	const vec2_t& margin = this->margin(), corner = this->corner();
	rect_t inner(r.inner(margin));
	const bool solid = fill == col[BG_COL];
	if(title.size()) {
		const int title_y = line_height()+margin.y*2;
		col[BG_COL].set(alpha);
		if(solid)
			draw_cornered_box(r,corner,true);
		else {
			draw_corner(rect_t(r.tl,r.tl+corner),true,true,OUTER);
			draw_corner(rect_t(r.br.x-corner.x,r.tl.y,r.br.x,r.tl.y+corner.y),false,true,OUTER);
			draw_filled_box(rect_t(r.tl.x+corner.x,r.tl.y,r.br.x-corner.x,r.tl.y+corner.y));
			draw_filled_box(rect_t(r.tl.x,r.tl.y+corner.y,r.br.x,r.tl.y+title_y));
		}
		col[TITLE_COL].set(alpha);
		font_mgr()->draw(inner.tl.x+corner.x,inner.tl.y,title);
		inner.tl.y = r.tl.y + title_y;
		col[BG_COL].set(alpha);
		if(!solid) {
			draw_corner(rect_t(inner.tl,inner.tl+corner),true,true,INNER);
			draw_corner(rect_t(inner.br.x-corner.x,inner.tl.y,inner.br.x,inner.tl.y+corner.y),false,true,INNER);
		}
	} else {
		col[BG_COL].set(alpha);
		draw_cornered_box(r,corner,solid);
	}
	col[OUTLINE_COL].set(alpha);
	draw_cornered_box(r,corner,false);
	if(!solid) {
		fill.set(alpha);
		draw_cornered_box(inner,corner,true);
	}
	return inner;
}

rect_t ui_component_t::calc_border(const rect_t& r,const std::string& title) const {
	rect_t inner = r.inner(margin());
	if(title.size())
		inner.move(0,line_height()+margin().y*2);
	return inner;
}

int ui_component_t::line_height() {
	static const int h = font_mgr()->measure(" ").y;
	return h;
}

const vec2_t& ui_component_t::corner() {
	static const vec2_t c(line_height()/2,line_height()/2);
	return c;
}

const vec2_t& ui_component_t::margin() {
	static const vec2_t m(3,3);
	return m;
}

bool ui_component_t::offer_children(const SDL_Event& event) {
	for(ui_component_t* child = first_child; child; child = child->next_peer)
		if(child->is_visible() && child->offer(event))
			return true;
	return false;
}

rect_t ui_component_t::clip() const {
	return clip(get_rect());
}

rect_t ui_component_t::clip(const rect_t& r) const {
	static rect_t old(0,0,0,0);
	rect_t o = old;
	old = r;
	glScissor(r.tl.x-1,mgr.get_screen_bounds().br.y-r.tl.y-r.h()-1,r.w()+2,r.h()+2); // flip Y
	return o;
}

ui_label_t::ui_label_t(const std::string& str,ui_component_t* parent):
	ui_component_t(FADE_VISIBLE,parent), r(0xff), g(0xff), b(0xff) {
	set_text(str);
}

void ui_label_t::set_text(const std::string& str) {
	s = str;
	sz = font_mgr()->measure(s.c_str());
	set_rect(rect_t(get_rect().tl,get_rect().tl+sz));
}

void ui_label_t::draw() {
	glColor3ub(r,g,b);
	font_mgr()->draw(get_rect().tl.x,get_rect().tl.y,s.c_str());
}

ui_cancel_button_t::ui_cancel_button_t(unsigned flags,handler_t& h,ui_component_t* parent): 
	ui_component_t(flags,parent), handler(h), radius_sqrd(0) {}

void ui_cancel_button_t::reshaped() {
	const rect_t r(get_rect());
	radius_sqrd = std::min(r.w(),r.h())/2;
	radius_sqrd *= radius_sqrd;
}

bool ui_cancel_button_t::offer(const SDL_Event& event) {
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

void ui_cancel_button_t::draw() {
	glColor4f(0.5,0,0,base_alpha());
	draw_circle(get_rect(),true);
}

struct ui_mgr_t::pimpl_t {
	pimpl_t();
	~pimpl_t() {
		components_t tmp = components;
		components.clear();
		for(components_t::iterator i=tmp.begin(); i!=tmp.end(); i++)
			delete *i;
	}
	typedef std::vector<ui_component_t*> components_t;
	components_t components;
	rect_t screen;
	void draw(ui_component_t* comp,components_t& destroyed);
};

ui_mgr_t::pimpl_t::pimpl_t() {
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT,vp);
	screen = rect_t(vp[0],vp[1],vp[0]+vp[2],vp[1]+vp[3]);
	std::cout << "screen: "<<screen<<std::endl;
}

void ui_mgr_t::pimpl_t::draw(ui_component_t* comp,components_t& destroyed) {
	if(comp->is_drawable()) { // not calling overriden function
		comp->clip();
		comp->draw();
		if(comp->first_child)
			draw(comp->first_child,destroyed);
	} else if(comp->flags & ui_component_t::DESTROYED)
		destroyed.push_back(comp);
	if(comp->next_peer)
		draw(comp->next_peer,destroyed);
}

ui_mgr_t::ui_mgr_t(): pimpl(new pimpl_t()) {}

ui_mgr_t::~ui_mgr_t() {
	delete pimpl;
}

void ui_mgr_t::draw() {
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0,pimpl->screen.w(),pimpl->screen.h(),0); // flip Y
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	pimpl_t::components_t destroyed;
	for(pimpl_t::components_t::iterator comp=pimpl->components.begin(); comp!=pimpl->components.end(); comp++)
		pimpl->draw(*comp,destroyed);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisable(GL_SCISSOR_TEST);
	for(pimpl_t::components_t::iterator i=destroyed.begin(); i!=destroyed.end(); i++)
		delete *i;
}

rect_t ui_mgr_t::get_screen_bounds() const {
	return pimpl->screen;
}

void ui_mgr_t::invalidate(const rect_t& r) {
	if(r.empty()) return;
}

bool ui_mgr_t::offer(const SDL_Event& event) {
	for(pimpl_t::components_t::iterator i=pimpl->components.begin(); i!=pimpl->components.end(); i++)
		if((*i)->is_visible() && (*i)->offer(event))
			return true;
	return false;
}

void ui_mgr_t::register_component(ui_component_t* comp) {
#ifndef NDEBUG
	for(pimpl_t::components_t::const_iterator i=pimpl->components.begin(); i!=pimpl->components.end(); i++)
		assert(*i != comp);
#endif
	pimpl->components.push_back(comp);
}

void ui_mgr_t::deregister_component(ui_component_t* comp) {
	for(pimpl_t::components_t::iterator i=pimpl->components.begin(); i!=pimpl->components.end(); i++)
		if(*i == comp) {
			pimpl->components.erase(i);
			return;
		}
}

ui_mgr_t* ui_mgr_t::get_ui_mgr() {
	static ui_mgr_t* singleton = NULL;
	if(!singleton)
		singleton = new ui_mgr_t();
	return singleton;
}

