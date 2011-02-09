/*
 mod_ui.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __MOD_UI_HPP__
#define __MOD_UI_HPP__

#include <memory>

class ref_t;

class mod_ui_t {
public:
	static std::auto_ptr<mod_ui_t> create();
	~mod_ui_t();
	void show(const ref_t& base);
	bool is_shown() const;
	void hide();
private:
	struct pimpl_t;
	pimpl_t* pimpl;
	mod_ui_t();
};

#endif //__MOD_UI_HPP__

