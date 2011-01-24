/*
 ui_xml_editor.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>

#include "font.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "ui_xml_editor.hpp"

ui_xml_editor_t::ui_xml_editor_t(const std::string& title,istream_t& in,ui_component_t* parent):
ui_component_t(parent), body(title.c_str(),in), r(0xff), g(0xff), b(0xff) {
	try {
		body.parse();
	} catch(data_error_t* de) {
		std::cerr << "Error parsing xml: "<<de<<std::endl;
	}
	set_rect(rect_t(20,50,500,mgr.get_screen_bounds().br.y));
}

void ui_xml_editor_t::draw() {
	const rect_t rect = get_rect();
	glColor4f(.2,.2,.2,.8);
	glBegin(GL_QUADS);
		glVertex2i(rect.tl.x,rect.tl.y);
		glVertex2i(rect.br.x,rect.tl.y);
		glVertex2i(rect.br.x,rect.br.y);
		glVertex2i(rect.tl.x,rect.br.y);
	glEnd();
	glColor3ub(r,g,b);
	font_mgr_t& f = *font_mgr();
	const int em = f.measure(" ").x;
	const int tab = em * 3;
	const int h = f.measure(" ").y;
	const int startx = rect.tl.x;
	const int maxy = rect.br.y;
	int depth=1;
	int x = startx;
	int y = rect.tl.y - h;
	bool in_tag = false;
	for(xml_parser_t::walker_t node = body.walker(); node.ok(); node.next()) {
		if(y > maxy) break;
		switch(node.type()) {
		case xml_parser_t::OPEN:
			if(in_tag)
				x += f.draw(x,y,">");
			y += h; x = startx + tab * depth++;
			x += f.draw(x,y,"<");
			x += f.draw(x,y,node.str().c_str());
			in_tag = true;
			break;
		case xml_parser_t::CLOSE:
			depth--;
			if(in_tag) {
				x += f.draw(x,y,"/>");
				in_tag = false;
			} else {
				y += h; x = startx + tab * depth;
				x += f.draw(x,y,"</");
				x += f.draw(x,y,node.str().c_str());
				x += f.draw(x,y,">");
			}
			break;
		case xml_parser_t::KEY:
			x += em;
			x += f.draw(x,y,node.str().c_str());
			break;
		case xml_parser_t::VALUE:
			x += f.draw(x,y,"=\"");
			x += f.draw(x,y,node.str().c_str());
			x += f.draw(x,y,"\"");
			break;
		case xml_parser_t::DATA:
			in_tag = false;
			x += f.draw(x,y,">");
			y += h;
			x = startx + tab * depth;
			x += f.draw(x,y,node.str().c_str());
			break;
		case xml_parser_t::ERROR:
			x += f.draw(x,y,"ERROR ");
			x += f.draw(x,y,node.str().c_str());
			return;
		}
	}
}


