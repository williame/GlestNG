/*
 ui_xml_editor.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UI_XML_EDITOR_HPP__
#define __UI_XML_EDITOR_HPP__

#include "ui.hpp"
#include "xml.hpp"

class ui_xml_editor_t: public ui_component_t {
public:
	ui_xml_editor_t(const std::string& title,istream_t& in,ui_component_t* parent=NULL);
	void set_color(uint8_t r_,uint8_t g_,uint8_t b_) { r = r_; g = g_; b = b_; }
private:
	void draw();
	xml_parser_t body;
	uint8_t r,g,b;
};

#endif //__UI_XML_EDITOR_HPP__

