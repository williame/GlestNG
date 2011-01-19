/*
 world.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

// ensure that NDEBUG is set when including <algorithm>, else sorts will test
// symmetry of comparators for every single swap 
#ifndef NDEBUG
//##	#define _GLESTNG_NDEBUG
//##	#define NDEBUG
#endif
#include <algorithm>
#ifdef _GLESTNG_NDEBUG
	#undef _GLESTNG_NDEBUG
	#undef NDEBUG
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "world.hpp"
#include "error.hpp"

class spartial_index_t: public bounds_t {
	/* an octree in this implementation.
	Objects are bounds rather than points.
	It consists of lists of children in each of its 8 subtrees, and those
	children that straddle more than one subtree.  Each straddling child
	is marked with a bitmask of those subtrees it straddles, to avoid
	unnecessary checks.
	*/
public:
	spartial_index_t(const bounds_t& bounds);
	void add(object_t* obj);
	void remove(object_t* obj);
	void intersection(const ray_t& r,unsigned type,world_t::hits_t& hits);
	void dump(std::ostream& out) const;
private:
	spartial_index_t(const bounds_t& bounds,spartial_index_t* parent);
	spartial_index_t* const parent;
	struct item_t {
		item_t(type_t t,object_t* o): straddles(~0), type(t), obj(o) {}
		item_t(unsigned s,type_t t,object_t* o): straddles(s), type(t), obj(o) {}
		uint8_t straddles;
		type_t type;
		object_t* obj;
	};
	enum { SPLIT = 8 }; // number of children to split
	typedef std::vector<item_t> items_t;
	struct sub_t {
		sub_t(): sub(NULL) {}
		bounds_t bounds;
		items_t items;
		spartial_index_t* sub;
	} sub[8];
	uint8_t straddlers;
	items_t items;
	void init_sub();
	static void intersection(items_t& items,const ray_t& r,unsigned type,world_t::hits_t& hits,uint8_t straddles=~0);
};

spartial_index_t::spartial_index_t(const bounds_t& bounds): bounds_t(bounds), parent(NULL), straddlers(0) {
	init_sub();
}

spartial_index_t::spartial_index_t(const bounds_t& bounds,spartial_index_t* p): bounds_t(bounds), parent(p), straddlers(0) {
	assert(p);
	init_sub();
}

void spartial_index_t::init_sub() {
	sub[0].bounds = bounds_t(a,centre);
	sub[1].bounds = bounds_t(vec_t(a.x,a.y,centre.z),vec_t(centre.x,centre.y,b.z));
	sub[2].bounds = bounds_t(vec_t(a.x,centre.y,a.z),vec_t(centre.x,b.y,centre.z));
	sub[3].bounds = bounds_t(vec_t(a.x,centre.y,centre.z),vec_t(centre.x,b.y,b.z));
	sub[4].bounds = bounds_t(vec_t(centre.x,a.y,a.z),vec_t(b.x,centre.y,centre.z));
	sub[5].bounds = bounds_t(vec_t(centre.x,a.y,centre.z),vec_t(b.x,centre.y,b.z));
	sub[6].bounds = bounds_t(vec_t(centre.x,centre.y,a.z),vec_t(b.x,b.y,centre.z));
	sub[7].bounds = bounds_t(centre,b);
#ifndef NDEBUG
	for(int i=0; i<8; i++)
		for(int j=0; j<8; j++) {
			if(j==i) continue;
			intersection_t bang = sub[i].bounds.intersects(sub[j].bounds);
			if(bang != MISS)
				panic(i<<":"<<sub[i].bounds<<" intersects "<<j<<":"<<sub[j].bounds<<" "<<bang);
		}
#endif
}

static std::ostream& indent(std::ostream& out,int depth) {
	while(depth-- > 0)
		out << "  ";
	return out;
}

static const char* fmtbin(unsigned val,int digits) {
	static char out[33];
	char *o = out;
	for(int i=0; i<digits; i++,val>>=1)
		*o++ = (val&1?'1':'0');
	*o = 0;
	return out;
}

void spartial_index_t::dump(std::ostream& out) const {
	int depth = 0;
	for(spartial_index_t* p=parent; p; p=p->parent)
		depth += 2;
	indent(out,depth) << "spartial_index_t<" << *this << ">" << std::endl;
	for(items_t::const_iterator i=items.begin(); i!=items.end(); i++)
		indent(out,depth+1) << i->type << "," << fmtbin(i->straddles,8) << "," << *i->obj << std::endl;
	for(int i=0; i<8; i++)
		if(sub[i].sub || sub[i].items.size()) {
			indent(out,depth+1) << "sub[" << i << "] " << sub[i].bounds << std::endl;
			for(items_t::const_iterator j=sub[i].items.begin(); j!=sub[i].items.end(); j++)
				indent(out,depth+2) << j->type << "," << *j->obj << std::endl;
			if(sub[i].sub)
				sub[i].sub->dump(out);
		}
}

void spartial_index_t::add(object_t* obj) {
	if(ALL != obj->intersects(*this))
		panic(*obj << " intersects " << *this << " = " << obj->intersects(*this))
	// would fit entirely inside a child?  delegate
	uint8_t s = 0;
	for(int i=0; i<8; i++)
		switch(obj->intersects(sub[i].bounds)) {
		case ALL:
			assert(!s);
			if(sub[i].sub)
				sub[i].sub->add(obj);
			else  if(sub[i].items.size() == SPLIT) {
				sub[i].sub = new spartial_index_t(sub[i].bounds,this);
				// move the items into the sub-node
				for(items_t::iterator j=sub[i].items.begin(); j!=sub[i].items.end(); j++)
					sub[i].sub->add(j->obj);
				sub[i].items.clear();
				// add the new object
				sub[i].sub->add(obj);
			} else
				// skip straddles
				sub[i].items.push_back(item_t(obj->type,obj));
			return;
		case SOME:
			s |= (1 << i);
			break;
		default:;
		}
	assert(__builtin_popcount(s) > 1);
	items.push_back(item_t(s,obj->type,obj));
	straddlers |= s;
}

void spartial_index_t::remove(object_t* obj) {
	assert(obj->spartial_index == this);
	if(ALL != obj->intersects(*this))
		panic(*obj << " intersects " << *this << " = " << obj->intersects(*this))
	for(items_t::iterator i=items.begin(); i!=items.end(); i++)
		if(i->obj == obj) {
			assert(i->type == obj->type);
			items.erase(i);
			obj->spartial_index = NULL;
			return;
		}
	panic("object could not be found in the octree");
}

void spartial_index_t::intersection(const ray_t& r,unsigned type,world_t::hits_t& hits) {
	if(parent && !intersects(r))
		panic(*this << " does not intersect " << r <<
			" (" << sphere_t::intersects(r) << "," << aabb_t::intersects(r) << ")");
	uint8_t s = 0;
	for(int i=0; i<8; i++)
		if(((straddlers&(1<<i)) || sub[i].sub || sub[i].items.size()) && sub[i].bounds.intersects(r)) {
			s |= (1 << i);
			if(sub[i].sub)
				sub[i].sub->intersection(r,type,hits);
			else
				intersection(sub[i].items,r,type,hits);
		}
	if(s&=straddlers)
		intersection(items,r,type,hits,s);
}

void spartial_index_t::intersection(items_t& items,const ray_t& r,unsigned type,world_t::hits_t& hits,uint8_t straddles) {
	for(items_t::iterator i=items.begin(); i!=items.end(); i++)
		if((i->type&type) && (i->straddles&straddles) && i->obj->intersects(r))
			hits.push_back(world_t::hit_t(
				i->obj->centre.distance_sqrd(r.o),
				i->type,
				i->obj));
}

struct world_t::pimpl_t {
	pimpl_t(): idx(bounds_t(vec_t(-1,-1,-1),vec_t(1,1,1))) {}
	spartial_index_t idx;
};

world_t* world_t::get_world() {
	static world_t* singleton = NULL;
	if(!singleton)
		singleton = new world_t();
	return singleton;
}

world_t::world_t(): pimpl(new pimpl_t()) {}

void world_t::add(object_t* obj) {
	assert(!obj->spartial_index);
	pimpl->idx.add(obj);
}

void world_t::remove(object_t* obj) {
	if(obj->spartial_index)
		obj->spartial_index->remove(obj);
}

static bool cmp_hits_distance(const world_t::hit_t& a,const world_t::hit_t& b) {
	return (a.d < b.d);
}

static bool cmp_hits_type(const world_t::hit_t& a,const world_t::hit_t& b) {
	return (a.type < b.type);
}

static bool cmp_hits_type_then_distance(const world_t::hit_t& a,const world_t::hit_t& b) {
	if(a.type == b.type)
		return (a.d < b.d);
	return (a.type < b.type);
}

void world_t::intersection(const ray_t& r,unsigned type,hits_t& hits,sort_by_t sort_by) {
	pimpl->idx.intersection(r,type,hits);
	bool (*func)(const world_t::hit_t& a,const world_t::hit_t& b);
	switch(sort_by) {
	case SORT_BY_DISTANCE:
		func = cmp_hits_distance;
		break;
	case SORT_BY_TYPE:
		func = cmp_hits_type;
		break;
	case SORT_BY_TYPE_THEN_DISTANCE:
		func = cmp_hits_type_then_distance;
		break;
	default:
		assert(sort_by == DONT_SORT);
		return;
	};
	std::sort(hits.begin(),hits.end(),func);
}

void world_t::dump(std::ostream& out) const {
	pimpl->idx.dump(out);
}

object_t::object_t(type_t t): type(t), spartial_index(NULL) {}

object_t::~object_t() {
	if(spartial_index)
		spartial_index->remove(this);
}

perf_t::perf_t() {
	reset();
}

void perf_t::reset() {
	for(int i=0; i<NUM_SLOTS; i++)
		slot[i] = 0;
	idx = 0;
}

void perf_t::tick(unsigned now) {
	if(++idx == NUM_SLOTS)
		idx = 0;
	slot[idx] = now;
}
	
double perf_t::per_second(unsigned now) const {
	enum { WINDOW = MAX_SECONDS*1000 };
	const unsigned first = slot[idx<(NUM_SLOTS-1)?idx+1:0];
	const unsigned len = (now-first);
	int count = 0;
	for(int i=0; i<NUM_SLOTS; i++)
		if(slot[i])
			count++;
	const double multi = len/1000.0;
	return count/multi;
}

static unsigned _now;

unsigned now() { return _now; }

void set_now(unsigned now) {
	_now = now;
}

#ifdef __WIN32
	#include <windows.h>
	uint64_t high_precision_time() {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		static __int64 base;
		static double freq;
		static bool inited = false;
		if(!inited) {
			base = now.QuadPart; 
			LARGE_INTEGER li;
			QueryPerformanceFrequency(&li);
			freq = (double)li.QuadPart/1000000000;
			std::cout << "FREQ "<<li.QuadPart<<","<<freq<<std::endl;
			inited = true;
		}
		std::cout<<"NOW "<<(now.QuadPart-base)<<","<<(now.QuadPart-base)/freq<<std::endl;
		return (now.QuadPart-base)/freq;	
	}
#else	
	#include <sys/time.h>
	uint64_t high_precision_time() {
		static uint64_t base = 0;
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC,&ts);
		if(!base)
			base = ts.tv_sec;
		return (ts.tv_sec-base)*1000000000+ts.tv_nsec;
	}
#endif

