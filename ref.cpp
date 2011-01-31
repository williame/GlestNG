/*
 ref.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <map>

#include "ref.hpp"

struct idx_t {
	idx_t(class_type_t t,const std::string& n):
		type(t), name(n), cls(NULL), refs(0) {}
	const class_type_t type;
	const std::string name;
	class_t* cls;
	size_t refs;
};

typedef std::map<std::string,idx_t> classes_t;

struct mgr_t::pimpl_t {
	idx_t& get(const ref_t& ref);
	void detach(const ref_t& cls);
	classes_t classes;
};

idx_t& mgr_t::pimpl_t::get(const ref_t& ref) {
	classes_t::iterator i = classes.find(ref.name);
	if(i == classes.end())
		return classes.insert(classes_t::value_type(ref.name,idx_t(ref.type,ref.name))).first->second;
	else if(ref.type != i->second.type)
		data_error("trying to reference "<<ref.name<<" which is a "<<i->second.type<<" when "<<ref.type<<" was expected");
	return i->second;
}

void mgr_t::pimpl_t::detach(const ref_t& ref) {
	classes_t::iterator i = classes.find(ref.name);
	if(i != classes.end() && !--i->second.refs) {
		delete i->second.cls;
		classes.erase(i);
	}
}

class_t::class_t(mgr_t& m,class_type_t t,const std::string& n):
mgr(m), type(t), name(n) {}

class_t::~class_t() {}

ref_t::ref_t(mgr_t& m,class_type_t t,const std::string& n):
mgr(m), type(t), name(n) {
	mgr.pimpl->get(*this).refs++;
}

ref_t::~ref_t() {
	mgr.pimpl->detach(*this);
}

mgr_t* mgr_t::create(fs_t& fs) { return new mgr_t(fs); }

mgr_t::~mgr_t() { delete pimpl; }

mgr_t::mgr_t(fs_t& fs): fs_handle_t(fs), pimpl(new pimpl_t()) {}

ref_t* mgr_t::ref(class_type_t type,const std::string& name) {
	return new ref_t(*this,type,name);
}


