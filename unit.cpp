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

bool unit_type_t::cellmap_at(int x,int y) const {
	if(x<0 || x>=size || y<0 || y>=size)
		panic(this<<" cellmap is "<<size<<", cannot get ("<<x<<','<<y<<')');
	return cellmap & ((1<<x) << (y*8));
}

void unit_type_t::reset() {
	resource_requirements.clear();
	unit_requirements.clear();
	upgrade_requirements.clear();
	resources_stored.clear();
	skills.clear();
}

void unit_type_t::_load_xml(xml_parser_t::walker_t& xml) {
	reset();
	xml.check("unit");
	height = xml.get_child("parameters").get_child("height").value_int();
	max_ep = xml.get_peer("max-ep").value_int();
	regeneration_ep = xml.has_key("regeneration")? xml.value_int("regeneration"): 0;
	max_hp = xml.get_peer("max-hp").value_int();
	regeneration_hp = xml.has_key("regeneration")? xml.value_int("regeneration"): 0;
	sight = xml.get_peer("sight").value_int();
	time = xml.get_peer("time").value_int();
	armour = xml.get_peer("armor").value_int();
	armour_type = mgr.techtree().armour_ID(xml.get_peer("armor-type").value_string());
	size = xml.get_peer("size").value_int();
	if(size<1 || size>8) data_error("size is out of range");
	cellmap = ~0;
	if(xml.up().has_child("cellmap")) {
		if(xml.get_child("cellmap").value_bool()) {
			cellmap = 0;
			for(int i=0; i<size; i++) {
				if(!xml.get_child("row",i)) data_error("not enough rows in cellmap");
				const std::string bits = xml.value_string();
				if(bits.size() != size) data_error("wrong number of bits in cellmap row");
				for(size_t j=0; j<size; j++)
					if(bits[j] == '1')
						cellmap |= (1 << j) << (i*8);
					else if(bits[j] != '0')
						data_error(bits[j]<<" is not binary digit");
				xml.up();
			}
			if(xml.get_child("row",size)) data_error("cellmap has too many rows");
			if(!cellmap) data_error("cellmap cannot be empty");
		}
		xml.up();
	}
	if(xml.has_child("multi-selection")) {
		multi_selection = xml.get_child("multi-selection").value_bool();
		xml.up();
	} else multi_selection = false;
	if(xml.has_child("light")) {
		if(xml.get_child("light").value_bool("enabled")) {
			light_colour.r = xml.value_float("red");
			if(light_colour.r<0 || light_colour.r>1) data_error("light colour red is illegal");
			light_colour.g = xml.value_float("green");
			if(light_colour.g<0 || light_colour.g>1) data_error("light colour green is illegal");
			light_colour.b = xml.value_float("blue");
			if(light_colour.b<0 || light_colour.b>1) data_error("light colour blue is illegal");
		}
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
	if(xml.has_child("resources-stored")) {
		xml.get_child("resources-stored");
		for(size_t i=0; xml.get_child("resource",i); xml.up(), i++) {
			const std::string name(xml.value_string("name"));
			if(!mgr.techtree().get_resources().contains(name))
				data_error("resource "<<name<<" not in "<<mgr.techtree());
			resources_stored.push_back(mgr.techtree(),RESOURCE,name,
				xml.value_int("amount"));
		}
		xml.up();
	}
	xml.check("parameters").up();
	if(xml.has_child("skills")) {
		xml.get_child("skills");
		for(size_t i=0; xml.get_child("skill",i); xml.up(), i++)
			skills.push_back(skill_t(xml));
		xml.up();
	}
}

unit_type_t::skill_t::skill_t(xml_parser_t::walker_t& xml) {
	type = xml.get_child("type").value_string();
	name = xml.get_peer("name").value_string();
	ep_cost = xml.get_peer("ep-cost").value_int();
	speed = xml.get_peer("speed").value_int();
	xml.up();
}

