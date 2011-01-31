/*
 techtree.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>
#include <algorithm>

#include "techtree.hpp"

techtree_t::techtree_t(fs_t& fs,const std::string& name):
	mgr_t(fs), xml_loadable_t(name),
	path(fs.canocial(std::string("techs")+"/"+name)) {
	const strings_t subdirs = fs.list_dirs(path+"/factions");
	for(strings_t::const_iterator i=subdirs.begin(); i!=subdirs.end(); i++)
		if(fs.exists(path+"/factions/"+*i+"/"+*i+".xml")) {
			factions.push_back(*i);
			faction_refs.push_back(ref(FACTION,*i));
		}
	if(!factions.size())
		data_error("techtree "<<name<<" contains no factions");
	fs_file_t::ptr_t f(fs.get(path+"/"+name+".xml"));
	istream_t::ptr_t in(f->reader());
	load_xml(*in);
}

techtree_t::~techtree_t() {
	reset();
	for(refs_t::iterator i=faction_refs.begin(); i!=faction_refs.end(); i++)
		delete *i;
}

void techtree_t::reset() {
	damage_multipliers.clear();
	attack_types.clear();
	armour_types.clear();
}

void techtree_t::_load_xml(xml_parser_t::walker_t& xml) {
	reset();
	xml.check("tech-tree");
	description = xml.get_child("description").value_string();
	xml.up().get_child("attack-types");
	while(xml.get_child("attack-type",attack_types.size())) {
		attack_types.push_back(xml.value_string("name"));
		xml.up();
	}
	xml.up().get_child("armor-types");
	while(xml.get_child("armor-type",armour_types.size())) {
		armour_types.push_back(xml.value_string("name"));
		xml.up();
	}
	xml.up().get_child("damage-multipliers");
	std::vector<bool> set(attack_types.size()*armour_types.size(),false);
	damage_multipliers = std::vector<float>(attack_types.size()*armour_types.size(),1);
	for(size_t dm = 0; xml.get_child("damage-multiplier",dm); dm++) {
		const size_t armour = armour_ID(xml.value_string("armor")),
			attack = attack_ID(xml.value_string("attack")),
			idx = armour*attack_types.size()+attack;
		if(set[idx]) data_error("duplicate damage-multiplier for "<<
			armour_types[armour]<<" vs "<<attack_types[attack]);
		damage_multipliers[idx] = xml.value_float();
		set[idx] = true;
		xml.up();
	}
}

float techtree_t::damage_multiplier(const std::string& armour,const std::string& attack) const {
	return damage_multiplier(armour_ID(armour),attack_ID(attack));
}

float techtree_t::damage_multiplier(size_t armour,size_t attack) const {
	if(armour >= armour_types.size()) panic("armour ID "<<armour<<" does not exist");
	if(attack >= attack_types.size()) panic("attack ID "<<attack<<" does not exist");
	return damage_multipliers[armour*attack_types.size()+attack];
}

static size_t _index_of(const strings_t& c,const std::string& s,const char* ctx) {
	const size_t i = std::distance(c.begin(),std::find(c.begin(),c.end(),s));
	if(i == c.size()) data_error("could not find "<<ctx<<" "<<s);
	return i;
}

size_t techtree_t::attack_ID(const std::string& s) const {
	return _index_of(attack_types,s,"attack");
}

size_t techtree_t::armour_ID(const std::string& s) const {
	return _index_of(armour_types,s,"armour");
}

faction_t* techtree_t::get_faction(const std::string& name) {
	return faction_refs[_index_of(factions,name,"faction")]->faction();
}

techtrees_t::techtrees_t(fs_t& fs): fs_handle_t(fs) {
	const strings_t subdirs = fs.list_dirs("techs");
	for(strings_t::const_iterator i=subdirs.begin(); i!=subdirs.end(); i++)
		if(fs.exists(std::string("techs")+"/"+*i+"/"+*i+".xml"))
			techtrees.push_back(*i);
}


