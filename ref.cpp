/*
 ref.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <map>

#include "ref.hpp"
#include "faction.hpp"
#include "resource.hpp"

struct idx_t {
	idx_t(class_type_t t,const std::string& n):
		type(t), name(n), cls(NULL), refs(0) {}
	const class_type_t type;
	const std::string name;
	class_t* cls;
	size_t refs;
	bool operator==(const idx_t& other) const { return name == other.name; }
};

typedef std::map<std::string,idx_t> classes_t;

struct mgr_t::pimpl_t {
	pimpl_t(mgr_t& m): mgr(m) {}
	mgr_t& mgr;
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
fs_handle_t(m.fs()), mgr(m), type(t), name(n) {}

class_t::~class_t() {}

ref_t::ref_t(mgr_t& m,class_type_t t,const std::string& n):
mgr(m), type(t), name(n) {
	mgr.pimpl->get(*this).refs++;
}

ref_t::~ref_t() {
	mgr.pimpl->detach(*this);
}

class_t* ref_t::get() {
	class_t* cls = mgr.pimpl->get(*this).cls;
	if(!cls) { // lazy construction
		switch(type) {
		case FACTION:
			cls = new faction_t(mgr.techtree(),name);
			break;
		case RESOURCE:
			cls = new resource_t(mgr.techtree(),name);
			break;
		default: panic(this<<" is not supported yet");
		}
		if(!cls) panic(this<<" was not created");
		if(cls->type != type) panic(cls<<" is not "<<this);
		mgr.pimpl->get(*this).cls = cls;
	}
	return cls;
}

faction_t* ref_t::faction() {
	if(type != FACTION) data_error(this<<" is not a faction");
	return static_cast<faction_t*>(get());
}

resource_t* ref_t::resource() {
	if(type != RESOURCE) data_error(this<<" is not a resource");
	return static_cast<resource_t*>(get());
}

mgr_t::~mgr_t() { delete pimpl; }

mgr_t::mgr_t(fs_t& fs): fs_handle_t(fs), pimpl(new pimpl_t(*this)) {}

ref_t* mgr_t::ref(class_type_t type,const std::string& name) {
	return new ref_t(*this,type,name);
}

techtree_t& mgr_t::techtree() { panic("this is not a tech tree"); }




