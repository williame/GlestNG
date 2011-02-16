/*
 faction.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "faction.hpp"

faction_t::faction_t(techtree_t& techtree,const  std::string& name):
	class_t(techtree,FACTION,name), xml_loadable_t(name),
	path(techtree.fs().canocial(techtree.path+"/factions/"+name)) {
	{
		const strings_t subdirs = fs().list_dirs(path+"/units");
		for(strings_t::const_iterator i=subdirs.begin(); i!=subdirs.end(); i++)
			if(fs().exists(path+"/units/"+*i+"/"+*i+".xml")) {
				unit_names.push_back(*i);
				units.push_back(techtree,UNIT_TYPE,*i);
			}
		if(!units.size())
			data_error("faction "<<name<<" contains no units");
	}
	if(fs().is_dir(path+"/upgrades")) {
		const strings_t subdirs = fs().list_dirs(path+"/upgrades");
		for(strings_t::const_iterator i=subdirs.begin(); i!=subdirs.end(); i++)
			if(fs().exists(path+"/upgrades/"+*i+"/"+*i+".xml")) {
				upgrade_names.push_back(*i);
				upgrades.push_back(techtree,UPGRADE,*i);
			}
	}
	if(fs().is_file(path+"/loading_screen.jpg"))
		loading_screen.set(techtree,IMAGE,fs().canocial(path+"/loading_screen.jpg"));
	fs_file_t::ptr_t f(fs().get(get_xml_path()));
	istream_t::ptr_t in(f->reader());
	load_xml(*in);
}

std::string faction_t::get_xml_path() const {
	return path+"/"+class_t::name+".xml";
}

faction_t::~faction_t() { reset(); }

void faction_t::reset() {
	upgrades.clear(); upgrade_names.clear();
	units.clear(); unit_names.clear();
	starting_resources.clear();
}

void faction_t::_load_xml(xml_parser_t::walker_t& xml) {
	xml.check("faction");
	xml.get_child("starting-resources");
	for(size_t i=0; xml.get_child("resource",i); xml.up(), i++) {
		const std::string name(xml.value_string("name"));
		if(!mgr.techtree().get_resources().contains(name))
			data_error("resource "<<name<<" not in "<<mgr.techtree());
		starting_resources.push_back(mgr.techtree(),RESOURCE,
			name,xml.value_int("amount"));
	}
	xml.get_peer("starting-units");
	for(size_t i=0; xml.get_child("unit",i); xml.up(), i++) {
		const std::string name(xml.value_string("name"));
		if(!units.contains(UNIT_TYPE,name))
			data_error("unit "<<name<<" not in "<<this);
		starting_units.push_back(mgr.techtree(),UNIT_TYPE,
			name,xml.value_int("amount"));
	}
	if(xml.up().has_child("music") && xml.get_child("music").value_bool())
		music.set(mgr.techtree(),AUDIO,xml.value_string("path"));
}

