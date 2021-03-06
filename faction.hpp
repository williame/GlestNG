/*
 faction.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __FACTION_HPP__
#define __FACTION_HPP__

#include "techtree.hpp"

class faction_t: public class_t, public xml_loadable_t {
public:
	faction_t(techtree_t& techtree,const std::string& name);
	const std::string path;
	std::string get_xml_path() const;
	const strings_t& get_units() const { return unit_names; }
	const strings_t& get_upgrades() const { return upgrade_names; }
private:
	~faction_t();
	void reset();
	void _load_xml(xml_parser_t::walker_t& xml);
	refs_t starting_resources, starting_units;
	strings_t unit_names, upgrade_names;
	refs_t units, upgrades;
	ref_t loading_screen, music;
};

#endif //__FACTION_HPP__

