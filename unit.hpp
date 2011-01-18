/*
 unit.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __UNIT_HPP__
#define __UNIT_HPP__

#include <string>

#include "xml.hpp"

class unit_type_t {
public:
	unit_type_t(const char* name,const char* xml);
	void load_xml();
	const std::string& get_name() const { return name; }
	void describe_xml(std::ostream& out);
private:
	std::string name;
	xml_parser_t xml;
	float size, height;
};

#endif //__UNIT_HPP__

