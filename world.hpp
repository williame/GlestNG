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

enum type_t {
	/* these denote the type of an object; each object can be only one type, but
	when querying the world for objects of particular types in a particular
	area you can | them together (so its important they are powers of 2) */
	TERRAIN = 0x01,
	BUILDING = 0x02,
	UNIT = 0x04,
	// these are masks by category
	STATIONARY = TERRAIN|BUILDING,
	MOVING = UNIT,
};

class world_t;
class spatial_index_t;

class object_t: public bounds_t {
public:
	virtual ~object_t();
	bool in_world() const { return spatial_index; }
	void bounds_reset();
	void bounds_include(const vec_t& v);
	void bounds_fix();
	virtual void draw(float d) = 0; // distance from camera
	virtual bool refine_intersection(const ray_t& r,vec_t& I) = 0;
	void move(const vec_t& relative) { set_pos(pos+relative); }
	void set_pos(const vec_t& absolute);
	vec_t get_pos() const { return pos; }
	const type_t type;
	bool is_visible() const { return visible; }
protected:
	object_t(type_t type);
private:
	friend class spatial_index_t;
	friend class world_t;
	spatial_index_t* spatial_index;
	vec_t pos;
	bounds_t bounds;
	uint8_t straddles;
	bool visible;
	void _do_set_pos(const vec_t& pos);
};

class world_t { // *the* spatial index of what is where in the world
public:
	static world_t* get_world();
	void add(object_t* obj);
	void remove(object_t* obj);
	size_t size() const;
	struct hit_t {
		hit_t(float d_,type_t t,object_t* o): d(d_),type(t),obj(o) {}
		float d; // distance from the origin (of ray or frustum)
		type_t type;
		object_t* obj;
	};
	typedef std::vector<hit_t> hits_t;
	enum sort_by_t {
		DONT_SORT,
		SORT_BY_DISTANCE,
		SORT_BY_TYPE,
		SORT_BY_TYPE_THEN_DISTANCE,
	};
	void sort(hits_t& hits,sort_by_t sort_by) const;
	void intersection(const ray_t& r,unsigned type,hits_t& hits,sort_by_t sort_by = SORT_BY_DISTANCE);
	void intersection(const frustum_t& f,unsigned type,hits_t& hits,sort_by_t sort_by = SORT_BY_TYPE_THEN_DISTANCE);
	void dump(std::ostream& out) const;
	void set_frustum(const matrix_t& projection,const matrix_t& modelview);
	intersection_t is_visible(const bounds_t& bounds) const;
	void clear_frustum();
	const hits_t& visible() const;
	bool has_frustum() const;
	const frustum_t& frustum() const;
	void check();
	vec_t unproject(const vec_t& in) const; //in.z: -1=near 1=far
private:
	friend class spatial_index_t;
	friend class object_t;
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

uint64_t high_precision_time();

// pretty printers for logs and panics and things

inline std::ostream& operator<<(std::ostream& out,type_t type) {
	out << "type<";
	switch(type) {
	case TERRAIN: out << "TERRAIN"; break;
	case BUILDING: out << "BUILDING"; break;
	case UNIT: out << "UNIT"; break;
	default: out << (int)type << "!";
	}
	out << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const world_t::hit_t& hit) {
	out << "hit<" << hit.d << "," << hit.type << "," << *hit.obj << ">";
	return out;
}

inline std::ostream& operator<<(std::ostream& out,const object_t& obj) {
	return out << "object_t<" << obj.type << "," << static_cast<const aabb_t&>(obj) << "," << obj.get_pos() << ">";
}

inline std::ostream& operator<<(std::ostream& out,const object_t* obj) {
	return out << *obj;
}

#endif //__WORLD_HPP__

