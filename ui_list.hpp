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
	ui_list_t(unsigned flags,const std::string& title,const strings_t& list,ui_component_t* parent=NULL);
	~ui_list_t();
	struct handler_t {
		virtual void on_selected(ui_list_t* lst,size_t idx,const vec2_t& pt) = 0;
		virtual void on_cancelled(ui_list_t* lst) {}
	};
	handler_t* set_handler(handler_t* handler);
	vec2_t preferred_size() const;
	bool is_enabled() const;
	void enable();
	void disable();
	const std::string& get_title() const;
	enum { // flags that can be placed in the strings_t that have special meaning to ui_list_t
		TAG_SEP = 0x10000,
	};
	const strings_t& get_list() const;
	bool has_selection() const;
	size_t get_selection() const;
	void set_selection(size_t idx);
	void clear_selection();
	static const unsigned default_flags;
protected:
	void reshaped();
	void visibility_changed(bool visible);
private:
	bool offer(const SDL_Event& event);
	void draw();
	struct pimpl_t;
	pimpl_t* pimpl;
};

#endif //__UI_LIST_HPP__

