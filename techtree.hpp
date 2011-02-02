/*
 techtree.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __TECHTREE_HPP__
#define __TECHTREE_HPP__

#include <string>

#include "fs.hpp"
#include "xml.hpp"
#include "ref.hpp"

class faction_t;
class resource_t;

class techtree_t: public mgr_t, public xml_loadable_t {
public:
	techtree_t(fs_t& fs,const std::string& name);
	~techtree_t();
	const std::string path;
	const strings_t& get_factions() const { return factions; }
	faction_t* get_faction(const std::string& name);
	const strings_t& get_resources() const { return resources; }
	resource_t* get_resource(const std::string& name);
	const strings_t& get_armour_types() const { return armour_types; }
	const strings_t& get_attack_types() const { return attack_types; }
	size_t attack_ID(const std::string& s) const;
	size_t armour_ID(const std::string& s) const;
	float damage_multiplier(const std::string& armour,const std::string& attack) const;
	float damage_multiplier(size_t armour,size_t attack) const;
private:
	void reset();
	virtual techtree_t& techtree() { return *this; }
	void _load_xml(xml_parser_t::walker_t& xml);
	std::string description;
	refs_t faction_refs;
	strings_t factions;
	refs_t resource_refs;
	strings_t resources;
	strings_t armour_types, attack_types;
	std::vector<float> damage_multipliers;
};

class techtrees_t: public fs_handle_t {
public:
	techtrees_t(fs_t& fs);
	const strings_t& get_techtrees() const { return techtrees; }
private:
	strings_t techtrees;
};

#endif //__TECHTREE_HPP__

