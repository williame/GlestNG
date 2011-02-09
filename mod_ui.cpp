/*
 mod_ui.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>

#include "mod_ui.hpp"
#include "techtree.hpp"
#include "resource.hpp"
#include "faction.hpp"

#include "ui_list.hpp"
#include "ui_xml_editor.hpp"

static ref_t base; 

struct base_handler_t: public ui_list_t::handler_t, public ui_xml_editor_t::handler_t {
	base_handler_t(): context_menu(NULL), xml(NULL) {}
	enum {	TAG_EDIT_XML=1,
		TAG_SEP,
		TAG_TT_FAC, TAG_TT_RES,
		TAG_FAC_UNITS, TAG_FAC_UPG,
	};
	void clear();
	void create(ref_t ref);
	void create_context_menu();
	void activate_last();
	refs_t refs;
	std::vector<ui_list_t*> menus;
	ui_list_t* context_menu;
	ui_xml_editor_t* xml;
	std::auto_ptr<xml_loadable_t> edit;
	void on_selected(ui_list_t* lst,size_t idx,const vec2_t& pt);
	void on_cancelled(ui_list_t* lst);
	void on_cancelled(ui_xml_editor_t* xml);
} base_handler;

void base_handler_t::clear() {
	if(menus.size()) {
		menus[0]->destroy();
		menus.clear();
	}
	context_menu = NULL;
	xml = NULL;
	edit.reset();
	refs.clear();
}

void base_handler_t::create(ref_t ref) {
	std::cout << "(create menu for "<<ref<<')'<<std::endl;
	refs.push_back(ref);
	ui_component_t* parent = (menus.size()? menus[0]: NULL);
	ui_list_t* menu = NULL;
	switch(ref.get_type()) {
	case TECHTREE: {
		techtree_t& techtree = ref.techtree();
		strings_t items, lst;
		items.push_back("factions:",TAG_SEP);
		lst = techtree.get_factions();
		for(strings_t::const_iterator i=lst.begin(); i!=lst.end(); i++)
			items.push_back(*i,TAG_TT_FAC);
		items.push_back("resources:",TAG_SEP);
		lst = techtree.get_resources();
		for(strings_t::const_iterator i=lst.begin(); i!=lst.end(); i++)
			items.push_back(*i,TAG_TT_RES);
		menu = new ui_list_t(ui_list_t::default_flags,techtree.name,items,parent);
	} break;
	case FACTION: {
		faction_t& faction = ref.faction();
		strings_t items;
		switch(ref.get_tag()) {
		case TAG_FAC_UNITS: items = faction.get_units(); break;
		case TAG_FAC_UPG: items = faction.get_upgrades(); break;
		default: panic(ref << " bad tag: "<<ref.get_tag());
		}
		menu = new ui_list_t(ui_list_t::default_flags,faction.class_t::name,items,parent);
	} break;
	default:
		std::cerr << "cannot create menu for " << ref << std::endl;
		hide_mod_ui();
		return;
	}
	if(!menu) panic("menu not set");
	menu->set_rect(rect_t(vec2_t(10,50)+(vec2_t(10,0)*menus.size()),vec2_t(10,50)+menu->preferred_size()));
	menu->set_handler(this);
	menus.push_back(menu);
}

void base_handler_t::create_context_menu() {
	const ref_t& ref = refs.back();
	strings_t items;
	items.push_back("edit XML",TAG_EDIT_XML);
	switch(ref.get_type()) {
	case TECHTREE:
		items.push_back("factions",TAG_TT_FAC);
		items.push_back("resources",TAG_TT_RES);
		break;
	case FACTION:
		items.push_back("units",TAG_FAC_UNITS);
		items.push_back("upgrades",TAG_FAC_UPG);
		break;
	default:
		std::cerr << "cannot create context menu for " << ref << std::endl;
		hide_mod_ui();
		return;
	}
	context_menu = new ui_list_t(ui_list_t::default_flags,ref.get_name(),items,menus.back());
	context_menu->set_rect(rect_t(vec2_t(10,50)+(vec2_t(10,0)*menus.size()),vec2_t(10,50)+context_menu->preferred_size()));
	context_menu->set_handler(this);
}

void base_handler_t::on_selected(ui_list_t* lst,size_t idx,const vec2_t& pt) {
	if(lst == context_menu) {
		switch(lst->get_list()[idx].tag) {
		case TAG_EDIT_XML: { 
			std::cout << "edit xml for " << lst->get_title() << std::endl;	
			/*
			edit.reset(new faction_t(*techtree,faction->class_t::name));
			xml = new ui_xml_editor_t(ui_xml_editor_t::default_flags,*edit,*this);
			rect_t r(pt.x,factions_menu->get_pos().y,
				pt.x+500,ui_mgr()->get_screen_bounds().h()-factions_menu->get_pos().y);
			xml->set_rect(r); */
		} break;
		default: {
			std::cerr << "context menu "<< idx << " " << lst->get_list()[idx] << " not handled" << std::endl;
			activate_last();
		}}
		context_menu->destroy(); context_menu = NULL;
	}
}

void base_handler_t::on_cancelled(ui_list_t* lst) {
	if(lst == menus.back()) {
		lst->destroy();
		refs.resize(refs.size()-1);
		menus.resize(menus.size()-1);
	} else if(lst == context_menu) {
		context_menu->destroy(); context_menu = NULL;
	} else
		panic(lst << " was cancelled, was expecting " << menus.back());
	activate_last();
}

void base_handler_t::activate_last() {
	if(menus.size()) {
		ui_list_t* menu = menus.back();
		menu->enable();
		menu->clear_selection();
		menu->show();
	} else
		hide_mod_ui();
}

void base_handler_t::on_cancelled(ui_xml_editor_t* xml) {
	xml->destroy(); xml = NULL;
	activate_last();
}

void show_mod_ui(const ref_t& b) {
	if(!b.is_set()) panic("cannot show a mod UI");
	hide_mod_ui();
	base = b;
	base_handler.create(base);
}

bool mod_ui_is_shown() {
	return base.is_set();
}

void hide_mod_ui() {
	base.clear();
	base_handler.clear();
}


