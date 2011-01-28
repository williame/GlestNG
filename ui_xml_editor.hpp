/*
 ui_xml_editor.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UI_XML_EDITOR_HPP__
#define __UI_XML_EDITOR_HPP__

#include "ui.hpp"

class xml_loadable_t;

class ui_xml_editor_t: public ui_component_t {
public:
	ui_xml_editor_t(xml_loadable_t& target,ui_component_t* parent=NULL);
	~ui_xml_editor_t();
	bool offer(const SDL_Event& event);
private:
	void draw();
	class pimpl_t;
	pimpl_t* pimpl;
};

#endif //__UI_XML_EDITOR_HPP__

