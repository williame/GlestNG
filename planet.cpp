/*
 planet.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <vector>

#include "terrain.hpp"
#include "graphics.hpp"

struct planet_t;

struct mesh_t {
	mesh_t(planet_t& planet);
};

struct planet_t: public terrain_t {
	planet_t	(int recursionLevel);
	void draw();
	bool intersection(int x,int y,vec_t& pt);
	std::vector<mesh_t> meshes;
};

planet_t::planet_t(int recursionLevel) {
	
}

void planet_t::draw() {
}

bool planet_t::intersection(int x,int y,vec_t& pt) {
	return false;
}

terrain_t* gen_planet(int recursionLevel) {
	return new planet_t(recursionLevel);
}
