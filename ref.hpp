/*
 ref.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __REF_HPP__
#define __REF_HPP__

#include <string>

#include "error.hpp"
#include "fs.hpp"

enum class_type_t {
	TECHTREE,
	FACTION,
	UNIT,
};

class mgr_t;

class class_t {
public:
	mgr_t& mgr;
	const class_type_t type;
	const std::string name;
protected:
	class_t(mgr_t& mgr,class_type_t type,const std::string& name);
private:
	friend class ref_t;
	friend class mgr_t;
	virtual ~class_t();
};

class techtree_t;
class faction_t;
class unit_type_t;

class ref_t {
public:
	virtual ~ref_t();
	mgr_t& mgr;
	const class_type_t type;
	const std::string name;
private:
	friend class mgr_t;
	ref_t(mgr_t& mgr,class_type_t type,const std::string& name);
};

class mgr_t: public fs_handle_t {
public:
	static mgr_t* create(fs_t& fs);
	virtual ~mgr_t();
	ref_t* ref(class_type_t type,const std::string& name);
private:
	friend class ref_t;
	mgr_t(fs_t& fs);
	struct pimpl_t;
	pimpl_t* pimpl;
};

#endif //__REF_HPP__


