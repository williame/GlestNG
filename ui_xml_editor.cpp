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

#include "font.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "xml.hpp"
#include "ui_xml_editor.hpp"

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
	pimpl_t(ui_xml_editor_t& ui_,const std::string& title,istream_t& in):
		em(font_mgr()->measure(' ').x),
		h(font_mgr()->measure(' ').y), 
		view_ofs(0,0),
		ui(ui_),
		body(title.c_str(),in), dirty(true) {
		cursor.row = cursor.col = 0;
	}
	void parse();
	const lines_t& get_lines() { parse(); return lines; }
	const char* get_title() const { return body.get_title(); }
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
	const int em; // width of space
	const int h; // line height
	vec2_t view_ofs;
private:
	ui_xml_editor_t& ui;
	void _append(line_t& line,int i,xml_parser_t::type_t type);
	xml_parser_t body;
	lines_t lines;
	bool dirty;
	cursor_t cursor;
};

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

void ui_xml_editor_t::pimpl_t::nav_pgup() {}

void ui_xml_editor_t::pimpl_t::nav_home() { cursor.col = 0; }

void ui_xml_editor_t::pimpl_t::nav_end() { cursor.col = lines[cursor.row].s.size(); }

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

void ui_xml_editor_t::pimpl_t::_append(line_t& line,int i,xml_parser_t::type_t type) {
	char ch = body.get_buf()[i];
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
	try {
		body.parse();
	} catch(data_error_t* de) {
		std::cerr << "Error parsing xml: "<<de<<std::endl;
	}
	size_t i = 0;
	line_t line;
	for(xml_parser_t::walker_t node = body.walker(); node.ok(); node.next()) {
		for(; i<node.ofs(); i++)
			_append(line,i,xml_parser_t::IGNORE);
		for(; i<(node.ofs()+node.len()); i++)
			_append(line,i,node.type());
	}
	for(; i<strlen(body.get_buf()); i++)
		_append(line,i,xml_parser_t::IGNORE);
	line.s.erase(line.s.find_last_not_of(" \n\r\t")+1);
	if(line.s.size())
		lines.push_back(line);
	dirty = false;
}


ui_xml_editor_t::ui_xml_editor_t(const std::string& title,istream_t& in,ui_component_t* parent):
ui_component_t(parent), pimpl(new pimpl_t(title.c_str(),in)) {
	set_rect(rect_t(20,50,500,mgr.get_screen_bounds().br.y-30));
}

ui_xml_editor_t::~ui_xml_editor_t() {
	delete pimpl;
}

bool ui_xml_editor_t::offer(const SDL_Event& event) {
	switch(event.type) {
	case SDL_KEYUP:
		return true; // ignore them but eat them
	case SDL_KEYDOWN: {
		switch(event.key.keysym.sym) {
		case SDLK_ESCAPE: return false;
		case SDLK_LEFT: pimpl->nav_left(); break;
		case SDLK_RIGHT: pimpl->nav_right(); break;
		case SDLK_UP: pimpl->nav_up(); break;
		case SDLK_DOWN: pimpl->nav_down(); break;
		case SDLK_PGUP: pimpl->nav_pgup(); break;
		case SDLK_PGDN: pimpl->nav_pgdn(); break;
		case SDLK_HOME: pimpl->nav_home(); break;
		case SDLK_END: pimpl->nav_end(); break;
		default:;
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
	{0xff,0x40,0x40,0xff}, //KEY
	{0x00,0x20,0xff,0xff}, //VALUE
	{0x00,0x20,0xff,0xff}, //DATA
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
	f.draw(r.tl.x+10,r.tl.y,pimpl->get_title());
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
	draw_filled_box(rect_t(caret,caret+vec2_t(3,h)));
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


