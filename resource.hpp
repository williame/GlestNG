/*
 resource.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __RESOURCE_HPP__
#define __RESOURCE_HPP__

#include "techtree.hpp"

class resource_t: public class_t, public xml_loadable_t {
public:
	resource_t(techtree_t& techtree,const std::string& name);
	const std::string path;
	enum resource_type_t {
		STATIC,
		CONSUMABLE,
	};
	resource_type_t resource_type() const { return _resource_type; }
	int interval() const;
private:
	~resource_t();
	void reset();
	void _load_xml(xml_parser_t::walker_t& xml);
	ref_t* _image;
	resource_type_t _resource_type;
	int _interval;
};

#endif // __RESOURCE_HPP__
          

