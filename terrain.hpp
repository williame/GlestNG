/*
 terrain.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __TERRAIN_HPP__
#define __TERRAIN_HPP__

#include "3d.hpp"

struct terrain_t {
	virtual ~terrain_t() {}
	virtual void draw() = 0;
	virtual bool intersection(int x,int y,vec_t& pt) = 0;
};

terrain_t* gen_planet(size_t recursionLevel,size_t iterations,size_t smoothing_passes);

#endif //__TERRAIN_HPP__

