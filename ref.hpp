/*
 ref.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __REF_HPP__
#define __REF_HPP__

#include <string>
#include <vector>

#include "error.hpp"
#include "fs.hpp"

enum class_type_t {
	TECHTREE,
	FACTION,
	UNIT_TYPE,
	RESOURCE,
	IMAGE,
	MODEL,
	PARTICLE,
};

class techtree_t;
class faction_t;
class resource_t;
class unit_type_t;

class mgr_t;

class class_t: public fs_handle_t {
public:
	virtual ~class_t();
	mgr_t& mgr;
	const class_type_t type;
	const std::string name;
protected:
	class_t(mgr_t& mgr,class_type_t type,const std::string& name);
};

class mgr_t: public fs_handle_t {
public:
	~mgr_t();
	strings_t list(class_type_t type) const;
protected:
	mgr_t(fs_t& fs);
private:
	friend class ref_t;
	virtual techtree_t& techtree();
	struct pimpl_t;
	pimpl_t* pimpl;
};

class ref_t {
public:
	ref_t(mgr_t& mgr,class_type_t type,const std::string& name,int tag=0);
	ref_t();
	ref_t(const ref_t& copy);
	~ref_t();
	ref_t& operator=(const ref_t& copy);
	void set(mgr_t& mgr,class_type_t type,const std::string& name);
	void set(mgr_t& mgr,class_type_t type,const std::string& name,int tag) {
		set(mgr,type,name); set_tag(tag);
	}
	void set_tag(int tag) { this->tag = tag; }
	int get_tag() const { return tag; }
	bool is_set() const { return ok; }
	const std::string& get_name() const;
	class_type_t get_type() const;
	void clear();
	techtree_t& techtree();
	faction_t& faction();
	resource_t& resource();
private:
	friend class mgr_t::pimpl_t;
	bool ok;
	int tag;
	mgr_t* mgr;
	class_t* ptr;
	class_type_t type;
	std::string name;
	class_t* get();
};

typedef std::vector<ref_t> refs_t;

inline std::ostream& operator<<(std::ostream& out,class_type_t type) {
	switch(type) {
	case TECHTREE: return out << "TECHTREE";
	case FACTION: return out << "FACTION";
	case UNIT_TYPE: return out << "UNIT_TYPE";
	case RESOURCE: return out << "RESOURCE";
	case IMAGE: return out << "IMAGE";
	case MODEL: return out << "MODEL";
	case PARTICLE: return out << "PARTICLE";
	default: return out << "class_type<"<<(int)type<<'>';
	}
}

inline std::ostream& operator<<(std::ostream& out,const class_t& cls) {
	return out << "class<"<<cls.type<<','<<cls.name<<'>';
}

inline std::ostream& operator<<(std::ostream& out,const class_t* cls) {
	return out << *cls;
}

inline std::ostream& operator<<(std::ostream& out,const ref_t& ref) {
	out << "ref<";
	if(ref.is_set()) out << ref.get_type() << ',' << ref.get_name();
	else out << '?';
	return out << '>';
}
inline std::ostream& operator<<(std::ostream& out,const ref_t* ref) {
	return out << *ref;
}

#endif //__REF_HPP__


