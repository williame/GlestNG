/*
 resource.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "resource.hpp"

resource_t::resource_t(techtree_t& techtree,const std::string& name):
	class_t(techtree,RESOURCE,name), xml_loadable_t(name),
	path(techtree.fs().canocial(techtree.path+"/resources/"+name))
{
	fs_file_t::ptr_t f(fs().get(path+"/"+name+".xml"));
	istream_t::ptr_t in(f->reader());
	load_xml(*in);	
}
	
resource_t::~resource_t() { reset(); }

void resource_t::reset() {
	_image.clear();
}

void resource_t::_load_xml(xml_parser_t::walker_t& xml) {
	xml.check("resource");
	_image.set(mgr,IMAGE,fs().join(path,xml.get_child("image").value_string("path")));
	const std::string type = xml.up().get_child("type").value_string();
	if(type=="consumable") {
		_resource_type = CONSUMABLE;
		_interval = xml.get_child("interval").value_int();
		xml.up();
	} else if(type=="static")
		_resource_type = STATIC;
	xml.up();
}

int resource_t::interval() const {
	if(resource_type()!=CONSUMABLE) panic(this<<" is not consumable");
	return _interval;
}

          

