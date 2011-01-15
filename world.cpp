/*
 world.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "world.hpp"

class octree_t: public bounds_t {
public:
	octree_t(const bounds_t& bounds);
	void add(world_t::category_t category,object_t* obj);
	void remove(world_t::category_t category,object_t* obj);
	void intersection(const ray_t& r,unsigned category,world_t::hits_t& hits);
private:
	struct item_t {
		item_t(world_t::category_t c,object_t* o): category(c), obj(o) {}
		world_t::category_t category;
		object_t* obj;
	};
	typedef std::vector<item_t> items_t;
	struct sub_t {
		sub_t(): sub(NULL) {}
		bounds_t bounds;
		items_t items;
		octree_t* sub;
	} sub[8];
	items_t items;
	static void intersection(items_t& items,const ray_t& r,unsigned category,world_t::hits_t& hits);
};

octree_t::octree_t(const bounds_t& bounds): bounds_t(bounds) {
	sub[0].bounds = bounds_t(a,centre);
	sub[1].bounds = bounds_t(vec_t(a.x,a.y,centre.z),vec_t(centre.x,centre.y,b.z));
	sub[2].bounds = bounds_t(vec_t(a.x,centre.y,a.z),vec_t(centre.x,b.y,centre.z));
	sub[3].bounds = bounds_t(vec_t(a.x,centre.y,centre.z),vec_t(centre.x,b.y,b.z));
	sub[4].bounds = bounds_t(vec_t(centre.x,a.y,a.z),vec_t(b.x,centre.y,centre.z));
	sub[5].bounds = bounds_t(vec_t(centre.x,a.y,centre.z),vec_t(b.x,centre.y,b.z));
	sub[6].bounds = bounds_t(vec_t(centre.x,centre.y,a.z),vec_t(b.x,b.y,centre.z));
	sub[7].bounds = bounds_t(centre,b);
}

void octree_t::add(world_t::category_t category,object_t* obj) {
	assert(ALL == intersects(*obj));
	// would fit entirely inside a child?  delegate
	for(int i=0; i<8; i++) {
		if(ALL == obj->intersects(sub[i].bounds)) {
			// TODO: threshold to break into a sub-octree
			sub[i].items.push_back(item_t(category,obj));
			return;
		}
	}
	items.push_back(item_t(category,obj));
}

void octree_t::remove(world_t::category_t category,object_t* obj) {
	assert(ALL == intersects(*obj));
}

void octree_t::intersection(const ray_t& r,unsigned category,world_t::hits_t& hits) {
	for(int i=0; i<8; i++) {
		if(sub[i].sub)
			sub[i].sub->intersection(r,category,hits);
		else
			intersection(sub[i].items,r,category,hits);
	}
	intersection(items,r,category,hits);
}

void octree_t::intersection(items_t& items,const ray_t& r,unsigned category,world_t::hits_t& hits) {
	for(items_t::iterator i=items.begin(); i!=items.end(); i++)
		if((i->category&category)&&i->obj->intersects(r))
			hits.push_back(world_t::hit_t(
				i->obj->centre.distance_sqrd(r.o),
				i->category,
				i->obj));
}

struct world_t::pimpl_t {
	pimpl_t(): idx(bounds_t(vec_t(-1,-1,-1),vec_t(1,1,1))) {}
	octree_t idx;
};

world_t* world_t::get_world() {
	static world_t* singleton = NULL;
	if(!singleton)
		singleton = new world_t();
	return singleton;
}

world_t::world_t(): pimpl(new pimpl_t()) {}

void world_t::add(category_t category,object_t* obj) {}

void world_t::remove(category_t category,object_t* obj) {}

void world_t::intersection(const ray_t& r,unsigned category,hits_t& hits) {}

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
