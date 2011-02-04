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
	struct handler_t {
		virtual void on_cancelled(ui_xml_editor_t* xml) = 0;
	};
	ui_xml_editor_t(unsigned flags,xml_loadable_t& target,handler_t& handler,ui_component_t* parent=NULL);
	~ui_xml_editor_t();
	bool offer(const SDL_Event& event);
	static const unsigned default_flags;
private:
	void reshaped();
	void visibility_changed();
	void draw();
	class pimpl_t;
	pimpl_t* pimpl;
};

#endif //__UI_XML_EDITOR_HPP__

