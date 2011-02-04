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
			if(fs().exists(path+"/units/"+*i+"/"+*i+".xml"))
				units.push_back(*i);
		if(!units.size())
			data_error("faction "<<name<<" contains no units");
	}
	{
		const strings_t subdirs = fs().list_dirs(path+"/upgrades");
		for(strings_t::const_iterator i=subdirs.begin(); i!=subdirs.end(); i++)
			if(fs().exists(path+"/upgrades/"+*i+"/"+*i+".xml"))
				upgrades.push_back(*i);
	}
	fs_file_t::ptr_t f(fs().get(get_xml_path()));
	istream_t::ptr_t in(f->reader());
	load_xml(*in);
}

std::string faction_t::get_xml_path() const {
	return path+"/"+class_t::name+".xml";
}

faction_t::~faction_t() { reset(); }

void faction_t::reset() {}

void faction_t::_load_xml(xml_parser_t::walker_t& xml) {
	xml.check("faction");
}

