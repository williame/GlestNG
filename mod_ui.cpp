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
#include "unit.hpp"

#include "ui_list.hpp"
#include "ui_xml_editor.hpp"

struct mod_ui_t::pimpl_t: public ui_list_t::handler_t, public ui_xml_editor_t::handler_t {
	pimpl_t(): context_menu(NULL), xml(NULL) {}
	enum {	TAG_EDIT_XML=1,
		TAG_TT_TT,
		TAG_TT_FAC, TAG_TT_RES,
		TAG_FAC_UNIT, TAG_FAC_UPG,
	};
	void clear();
	void create(ref_t ref);
	void create_context_menu(ref_t ref,const vec2_t& pt);
	void activate_last();
	refs_t refs;
	std::vector<ui_list_t*> menus;
	ref_t context_ref;
	ui_list_t* context_menu;
	ui_xml_editor_t* xml;
	std::auto_ptr<xml_loadable_t> edit;
	void on_selected(ui_list_t* lst,size_t idx,const vec2_t& pt);
	void on_cancelled(ui_list_t* lst);
	void on_cancelled(ui_xml_editor_t* xml);
};

void mod_ui_t::pimpl_t::clear() {
	if(menus.size()) {
		menus[0]->destroy();
		menus.clear();
	}
	context_ref.clear();
	context_menu = NULL;
	xml = NULL;
	edit.reset();
	refs.clear();
}

void mod_ui_t::pimpl_t::create(ref_t ref) {
	std::cout << "(create menu for "<<ref<<')'<<std::endl;
	refs.push_back(ref);
	ui_component_t* parent = (menus.size()? menus[0]: NULL);
	ui_list_t* menu = NULL;
	switch(ref.get_type()) {
	case TECHTREE: {
		techtree_t& techtree = ref.techtree();
		strings_t items, lst;
		items.push_back("techtree",TAG_TT_TT);
		items.push_back("factions:",ui_list_t::TAG_SEP);
		lst = techtree.get_factions();
		for(strings_t::const_iterator i=lst.begin(); i!=lst.end(); i++)
			items.push_back(*i,TAG_TT_FAC);
		items.push_back("resources:",ui_list_t::TAG_SEP);
		lst = techtree.get_resources();
		for(strings_t::const_iterator i=lst.begin(); i!=lst.end(); i++)
			items.push_back(*i,TAG_TT_RES);
		menu = new ui_list_t(ui_list_t::default_flags,techtree.name,items,parent);
	} break;
	case FACTION: {
		faction_t& faction = ref.faction();
		strings_t items;
		switch(ref.get_tag()) {
		case TAG_FAC_UNIT: items = faction.get_units(); break;
		case TAG_FAC_UPG: items = faction.get_upgrades(); break;
		default: panic(ref << " bad tag: "<<ref.get_tag());
		}
		for(strings_t::iterator i=items.begin(); i!=items.end(); i++)
			i->tag = ref.get_tag();
		menu = new ui_list_t(ui_list_t::default_flags,faction.class_t::name,items,parent);
	} break;
	default:
		panic("cannot create menu for " << ref);
	}
	if(!menu) panic("menu not set");
	const vec2_t sz(menu->preferred_size()), ofs(vec2_t(10,50)+(vec2_t(50,0)*menus.size()));
	menu->set_rect(rect_t(ofs,ofs+sz));
	menu->set_handler(this);
	menus.push_back(menu);
}

void mod_ui_t::pimpl_t::create_context_menu(ref_t ref,const vec2_t& pt) {
	context_ref = ref;
	strings_t items;
	items.push_back("edit XML",TAG_EDIT_XML);
	switch(ref.get_type()) {
	case FACTION:
		items.push_back("units",TAG_FAC_UNIT);
		items.push_back("upgrades",TAG_FAC_UPG);
		break;
	case TECHTREE:
	case RESOURCE:
	case UNIT_TYPE:
		break;
	default:
		panic("cannot create context menu for " << ref);
	}
	context_menu = new ui_list_t(ui_list_t::default_flags,ref.get_name(),items,menus.back());
	const vec2_t sz(context_menu->preferred_size()), ofs(pt-vec2_t(10,sz.y/2));
	context_menu->set_rect(rect_t(ofs,ofs+sz));
	context_menu->set_handler(this);
}

void mod_ui_t::pimpl_t::on_selected(ui_list_t* lst,size_t idx,const vec2_t& pt) {
	const tagged_string_t& item = lst->get_list()[idx];
	if(lst == context_menu) {
		techtree_t& techtree = context_ref.get_mgr().techtree();
		const std::string name = context_ref.get_name();
		if(item.tag == TAG_EDIT_XML) {
			std::cout << "edit xml for " << context_ref << std::endl;
			switch(context_ref.get_type()) {
			case TECHTREE: 
				edit.reset(new techtree_t(techtree.fs(),name));
				break;
			case RESOURCE:
				edit.reset(new resource_t(techtree,name));
				break;
			case FACTION:
				edit.reset(new faction_t(techtree,name));
				break;
			case UNIT_TYPE:
				edit.reset(new unit_type_t(refs.back().faction(),name));
				break;
			default: panic(context_ref << " not handled"); }
			xml = new ui_xml_editor_t(ui_xml_editor_t::default_flags,*edit,*this);
			const short y = menus.back()->get_pos().y;
			const rect_t r(pt.x,y,pt.x+500,ui_mgr()->get_screen_bounds().h()-y);
			xml->set_rect(r);
		} else {
			context_ref.set_tag(item.tag);
			create(context_ref);
		}
		context_ref.clear();
		context_menu->destroy(); context_menu = NULL;
	} else {
		lst->disable();
		class_type_t type;
		std::string name = item;
		switch(item.tag) {
		case TAG_TT_TT: type = TECHTREE; name = refs.front().get_name(); break;
		case TAG_TT_FAC: type = FACTION; break;
		case TAG_TT_RES: type = RESOURCE; break;
		case TAG_FAC_UNIT: type = UNIT_TYPE; break;
		default: panic(item << " not handled");
		}
		create_context_menu(ref_t(refs.front().get_mgr(),type,name),pt);
	}
}

void mod_ui_t::pimpl_t::on_cancelled(ui_list_t* lst) {
	if(lst == menus.back()) {
		refs.resize(refs.size()-1);
		menus.resize(menus.size()-1);
		lst->destroy();
	} else if(lst == context_menu) {
		context_menu->destroy(); context_menu = NULL;
		refs.resize(refs.size()-1);
		menus.back()->enable();
	} else
		panic(lst << " was cancelled, was expecting " << menus.back());
	activate_last();
}

void mod_ui_t::pimpl_t::activate_last() {
	if(menus.size()) {
		ui_list_t* menu = menus.back();
		menu->enable();
		menu->clear_selection();
		menu->show();
	} else
		clear();
}

void mod_ui_t::pimpl_t::on_cancelled(ui_xml_editor_t* xml) {
	xml->destroy(); xml = NULL;
	activate_last();
}

std::auto_ptr<mod_ui_t> mod_ui_t::create() {
	return std::auto_ptr<mod_ui_t>(new mod_ui_t());
}

mod_ui_t::mod_ui_t(): pimpl(new pimpl_t()) {}

mod_ui_t::~mod_ui_t() {
	delete pimpl;
}
	
void mod_ui_t::show(const ref_t& base) {
	if(!base.is_set()) panic("cannot show a mod UI");
	hide();
	pimpl->create(base);
}

bool mod_ui_t::is_shown() const {
	return pimpl->menus.size();
}

void mod_ui_t::hide() {
	pimpl->clear();
}


