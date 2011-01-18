/*
 unit.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "unit.hpp"
#include "xml.hpp"

unit_type_t::unit_type_t(const char* n,const char* x): name(n), xml(n,x) {}

void unit_type_t::load_xml() {
	xml_parser_t::walker_t xml = this->xml.walker();
	xml.check("unit");
	height = xml.get_child("parameters").get_child("height").value_float();
	size = xml.up().get_child("size").value_float();
}

void unit_type_t::describe_xml(std::ostream& out) {
	xml.describe_xml(out);
}
