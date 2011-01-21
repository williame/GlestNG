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

class spatial_index_t: public bounds_t {
	/* an octree in this implementation.
	Objects are bounds rather than points.
	It consists of lists of children in each of its 8 subtrees, and those
	children that straddle more than one subtree.  Each straddling child
	is marked with a bitmask of those subtrees it straddles, to avoid
	unnecessary checks.
	*** It currently walks through the bounding boxes recursively, whereas it could use breshenham or
	at least derive the subdivisions based upon the intersection of the centred XY XZ YZ planes
	instead of treating each box individually.
	*/
public:
	spatial_index_t(const bounds_t& bounds);
	void add(object_t* obj);
	void remove(object_t* obj);
	void intersection(const ray_t& r,unsigned type,world_t::hits_t& hits) const;
	void intersection(const frustum_t& f,unsigned type,world_t::hits_t& hits,bool world_frustum) const;
	void dump(std::ostream& out) const;
	bool moves(const object_t* obj,const vec_t& relative) const;
	void add_all(const vec_t& origin,unsigned type,world_t::hits_t& hits,bool world_frustum) const;
	void clear_frustum();
private:
	spatial_index_t(const bounds_t& bounds,spatial_index_t* parent);
	spatial_index_t* const parent;
	struct item_t {
		item_t(uint8_t s,type_t t,object_t* o): straddles(s), type(t), obj(o) {}
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
		spatial_index_t* sub;
	} sub[8];
	uint8_t straddlers;
	mutable uint8_t frustum;
	items_t items;
	void init_sub();
	static void intersection(const items_t& items,const ray_t& r,unsigned type,world_t::hits_t& hits,uint8_t straddles=~0);
	static void intersection(const items_t& items,const frustum_t& f,unsigned type,world_t::hits_t& hits,uint8_t straddles=~0);
};

struct world_t::pimpl_t {
	pimpl_t(): idx(bounds_t(vec_t(-1,-1,-1),vec_t(1,1,1))), has_frustum(false) {}
	void add_visible(object_t* obj);
	void remove_visible(object_t* obj);
	spatial_index_t idx;
	bool has_frustum;
	frustum_t frustum;
	hits_t visible;
	int visible_dirty; // index of first unsorted entry; consider tombstones and unsorted part
};

spatial_index_t::spatial_index_t(const bounds_t& bounds):
	bounds_t(bounds), parent(NULL), straddlers(0), frustum(0) {
	init_sub();
}

spatial_index_t::spatial_index_t(const bounds_t& bounds,spatial_index_t* p):
	bounds_t(bounds), parent(p), straddlers(0), frustum(0) {
	assert(p);
	init_sub();
}

void spatial_index_t::init_sub() {
	sub[0].bounds = bounds_t(a,centre);
	sub[1].bounds = bounds_t(vec_t(a.x,a.y,centre.z),vec_t(centre.x,centre.y,b.z));
	sub[2].bounds = bounds_t(vec_t(a.x,centre.y,a.z),vec_t(centre.x,b.y,centre.z));
	sub[3].bounds = bounds_t(vec_t(a.x,centre.y,centre.z),vec_t(centre.x,b.y,b.z));
	sub[4].bounds = bounds_t(vec_t(centre.x,a.y,a.z),vec_t(b.x,centre.y,centre.z));
	sub[5].bounds = bounds_t(vec_t(centre.x,a.y,centre.z),vec_t(b.x,centre.y,b.z));
	sub[6].bounds = bounds_t(vec_t(centre.x,centre.y,a.z),vec_t(b.x,b.y,centre.z));
	sub[7].bounds = bounds_t(centre,b);
#if 0 //ndef NDEBUG
	for(int i=0; i<8; i++) {
		intersection_t bang = sub[i].bounds.intersects(*this);
		if(bang != ALL)
			panic(i<<":"<<sub[i].bounds<<" intersects "<<*this<<" "<<bang);
		for(int j=0; j<8; j++) {
			if(j==i) continue;
			bang = sub[i].bounds.intersects(sub[j].bounds);
			if(bang != MISS)
				panic(i<<":"<<sub[i].bounds<<" intersects "<<j<<":"<<sub[j].bounds<<" "<<bang);
		}
	}
#endif
}

static std::ostream& indent(std::ostream& out,int depth) {
	while(depth-- > 0)
		out << "  ";
	return out;
}

static std::string fmtbin(unsigned val,int digits) {
	static char out[65];
	char *o = out;
	for(int i=0; i<digits; i++,val>>=1)
		*o++ = (val&1?'1':'0');
	*o = 0;
	return out;
}

void spatial_index_t::dump(std::ostream& out) const {
	int depth = 0;
	for(spatial_index_t* p=parent; p; p=p->parent)
		depth += 2;
	indent(out,depth) << "spatial_index_t<" << *this << ">" << std::endl;
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

void spatial_index_t::add(object_t* obj) {
	const bounds_t bounds = obj->pos_bounds();
	if(ALL != bounds.intersects(*this)) {
		if(!parent)
			panic(*obj << "(" << obj->pos << "," << bounds << ") intersects " << *this << " = " << bounds.intersects(*this))
		parent->add(obj);
		return;
	}
	// would fit entirely inside a child?  delegate
	obj->straddles = 0;
	for(int i=0; i<8; i++)
		switch(bounds.intersects(sub[i].bounds)) {
		case ALL:
			assert(!obj->straddles);
			if(sub[i].sub)
				sub[i].sub->add(obj);
			else  if(sub[i].items.size() == SPLIT) {
				sub[i].sub = new spatial_index_t(sub[i].bounds,this);
				// move the items into the sub-node
				for(items_t::iterator j=sub[i].items.begin(); j!=sub[i].items.end(); j++)
					sub[i].sub->add(j->obj);
				sub[i].items.clear();
				// add the new object
				sub[i].sub->add(obj);
				// frustum flags for new sub
				if(frustum & (1<<i)) {
					for(int j=0; j<8; j++)
						if(world()->frustum().contains(sub[i].sub->sub[j].bounds))
							sub[i].sub->frustum |= (1 << j);
				}
			} else {
				if(!sub[i].items.size() && (~frustum & (1 << i)) && world()->has_frustum()) {
					if(world()->frustum().contains(sub[i].bounds))
						frustum |= (1 << i);
				}
				obj->straddles |= (1 << i);
				sub[i].items.push_back(item_t(obj->straddles,obj->type,obj));
				obj->spatial_index = this;
				if(frustum & (1<<i))
					world()->pimpl->add_visible(obj);
			}
			return;
		case SOME:
			obj->straddles |= (1 << i);
			break;
		default:;
		}
	if(__builtin_popcount(obj->straddles) < 2) {
		for(int i=0; i<8; i++)
			std::cerr << i << ": " << sub[i].bounds << " = " << bounds.intersects(sub[i].bounds) << std::endl;
		panic(obj<<"does not straddle "<<*this<<" ("<<fmtbin(obj->straddles,8)<<")");
	}
	items.push_back(item_t(obj->straddles,obj->type,obj));
	obj->spatial_index = this;
	straddlers |= obj->straddles;
	if(frustum & obj->straddles)
		world()->pimpl->add_visible(obj);
}

void spatial_index_t::remove(object_t* obj) {
	assert(obj->spatial_index == this);
	const bounds_t bounds = obj->pos_bounds();
	if(ALL != bounds.intersects(*this))
		panic(obj << " intersects " << *this << " = " << bounds.intersects(*this))
	assert(obj->straddles);
	items_t& items = (
		__builtin_popcount(obj->straddles)>1? 
		this->items:
		sub[__builtin_ffs(obj->straddles)-1].items);
	if(obj->straddles & frustum) // possibly visible?
		world()->pimpl->remove_visible(obj);
	for(items_t::iterator i=items.begin(); i!=items.end(); i++)
		if(i->obj == obj) {
			assert(i->type == obj->type);
			items.erase(i);
			obj->spatial_index = NULL;
			return;
		}
	panic("object could not be found in the octree");
}

bool spatial_index_t::moves(const object_t* obj,const vec_t& relative) const {
	assert(obj->spatial_index == this);
	assert(obj->straddles);
	const bounds_t bounds = obj->pos_bounds()+relative;
	if(__builtin_popcount(obj->straddles)>1) {
		if(ALL != bounds.intersects(*this)) return true;
		for(int i=0; i<8; i++)
			if(SOME == bounds.intersects(sub[i].bounds)) {
				if(~obj->straddles & (1<<i)) return true;
			} else {
				if(obj->straddles & (1<<i)) return true;
			}
		return false;
	} else {
		const int i = __builtin_ffs(obj->straddles)-1;
		return (bounds.intersects(sub[i].bounds) != ALL);
	}
}	

void spatial_index_t::intersection(const ray_t& r,unsigned type,world_t::hits_t& hits) const {
	if(!intersects(r)) {
		if(parent)
			panic(*this << " does not intersect " << r <<
				" (" << sphere_t::intersects(r) << "," << aabb_t::intersects(r) << ")");
		return;
	}
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

void spatial_index_t::intersection(const items_t& items,const ray_t& r,unsigned type,world_t::hits_t& hits,uint8_t straddles) {
	for(items_t::const_iterator i=items.begin(); i!=items.end(); i++)
		if((i->type&type) && (i->straddles&straddles) && i->obj->intersects(r))
			hits.push_back(world_t::hit_t(
				i->obj->centre.distance_sqrd(r.o),
				i->type,
				i->obj));
}

void spatial_index_t::intersection(const items_t& items,const frustum_t& f,unsigned type,world_t::hits_t& hits,uint8_t straddles) {
	for(items_t::const_iterator i=items.begin(); i!=items.end(); i++)
		if((i->type&type) && (i->straddles&straddles)) {
			double d;
			if(MISS != f.contains(*i->obj,d))
				hits.push_back(world_t::hit_t(sqrd(d),i->type,i->obj));
		}
}

void spatial_index_t::intersection(const frustum_t& f,unsigned type,world_t::hits_t& hits,bool world_frustum) const {
	switch(f.contains(*this)) {
	case ALL:
		add_all(vec_t(0,0,-1),type,hits,world_frustum);
		return;
	case SOME:
		break;
	case MISS:
		if(parent) panic(*this << " does not intersect frustum");
		return;
	}
	uint8_t s = 0;
	for(int i=0; i<8; i++)
		if(f.contains(sub[i].bounds)) {
			s |= (1 << i);
			if(sub[i].sub)
				sub[i].sub->intersection(f,type,hits,world_frustum);
			else
				intersection(sub[i].items,f,type,hits);
		}
	if(s&=straddlers)
		intersection(items,f,type,hits,s);
	if(world_frustum)
		frustum = s;
}

void spatial_index_t::add_all(const vec_t& origin,unsigned type,world_t::hits_t& hits,bool world_frustum) const {
	if(world_frustum)
		frustum = ~0;
	for(int s=0; s<8; s++)
		if(sub[s].sub)
			sub[s].sub->add_all(origin,type,hits,world_frustum);
		else
			for(items_t::const_iterator i=sub[s].items.begin(); i!=sub[s].items.end(); i++)
				if(i->obj->type & type) {
					float d = origin.distance_sqrd(i->obj->centre);
					hits.push_back(world_t::hit_t(
						d,
						i->obj->type,
						i->obj));
				}
	for(items_t::const_iterator i=items.begin(); i!=items.end(); i++)
		if(i->obj->type & type) {
			float d = origin.distance_sqrd(i->obj->centre);
			hits.push_back(world_t::hit_t(
				d,
				i->obj->type,
				i->obj));
		}
}

void spatial_index_t::clear_frustum() {
	if(!frustum) {
		if(parent) panic(*this << " cannot clear the frustum when I\'m not visible");
		return;
	}
	for(int s=0; s<8; s++)
		if(sub[s].sub && (frustum&(1<<s)))
			sub[s].sub->clear_frustum();
	frustum = 0;
}

void world_t::pimpl_t::add_visible(object_t* obj) {
	double d;
	const bool is_vis = world()->frustum().contains(*obj,d);
	//### optimise with visible_dirty
	for(size_t i=0; i<visible.size(); i++)
		if(visible[i].obj == obj) {
			if(!is_vis)
				visible.erase(visible.begin()+i);
			return;
		}
	if(is_vis)
		visible.push_back(hit_t(d,obj->type,obj));
}

void world_t::pimpl_t::remove_visible(object_t* obj) {
	for(size_t i=0; i<visible.size(); i++)
		if(visible[i].obj == obj) {
			visible.erase(visible.begin()+i);
			return;
		}
}

world_t* world_t::get_world() {
	static world_t* singleton = NULL;
	if(!singleton)
		singleton = new world_t();
	return singleton;
}

world_t::world_t(): pimpl(new pimpl_t()) {}

void world_t::add(object_t* obj) {
	assert(!obj->spatial_index);
	pimpl->idx.add(obj);
}

void world_t::remove(object_t* obj) {
	if(obj->spatial_index)
		obj->spatial_index->remove(obj);
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

void world_t::sort(hits_t& hits,sort_by_t sort_by) const {
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


void world_t::intersection(const ray_t& r,unsigned type,hits_t& hits,sort_by_t sort_by) {
	pimpl->idx.intersection(r,type,hits);
	sort(hits,sort_by);
}

void world_t::intersection(const frustum_t& f,unsigned type,hits_t& hits,sort_by_t sort_by) {
	pimpl->idx.intersection(f,type,hits,false);
	sort(hits,sort_by);
}

void world_t::dump(std::ostream& out) const {
	pimpl->idx.dump(out);
}

void world_t::set_frustum(const matrix_t& projection,const matrix_t& modelview) {
	clear_frustum();
	pimpl->has_frustum = true;
	pimpl->frustum = frustum_t(projection,modelview);
	pimpl->idx.intersection(pimpl->frustum,~0,pimpl->visible,true);
	pimpl->visible_dirty = pimpl->visible.size()-1;
}

void world_t::clear_frustum() {
	if(pimpl->has_frustum) {
		pimpl->idx.clear_frustum();
		pimpl->visible.clear();
		pimpl->has_frustum = false;
	}
}

bool world_t::has_frustum() const {
	return pimpl->has_frustum;
}

const frustum_t& world_t::frustum() const {
	if(!has_frustum()) panic("there is no frustum set on the world");
	return pimpl->frustum;
}


const world_t::hits_t& world_t::visible() const {
	if(!has_frustum()) panic("there is no frustum set on the world");
	if(pimpl->visible_dirty) {
		//### optimisation: could only sort the dirty tail
		sort(pimpl->visible,SORT_BY_TYPE_THEN_DISTANCE);
		pimpl->visible_dirty = -1;
	}
	return pimpl->visible;
}

object_t::object_t(type_t t): type(t), spatial_index(NULL), pos(0,0,0) {}

object_t::~object_t() {
	if(spatial_index)
		spatial_index->remove(this);
}

void object_t::set_pos(const vec_t& absolute) {
	if(spatial_index && spatial_index->moves(this,absolute-pos)) {
		// remember old values
		const vec_t old_pos = pos;
		spatial_index_t* const idx = spatial_index;
		try {
			// see if it succeeds
			idx->remove(this);
			pos = absolute;
			idx->add(this);
		} catch(...) {
			// restore and throw error
			pos = old_pos;
			idx->add(this);
			throw;
		}
	} else
		pos = absolute;
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

