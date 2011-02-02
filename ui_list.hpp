/*
 ui_list.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UI_LIST_HPP__
#define __UI_LIST_HPP__

#include "fs.hpp" //strings_t
#include "ui.hpp"

class ui_list_t: public ui_component_t {
public:
	enum style_t {
		LIST,
		MENU,
	};
	ui_list_t(style_t style,const strings_t& list,ui_component_t* parent=NULL);
	~ui_list_t();
	bool offer(const SDL_Event& event);
private:
	void draw();
	struct pimpl_t;
	pimpl_t* pimpl;
};

#endif //__UI_LIST_HPP__

