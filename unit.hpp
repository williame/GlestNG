/*
 unit.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UNIT_HPP__
#define __UNIT_HPP__

#include <string>

#include "world.hpp"
#include "xml.hpp"
#include "ref.hpp"

class unit_type_t;

class unit_t: public object_t {
public:
	unit_t(const unit_type_t& type);
private:
	const unit_type_t& type;
};

class unit_type_t: public class_t, public xml_loadable_t {
public:
	unit_type_t(faction_t& faction,const std::string& name);
	~unit_type_t();
	bool cellmap_at(int x,int y) const;
	const std::string path;
	struct skill_t {
		skill_t() {}
		skill_t(xml_parser_t::walker_t& xml);
		std::string type, name;
		int ep_cost, speed;
	};	
	typedef std::vector<skill_t> skills_t;
	friend class unit_t;
private:
	faction_t& faction;
	void reset();
	void _load_xml(xml_parser_t::walker_t& xml);
	int height;
	int size;
	uint64_t cellmap;
	int max_ep, regeneration_ep;
	int max_hp, regeneration_hp;
	int sight;
	int time;
	int armour;
	size_t armour_type;
	bool multi_selection;
	bool light; struct { float r,g,b; } light_colour;
	refs_t resource_requirements, unit_requirements, upgrade_requirements;
	refs_t resources_stored;
	skills_t skills;
};

#endif //__UNIT_HPP__

