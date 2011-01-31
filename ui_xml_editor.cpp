/*
 ui_xml_editor.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

/* this implementation of an xml editor is not designed for speed;
   its amazing how back on the 286 I used to care so deeply about
   efficiency; now you can just put the file into strings that you
   constantly reallocate and redraw unnecessarily, and it doesn't
   matter */

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

#include "font.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "xml.hpp"
#include "ui_xml_editor.hpp"
#include "world.hpp"

namespace {
	struct line_t {
		size_t ofs;
		std::vector<xml_parser_t::type_t> type;
		std::string s;
	};
	typedef std::vector<line_t> lines_t;
	enum {
		TAB_SIZE = 3,
	};
}

class ui_xml_editor_t::pimpl_t {
public:
	pimpl_t(ui_xml_editor_t& ui_,xml_loadable_t& target_):
		em(font_mgr()->measure(' ').x),
		h(font_mgr()->measure(' ').y), 
		view_ofs(0,0), mouse_grab(false),
		ui(ui_), target(target_),
		dirty(true) {
		cursor.row = cursor.col = 0;
	}
	const std::string& get_title() const { return target.name; }
	const lines_t& get_lines() { parse(); return lines; }
	struct cursor_t {
		size_t row;
		size_t col;
	};
	cursor_t get_cursor() const { return cursor; }
	int char_from_ofs(int x,int row);
	int char_to_ofs(int col,int row);
	char char_at(const cursor_t& cursor);
	void nav_left();
	void nav_right();
	void nav_up();
	void nav_down();
	void nav_pgdn();
	void nav_pgup();
	void nav_home();
	void nav_end();
	void insert(char ch);
	void bksp();
	void del();
	void button_up(const vec2_t& pt);
	const int em; // width of space
	const int h; // line height
	vec2_t view_ofs;
	bool mouse_grab;
private:
	ui_xml_editor_t& ui;
	xml_loadable_t& target;
	void _append(const char ch,line_t& line,int i,xml_parser_t::type_t type);
	void parse();
	void reload(const std::string& buf);
	lines_t lines;
	bool dirty;
	cursor_t cursor;
};

void ui_xml_editor_t::pimpl_t::button_up(const vec2_t& pt) {
	assert(pt.y >= 0);
	assert(pt.x >= 0);
	cursor.row = pt.y / h;
	cursor.row = std::min(cursor.row,lines.size()-1);
	cursor.col = char_from_ofs(pt.x,cursor.row);
}

void ui_xml_editor_t::pimpl_t::nav_left() {
	if(cursor.col)
		cursor.col--;
	else if(cursor.row) {
		cursor.row--;
		if(cursor.col)
			cursor.col = lines[cursor.row].s.size();
	}
}

void ui_xml_editor_t::pimpl_t::nav_right() {
	cursor.col++;
}

void ui_xml_editor_t::pimpl_t::nav_up() {
	if(cursor.row) {
		const int ofs = char_to_ofs(cursor.col,cursor.row);
		cursor.row--;
		if(cursor.col)
			cursor.col = std::min<int>(char_from_ofs(ofs,cursor.row),lines[cursor.row].s.size());
	}
}

void ui_xml_editor_t::pimpl_t::nav_down() {
	if(cursor.row < lines.size()-1) {
		const int ofs = char_to_ofs(cursor.col,cursor.row);
		cursor.row++;
		if(cursor.col)
			cursor.col = std::min<int>(char_from_ofs(ofs,cursor.row),lines[cursor.row].s.size());
	}
}

void ui_xml_editor_t::pimpl_t::nav_pgdn() {
	const short screenful = ui.get_rect().h()/h;
	cursor.row = ((cursor.row / screenful)+1)*screenful;
	if(cursor.row >= lines.size())
		cursor.row = lines.size()-1;
}

void ui_xml_editor_t::pimpl_t::nav_pgup() {
	const size_t screenful = ui.get_rect().h()/h;
	if(cursor.row <= screenful)
		cursor.row = 0;
	else if(cursor.row % screenful)
		cursor.row = (cursor.row/screenful)*screenful;
	else
		cursor.row -= screenful;
}

void ui_xml_editor_t::pimpl_t::nav_home() { cursor.col = 0; }

void ui_xml_editor_t::pimpl_t::nav_end() { cursor.col = lines[cursor.row].s.size(); }

void ui_xml_editor_t::pimpl_t::insert(char ch) {
	size_t len = 0;
	for(size_t i=0; i<lines.size(); i++)
		len += lines[i].s.size() + 1;
	std::string buf;
	buf.reserve(len+1);
	for(size_t i=0; i<lines.size(); i++) {
		if(buf.size()) buf += '\n';
		if(i == cursor.row) {
			const line_t& line = lines[cursor.row];
			for(size_t j=0; j<cursor.col; j++)
				if(j >= line.s.size())
					buf += ' ';
				else
					buf += line.s[j];
			buf += ch;
			for(size_t j=cursor.col; j<line.s.size(); j++)
				buf += line.s[j];
		} else
			buf.append(lines[i].s);
	}		
	reload(buf);
	if(ch == '\n') {
		cursor.row++;
		cursor.col = 0;
	} else
		cursor.col++;
}

void ui_xml_editor_t::pimpl_t::bksp() {
	if(!cursor.row && !cursor.col) return;
	size_t len = 0;
	for(size_t i=0; i<lines.size(); i++)
		len += lines[i].s.size() + 1;
	std::string buf;
	buf.reserve(len+1);
	int lastline = 0, delline;
	for(size_t i=0; i<lines.size(); i++) {
		if(buf.size()) buf += '\n';
		if(i == cursor.row) {
			const line_t& line = lines[cursor.row];
			if(cursor.col) {
				for(size_t j=0; j<cursor.col-1; j++)
					if(j >= line.s.size())
						buf += ' ';
					else
						buf += line.s[j];
			} else {
				delline = lastline;
				buf.resize(buf.size()-1);
			}
			for(size_t j=cursor.col; j<line.s.size(); j++)
				buf += line.s[j];
		} else
			buf.append(lines[i].s);
		lastline = lines[i].s.size();
	}		
	reload(buf);
	if(cursor.col)
		cursor.col--;
	else {
		cursor.row--;
		cursor.col = delline;
	}
}

int ui_xml_editor_t::pimpl_t::char_from_ofs(int x,int row) {
	if(row >= (int)lines.size()) return -1;
	const line_t& line = lines[row];
	int i = 0, ofs = 0;
	while(ofs<=x) {
		if(i<(int)line.s.size())
			ofs += font_mgr()->measure(line.s[i]).x;
		else
			ofs += em;
		i++;
	}
	return i-1;
}

int ui_xml_editor_t::pimpl_t::char_to_ofs(int col,int row) {
	if((size_t)row >= lines.size()) return -1;
	const line_t& line = lines[row];
	if((size_t)col >= line.s.size())
		return font_mgr()->measure(line.s.c_str()).x+(em*(col-line.s.size()));
	return font_mgr()->measure(line.s.c_str(),col).x;
}

void ui_xml_editor_t::pimpl_t::_append(const char ch,line_t& line,int i,xml_parser_t::type_t type) {
	if(ch == '\n') {
		line.s.erase(line.s.find_last_not_of(" \n\r\t")+1);
		lines.push_back(line);
		line = line_t();
		line.ofs = i;
	} else if(ch == '\t') {
		for(size_t j=0; j<TAB_SIZE; j++) {
			line.s += ' ';
			line.type.push_back(type);
		}
	} else if(ch != '\r') {
		line.s += ch;
		line.type.push_back(type);
	}
}

char ui_xml_editor_t::pimpl_t::char_at(const cursor_t& cursor) {
	const line_t& line = get_lines()[cursor.row];
	if(cursor.col >= line.s.size()) return ' ';
	return line.s[cursor.col];
}

void ui_xml_editor_t::pimpl_t::parse() {
	if(!dirty) return;
	lines.clear();
	size_t i = 0;
	line_t line;
	const char* ch = target.get_xml()->get_buf();
	for(xml_parser_t::walker_t node = target.get_xml()->walker(); node.ok(); node.next()) {
		for(; i<node.ofs(); i++)
			_append(*ch++,line,i,xml_parser_t::IGNORE);
		for(; i<(node.ofs()+node.len()); i++)
			_append(*ch++,line,i,node.type());
	}
	for(; *ch; i++)
		_append(*ch++,line,i,xml_parser_t::IGNORE);
	line.s.erase(line.s.find_last_not_of(" \n\r\t")+1);
	if(line.s.size())
		lines.push_back(line);
	dirty = false;
}

void ui_xml_editor_t::pimpl_t::reload(const std::string& buf) {
	if(!target.load_xml(buf))
		std::cerr << target.name << " did not like XML" << std::endl;
	dirty = true;
	parse();
}

ui_xml_editor_t::ui_xml_editor_t(xml_loadable_t& target,ui_component_t* parent):
ui_component_t(parent), pimpl(new pimpl_t(*this,target)) {
	set_rect(rect_t(20,50,500,mgr.get_screen_bounds().br.y-30));
}

ui_xml_editor_t::~ui_xml_editor_t() {
	delete pimpl;
}

bool ui_xml_editor_t::offer(const SDL_Event& event) {
	if(offer_children(event))
		return true;
	switch(event.type) {
	case SDL_KEYUP:
		return true; // ignore them but eat them
	case SDL_MOUSEMOTION:
		return pimpl->mouse_grab;
	case SDL_MOUSEBUTTONDOWN:
		pimpl->mouse_grab = get_rect().contains(vec2_t(event.button.x,event.button.y));
		return pimpl->mouse_grab;
	case SDL_MOUSEBUTTONUP:
		if(pimpl->mouse_grab) {
			if(get_rect().contains(vec2_t(event.button.x,event.button.y))) {
				const int x = event.button.x - get_rect().tl.x + pimpl->view_ofs.x;
				const int y = event.button.y - get_rect().tl.y + pimpl->view_ofs.y - pimpl->h;
				if(y >= 0) // not on title
					pimpl->button_up(vec2_t(x,y));
			}
			return true;
		}
		return false;
	case SDL_KEYDOWN: {
		switch(event.key.keysym.sym) {
		case SDLK_ESCAPE: return false;
		case SDLK_LEFT: pimpl->nav_left(); break;
		case SDLK_RIGHT: pimpl->nav_right(); break;
		case SDLK_UP: pimpl->nav_up(); break;
		case SDLK_DOWN: pimpl->nav_down(); break;
		case SDLK_PAGEUP: pimpl->nav_pgup(); break;
		case SDLK_PAGEDOWN: pimpl->nav_pgdn(); break;
		case SDLK_HOME: pimpl->nav_home(); break;
		case SDLK_END: pimpl->nav_end(); break;
		case SDLK_RETURN: pimpl->insert('\n'); break;
		case SDLK_BACKSPACE: pimpl->bksp(); break;	
		default: 
			if(!(event.key.keysym.unicode&0xFF80)) {
				const char ch = event.key.keysym.unicode & 0x7F;
				if(!ch)
					std::cerr << "is unicode not enabled?" << std::endl;
				else
					pimpl->insert(ch);
			} else
				std::cout << "ignoring non-ascii "<<event.key.keysym.unicode<<std::endl;
		}
		return true; // keys don't escape!
	} break;
	default: return false;
	}
}

enum { BG_COL, BORDER_COL, TITLE_COL, TITLE_BG_COL, CURSOR_COL, NUM_COLORS };

struct color_t {
	uint8_t r,g,b,a;
	void set() const { glColor4ub(r,g,b,a); }
} static const COL[NUM_COLORS] = {
	{0x40,0x40,0x40,0xc0}, //BG_COL
	{0x00,0x00,0xff,0xff}, //BORDER_COL
	{0xff,0xff,0xff,0xff}, //TITLE_COL
	{0x20,0x20,0x20,0xc0}, //TITLE_BG_COL
	{0x80,0x80,0x00,0xc0}, //CURSOR_COL
}, TEXT_COL[xml_parser_t::NUM_TYPES] = {
	{0x80,0x80,0x80,0xff}, //IGNORE
	{0x00,0x80,0xff,0xff}, //OPEN
	{0x00,0x40,0x80,0xff}, //CLOSE
	{0x40,0xa0,0xa0,0xff}, //KEY
	{0x00,0x20,0xff,0xff}, //VALUE
	{0xa0,0xa0,0xa0,0xff}, //DATA
	{0xff,0x00,0x00,0xff}, //ERROR
};

void ui_xml_editor_t::draw() {
	// title and background
	rect_t r = get_rect();
	font_mgr_t& f = *font_mgr();
	const int h = pimpl->h;
	COL[TITLE_BG_COL].set();
	draw_filled_box(r.tl.x,r.tl.y,r.w(),h+2);
	COL[TITLE_COL].set();
	f.draw(r.tl.x+10,r.tl.y,pimpl->get_title().c_str());
	COL[BORDER_COL].set();
	draw_box(r.tl.x,r.tl.y,r.w(),h+2);
	r.tl.y += h + 2;
	COL[BG_COL].set();
	draw_filled_box(r);
	COL[BORDER_COL].set();
	draw_box(r);
	// get the lines and cursor
	const lines_t& lines = pimpl->get_lines();
	const pimpl_t::cursor_t cursor = pimpl->get_cursor();
	vec2_t caret(pimpl->char_to_ofs(cursor.col,cursor.row),cursor.row*h);
	// ensure cursor is visible
	const vec2_t margin(3*pimpl->em,3*h);
	vec2_t view_ofs = pimpl->view_ofs;
	if(view_ofs.x > caret.x-margin.x) view_ofs.x = std::max(0,caret.x-margin.x);
	if(caret.x+margin.x > view_ofs.x+r.w()) view_ofs.x = caret.x+margin.x-r.w();
	if(view_ofs.y > caret.y-margin.y) view_ofs.y = std::max(0,caret.y-margin.y);
	if(caret.y+margin.y > view_ofs.y+r.h()) view_ofs.y = ((caret.y+margin.y-r.h())/h)*h;
	pimpl->view_ofs = view_ofs;
	// and draw
	COL[CURSOR_COL].set();
	caret += r.tl;
	caret -= view_ofs;
	draw_filled_box(rect_t(caret,caret+vec2_t((now()%700)>350?10:3,h)));
	int y = r.tl.y;
	for(size_t i = view_ofs.y/h; i<lines.size(); i++) {
		if(y > r.br.y) break;
		const line_t& line = lines[i];
		const int start = pimpl->char_from_ofs(view_ofs.x,i);
		int x = r.tl.x - (view_ofs.x - pimpl->char_to_ofs(start,i));
		for(size_t j=start; j<line.s.size(); j++) {
			TEXT_COL[line.type[j]].set();
			x += f.draw(x,y,line.s[j]);
		}
		y += h;
	}
}


