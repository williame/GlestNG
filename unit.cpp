/*
 unit.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>

#include "unit.hpp"
#include "faction.hpp"
#include "fs.hpp"

unit_type_t::unit_type_t(faction_t& fraction_,const std::string& name):
	class_t(fraction_.mgr,UNIT_TYPE,name),
	xml_loadable_t(name),
	path(fraction_.fs().canocial(fraction_.path+"/units/"+name)),
	faction(fraction_)
{
	fs_file_t::ptr_t f(fs().get(path+"/"+name+".xml"));
	istream_t::ptr_t in(f->reader());
	load_xml(*in);	
}

unit_type_t::~unit_type_t() { reset(); }

void unit_type_t::reset() {
	resource_requirements.clear();
	unit_requirements.clear();
	upgrade_requirements.clear();
}

void unit_type_t::_load_xml(xml_parser_t::walker_t& xml) {
	reset();
	xml.check("unit");
	height = xml.get_child("parameters").get_child("height").value_float();
	size = xml.get_peer("size").value_float();
	max_ep = xml.get_peer("max-ep").value_int();
	regeneration_ep = xml.has_key("regeneration")? xml.value_int("regeneration"): 0;
	max_hp = xml.get_peer("max-hp").value_int();
	regeneration_hp = xml.has_key("regeneration")? xml.value_int("regeneration"): 0;
	sight = xml.get_peer("sight").value_int();
	time = xml.get_peer("time").value_int();
	armour = xml.get_peer("armor").value_int();
	armour_type = mgr.techtree().armour_ID(xml.get_peer("armor-type").value_string());
	if(xml.up().has_child("multi-selection")) {
		multi_selection = xml.get_child("multi-selection").value_bool();
		xml.up();
	} else multi_selection = false;
	if(xml.has_child("light")) {
		light = xml.get_child("light").value_bool("enabled");
		xml.up();
	} else light = false;
	xml.get_child("resource-requirements");
	for(size_t i=0; xml.get_child("resource",i); xml.up(), i++) {
		const std::string name(xml.value_string("name"));
		if(!mgr.techtree().get_resources().contains(name))
			data_error("resource "<<name<<" not in "<<mgr.techtree());
		resource_requirements.push_back(mgr.techtree(),RESOURCE,name,
			xml.value_int("amount"));
	}
	xml.up();
	if(xml.has_child("unit-requirements")) {
		xml.get_child("unit-requirements");
		for(size_t i=0; xml.get_child("unit",i); xml.up(), i++) {
			const std::string name(xml.value_string("name"));
			if(!faction.get_units().contains(name))
				data_error("unit "<<name<<" not in "<<faction);
			unit_requirements.push_back(mgr.techtree(),UNIT_TYPE,name);
		}
		xml.up();
	}
	if(xml.has_child("upgrade-requirements")) {
		xml.get_child("upgrade-requirements");
		for(size_t i=0; xml.get_child("upgrade",i); xml.up(), i++) {
			const std::string name(xml.value_string("name"));
			if(!faction.get_upgrades().contains(name))
				data_error("upgrade "<<name<<" not in "<<faction);
			resource_requirements.push_back(mgr.techtree(),UPGRADE,name);
		}
		xml.up();
	}
}

