/*
 world.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include <inttypes.h>
#include <vector>
#include <iostream>

#include "3d.hpp"

class object_t: public bounds_t {
public:
	virtual ~object_t() {}
	virtual void draw() = 0;
protected:
	object_t() {}
};

class world_t { // *the* spartial index of what is where in the world
public:
	static world_t* get_world();
	enum category_t {
		TERRAIN = 0x01,
		BUILDING = 0x02,
	};
	void add(category_t category,object_t* obj);
	void remove(category_t category,object_t* obj);
	struct hit_t {
		hit_t(float d_,category_t c,object_t* o): d(d_),category(c),obj(o) {}
		float d;
		category_t category;
		object_t* obj;
	};
	typedef std::vector<hit_t> hits_t;
	void intersection(const ray_t& r,unsigned category,hits_t& hits);
	void dump(std::ostream& out) const;
private:
	world_t();
	struct pimpl_t;
	pimpl_t* pimpl;
};

inline world_t* world() { return world_t::get_world(); }

/*** GAME TIME
 Game time is in milliseconds since game-start */

unsigned now();
void set_now(unsigned now);

class perf_t { // for fps etc
public:
	perf_t();
	void reset();
	void tick(unsigned now);
	double per_second(unsigned now) const;
private:
	enum { MAX_SECONDS = 3, NUM_SLOTS = 64 };
	unsigned slot[NUM_SLOTS];
	int idx;
};

#endif //__WORLD_HPP__

