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

ui_component_t::ui_component_t(ui_component_t* p):
	mgr(*ui_mgr()), r(0,0,0,0), visible(true),
	parent(p), first_child(NULL), next_peer(NULL) {
	if(parent)
		parent->add_child(this);
	else
		mgr.register_component(this);
}

ui_component_t::~ui_component_t() {
	if(parent)
		parent->remove_child(this);
	else
		mgr.deregister_component(this);
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
	delete this;
}

void ui_component_t::invalidate() {
	if(visible)
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

void ui_component_t::set_visible(bool v) {
	if(v == visible) return;
	visible = v;
	mgr.invalidate(r);
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

void ui_component_t::draw_corner(const rect_t& r,bool left,bool top,bool filled) const {
	const float cx = (left?r.br.x:r.tl.x),
		cy = (top?r.br.y:r.tl.y);
	const int quadrant = (!left&&!top? 0: left&&!top? 1: left&&top? 2: 3);
	glBegin(filled? GL_POLYGON: GL_LINE_STRIP);
	_draw_arc(std::min(r.h(),r.w()),cx,cy,quadrant,1,true); 
	if(filled)
		glVertex2f(cx,cy);
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
	draw_corner(rect_t(r.tl,r.tl+corner),true,true,filled);
	draw_corner(rect_t(r.br.x-corner.x,r.tl.y,r.br.x,r.tl.y+corner.y),false,true,filled);
	draw_corner(rect_t(r.br-corner,r.br),false,false,filled);
	draw_corner(rect_t(r.tl.x,r.br.y-corner.y,r.tl.x+corner.x,r.br.y),true,false,filled);
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

bool ui_component_t::offer_children(const SDL_Event& event) {
	for(ui_component_t* child = first_child; child; child = child->next_peer)
		if(child->visible && child->offer(event))
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
	ui_component_t(parent), r(0xff), g(0xff), b(0xff) {
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
	void draw(ui_component_t* comp);
};

ui_mgr_t::pimpl_t::pimpl_t() {
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT,vp);
	screen = rect_t(vp[0],vp[1],vp[0]+vp[2],vp[1]+vp[3]);
	std::cout << "screen: "<<screen<<std::endl;
}

void ui_mgr_t::pimpl_t::draw(ui_component_t* comp) {
	if(!comp->is_visible()) return;
	comp->clip();
	comp->draw();
	if(comp->first_child)
		draw(comp->first_child);
	if(comp->next_peer)
		draw(comp->next_peer);
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
	for(pimpl_t::components_t::iterator comp=pimpl->components.begin(); comp!=pimpl->components.end(); comp++)
		pimpl->draw(*comp);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisable(GL_SCISSOR_TEST);
}

rect_t ui_mgr_t::get_screen_bounds() const {
	return pimpl->screen;
}

void ui_mgr_t::invalidate(const rect_t& r) {
	if(r.empty()) return;
}

bool ui_mgr_t::offer(const SDL_Event& event) {
	for(pimpl_t::components_t::iterator i=pimpl->components.begin(); i!=pimpl->components.end(); i++)
		if((*i)->visible && (*i)->offer(event))
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

