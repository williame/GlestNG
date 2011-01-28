/*
 unit.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>

#include "unit.hpp"
#include "xml.hpp"

unit_type_t::unit_type_t(const std::string& name):
	xml_loadable_t(name)
{}

unit_type_t::~unit_type_t() {}

void unit_type_t::_load_xml(xml_parser_t::walker_t& xml) {
	//### release any existing assets
	xml.check("unit");
	height = xml.get_child("parameters").get_child("height").value_float();
	size = xml.up().get_child("size").value_float();
	//### go through the units
}

