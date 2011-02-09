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
	unit_t(unit_type_t& type);
private:
	unit_type_t& type;
};

class unit_type_t: public class_t, public xml_loadable_t {
public:
	unit_type_t(faction_t& faction,const std::string& name);
	~unit_type_t();
	const std::string path;
private:
	void reset();
	void _load_xml(xml_parser_t::walker_t& xml);
	float size, height;
};

#endif //__UNIT_HPP__

