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
	const strings_t& get_units() const { return units; }
	const strings_t& get_upgrades() const { return upgrades; }
private:
	~faction_t();
	void reset();
	void _load_xml(xml_parser_t::walker_t& xml);
	strings_t units, upgrades;
};

#endif //__FACTION_HPP__

