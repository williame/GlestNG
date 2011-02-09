/*
 ref.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <map>
#include <iostream>

#include "ref.hpp"
#include "faction.hpp"
#include "resource.hpp"

struct idx_t {
	idx_t(class_type_t t,const std::string& n):
		type(t), name(n), cls(NULL), refs(1) {}
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
	class_t* attach(const ref_t& ref);
	class_t* get(const ref_t& ref);
	void detach(const ref_t& cls);
	classes_t classes;
};

ref_t::ref_t(mgr_t& mgr,class_type_t type,const std::string& name,int t):
	ok(false), tag(t) { 
	set(mgr,type,name);
}

ref_t::ref_t(): ok(false) {}

ref_t::ref_t(const ref_t& copy): ok(false) {
	if(copy.ok)
		set(*copy.mgr,copy.type,copy.name,copy.tag);
}

ref_t::~ref_t() { clear(); }

void ref_t::set(mgr_t& mgr,class_type_t type,const std::string& name) {
	if(!name.size()) panic(this << ": name is not set");
	clear();
	this->mgr = &mgr;
	this->type = type;
	this->name = name;
	ok = true;
	ptr = mgr.pimpl->attach(*this);
}

const std::string& ref_t::get_name() const {
	if(!ok) panic(this << " is not set");
	return name;
}

class_type_t ref_t::get_type() const {
	if(!ok) panic(this << " is not set");
	return type;
}

void ref_t::clear() {
	if(ok) {
		mgr->pimpl->detach(*this);
		ptr = NULL;
		mgr = NULL;
		name.clear();
		ok = false;
	}
}

faction_t& ref_t::faction() {
	if(!ok) panic(this << " is not set");
	if(type != FACTION) panic(this << " is not a faction");
	return *static_cast<faction_t*>(get());
}

resource_t& ref_t::resource() {
	if(!ok) panic(this << " is not set");
	if(type != RESOURCE) panic(this << " is not a resource");
	return *static_cast<resource_t*>(get());
}

techtree_t& ref_t::techtree() {
	if(!ok) panic(this << " is not set");
	if(type != TECHTREE) panic(this << " is not a techtree");
	techtree_t& techtree = mgr->techtree();
	if(techtree.name != name) panic(this << " does not ref "<<techtree);
	return techtree;
}

class_t* ref_t::get() {
	if(!ok) panic(this << " is not set");
	if(!ptr) ptr = mgr->pimpl->get(*this);
	return ptr;
}


class_t* mgr_t::pimpl_t::attach(const ref_t& ref) {
	if(!ref.ok) panic(ref << " is not set");
	classes_t::iterator i = classes.find(ref.name);
	if(i == classes.end()) {
		classes.insert(classes_t::value_type(ref.name,idx_t(ref.type,ref.name)));
		return NULL;
	}
	i->second.refs++;
	return i->second.cls;
}

class_t* mgr_t::pimpl_t::get(const ref_t& ref) {
	if(!ref.ok) panic(ref << " is not set");
	classes_t::iterator i = classes.find(ref.name);
	if(i == classes.end()) panic("could not find "<<ref);
	if(ref.type != i->second.type)
		data_error("trying to reference "<<ref.name<<" which is a "<<i->second.type<<" when "<<ref.type<<" was expected");
	if(!i->second.cls) { // lazy construction
		switch(i->second.type) {
		case FACTION:
			i->second.cls = new faction_t(mgr.techtree(),i->first);
			break;
		case RESOURCE:
			i->second.cls = new resource_t(mgr.techtree(),i->first);
			break;
		default: panic(ref<<" is not supported yet");
		}
	}
	return i->second.cls;
}

void mgr_t::pimpl_t::detach(const ref_t& ref) {
	if(!ref.ok) panic(ref << " is not set");
	classes_t::iterator i = classes.find(ref.name);
	if(i == classes.end()) panic("could not find "<<ref);
	if(!--i->second.refs) {
		delete i->second.cls;
		classes.erase(i);
	}
}

class_t::class_t(mgr_t& m,class_type_t t,const std::string& n):
fs_handle_t(m.fs()), mgr(m), type(t), name(n) {}

class_t::~class_t() {}

mgr_t::~mgr_t() { delete pimpl; }

mgr_t::mgr_t(fs_t& fs): fs_handle_t(fs), pimpl(new pimpl_t(*this)) {}

strings_t mgr_t::list(class_type_t type) const {
	strings_t ret;
	for(classes_t::iterator i = pimpl->classes.begin(); i != pimpl->classes.end(); i++)
		if(i->second.type == type)
			ret.push_back(i->first);
	return ret;
}

techtree_t& mgr_t::techtree() { panic("this is not a tech tree"); }




