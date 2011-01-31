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
	FACTION,
	UNIT_TYPE,
};

class mgr_t;

class class_t {
public:
	virtual ~class_t();
	mgr_t& mgr;
	const class_type_t type;
	const std::string name;
protected:
	class_t(mgr_t& mgr,class_type_t type,const std::string& name);
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
	faction_t* faction();
private:
	friend class mgr_t;
	ref_t(mgr_t& mgr,class_type_t type,const std::string& name);
	class_t* get();
};

typedef std::vector<ref_t*> refs_t;

class mgr_t: public fs_handle_t {
public:
	~mgr_t();
	virtual ref_t* ref(class_type_t type,const std::string& name);
protected:
	mgr_t(fs_t& fs);
private:
	friend class ref_t;
	virtual techtree_t& techtree();
	struct pimpl_t;
	pimpl_t* pimpl;
};

#endif //__REF_HPP__


