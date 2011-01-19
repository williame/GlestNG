/*
 terrain.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __TERRAIN_HPP__
#define __TERRAIN_HPP__

#include "3d.hpp"
#include "world.hpp"
#include <vector>

struct terrain_t {
	virtual ~terrain_t() {}
	virtual void draw() = 0;
	virtual bool surface_at(const vec_t& normal,vec_t& pt) const = 0;
	struct test_t {
		test_t(const object_t* o,vec_t h): obj(o), hit(h) {}
		const object_t* obj;
		vec_t hit;
	};
	typedef std::vector<test_t> test_hits_t;
	virtual void intersection(const ray_t& r,test_hits_t& hits) const = 0;
};

terrain_t* gen_planet(size_t recursionLevel,size_t iterations,size_t smoothing_passes);

#endif //__TERRAIN_HPP__

