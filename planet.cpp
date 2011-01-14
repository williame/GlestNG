/*
 planet.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <vector>
#include <map>
#include <algorithm>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <iostream>

#include "memcheck.h"
#include "terrain.hpp"
#include "graphics.hpp"
#include "world.hpp"

enum {
	DIVIDE_THRESHOLD = 4,
	FACE_IDX = 18, // how much to shift to get the ID of the mesh the face belongs to?
	FACE_MASK = (1<<FACE_IDX)-1,
};

struct face_t {
	face_t() {}
	face_t(GLuint a_,GLuint b_,GLuint c_): a(a_), b(b_), c(c_) {}
	GLuint a,b,c;
};

struct rgb_t {
	rgb_t() {}
	rgb_t(GLubyte r_,GLubyte g_,GLubyte b_): r(r_), g(g_), b(b_) {}
	GLubyte r,g,b;
};

struct planet_t;

struct mesh_t {
	mesh_t(planet_t& planet,face_t tri,size_t recursionLevel);
	void draw();
	planet_t& planet;
	const GLuint ID;
	bounds_t bounds;
	GLuint mn_point, mx_point;
	size_t start, stop;
};

struct planet_t: public terrain_t {
	planet_t	(size_t recursionLevel,size_t iterations,size_t smoothing_passes);
	~planet_t();
	static size_t num_points(size_t recursionLevel);
	static size_t num_faces(size_t recursionLevel);
	GLuint midpoint(GLuint a,GLuint b);
	GLuint find_face(GLuint a,GLuint b,GLuint c);
	void draw();
	void divide(const face_t& tri,size_t recursionLevel,size_t depth);
	void gen(size_t iterations,size_t smoothing_passes);
	bool intersection(int x,int y,vec_t& pt);
	typedef std::map<uint64_t,GLuint> midpoints_t;
	midpoints_t midpoints;
	typedef std::vector<mesh_t*> meshes_t;
	meshes_t meshes;
	fixed_array_t<vec_t> points;
	fixed_array_t<vec_t> normals;
	fixed_array_t<rgb_t> colours;
	fixed_array_t<face_t> faces;
	struct adjacent_t {
		adjacent_t();
		size_t size() const;
		GLuint adj[6];
		void add(GLuint neighbour);
		adjacent_t& operator-=(adjacent_t a);
		static const GLuint EMPTY = ~0; 
	};
	fixed_array_t<adjacent_t> adjacent_faces;
	fixed_array_t<adjacent_t> adjacent_points;
	static const float
		WATER_LEVEL = 0.85f,
		MOUNTAIN_LEVEL = 0.95f;
	enum type_t {
		WATER,
		ICE,
		LAND,
		MOUNTAIN,
		ICE_FLOW,
	};
	fixed_array_t<type_t> types;
	vec_t sun;
};


mesh_t::mesh_t(planet_t& p,face_t tri,size_t recursionLevel):
	planet(p),
	ID(p.meshes.size()<<FACE_IDX),
	mn_point(~0), mx_point(0)
{
	start = stop = planet.faces.append(tri);
	for(size_t i=0; i<=recursionLevel; i++) {
		const size_t prev = stop;
		for(size_t j=start; j<=prev; j++) {
			const face_t& f = planet.faces[j];
			const GLuint
				a = planet.midpoint(f.a,f.b),
				b = planet.midpoint(f.b,f.c),
				c = planet.midpoint(f.c,f.a);
			planet.faces.append(face_t(f.a,a,c));
			planet.faces.append(face_t(f.b,b,a));
			stop = planet.faces.append(face_t(f.c,c,b));
			planet.faces[j] = face_t(a,b,c);
		}
	}
	for(size_t i=start; i<=stop; i++) {
		const face_t& f = planet.faces[i];
		const GLuint a = f.a, b = f.b, c = f.c;
		mn_point = std::min<GLuint>(mn_point,std::min<GLuint>(a,std::min<GLuint>(b,c)));
		mx_point = std::max<GLuint>(mx_point,std::max<GLuint>(a,std::max<GLuint>(b,c)));
		planet.adjacent_faces[a].add(ID|i);
		planet.adjacent_faces[b].add(ID|i);
		planet.adjacent_faces[c].add(ID|i);
		assert(planet.find_face(a,b,c)==(ID|i));
		planet.adjacent_points[a].add(b);
		planet.adjacent_points[b].add(a);
		planet.adjacent_points[a].add(c);
		planet.adjacent_points[c].add(a);
		planet.adjacent_points[b].add(c);
		planet.adjacent_points[c].add(b);
	}
}

static void _vertex(const rgb_t& c,const vec_t& n,const vec_t& v) {
	glColor3ub(c.r,c.g,c.b);
	glNormal3f(n.x,n.y,n.z);
	glVertex3f(v.x,v.y,v.z);
}

void mesh_t::draw() {
	glColor3f(1.,0,0);
	glBegin(GL_TRIANGLES);
	for(size_t i=start; i<=stop; i++) {
		const face_t& f = planet.faces[i];
		_vertex(planet.colours[f.a],planet.normals[f.a],planet.points[f.a]);
		_vertex(planet.colours[f.b],planet.normals[f.b],planet.points[f.b]);
		_vertex(planet.colours[f.c],planet.normals[f.c],planet.points[f.c]);
	}
	glEnd();
}

std::ostream& operator<<(std::ostream& out,const planet_t::adjacent_t& adj) {
	out << "[";
	for(int i=0; i<6; i++) {
		if(adj.adj[i]!=planet_t::adjacent_t::EMPTY)
			out << (unsigned)adj.adj[i];
		if(i<5)
			out << ",";
	}
	out << "]";
	return out;
}

planet_t::adjacent_t::adjacent_t() {
	for(int i=0; i<6; i++)
		adj[i] = EMPTY;
}

void planet_t::adjacent_t::add(GLuint neighbour) {
	for(int i=0; i<6; i++) {
		if(adj[i] == neighbour)
			return;
		if(adj[i] == EMPTY) {
			adj[i] = neighbour;
			return;
		}
	}
	std::cout << "cannot add " << neighbour << " to " << *this << std::endl;
	assert(false);
}

size_t planet_t::adjacent_t::size() const {
	for(int i=5; i>=0; i++)
		if(adj[i] != EMPTY)
			return i+1;
	return 0;
}

planet_t::adjacent_t& planet_t::adjacent_t::operator-=(adjacent_t a) {
	unsigned intersection = 0, used = 0;
	for(int i=0; i<6; i++) {
		if(adj[i] == EMPTY)
			break;
		for(int j=0; j<6; j++) {
			if(a.adj[j] == EMPTY)
				break;
			if(a.adj[j] == adj[i]) {
				assert(~used&(1<<j));
				used |= (1<<j);
				intersection |= (1<<i);
				break;
			}
		}
	}
	if(intersection) {
		for(int i=0; i<6; i++) {
			if(intersection&(1<<i)) {
				intersection &= ~(1<<i);
				for(int j=0; j<i; j++) {
					if(adj[j] == EMPTY) {
						adj[j] = adj[i];
						adj[i] = EMPTY;
						break;
					}
				}
			} else
				adj[i] = EMPTY;
		}
	} else {
		for(int i=0; i<6; i++) {
			if(adj[i] == EMPTY)
				break;
			adj[i] = EMPTY;
		}
	}
	return *this;
}

size_t planet_t::num_points(size_t recursionLevel) {
	assert(recursionLevel);
	return (5*pow(2,2*recursionLevel+3)+2);
}

size_t planet_t::num_faces(size_t recursionLevel) {
	return (20*pow(4,recursionLevel+1));
}

planet_t::planet_t(size_t recursionLevel,size_t iterations,size_t smoothing_passes):
	points(num_points(recursionLevel)),
	normals(num_points(recursionLevel)),
	colours(num_points(recursionLevel)),
	faces(num_faces(recursionLevel)),
	adjacent_faces(num_points(recursionLevel),true),
	adjacent_points(num_points(recursionLevel),true),
	types(num_points(recursionLevel)),
	sun(0,0,100)
{
	std::cout << "terraforming...\n: recursionLevel = " << recursionLevel << std::endl;
	static const float t = (1.0f + sqrt(5.0f)) / 2.0f;
	static const vec_t Ts[12] = {
		vec_t(-1, t, 0),vec_t( 1, t, 0),vec_t(-1,-t, 0),vec_t( 1,-t, 0),
		vec_t( 0,-1, t),vec_t( 0, 1, t),vec_t( 0,-1,-t),vec_t( 0, 1,-t),
        		vec_t( t, 0,-1),vec_t( t, 0, 1),vec_t(-t, 0,-1),vec_t(-t, 0, 1)};
        for(int p=0; p<12; p++)
		points.append(vec_t::normalise(Ts[p]));
	static const face_t Fs[20] = {
            face_t(0,11,5),face_t(0,5,1),face_t(0,1,7),face_t(0,7,10),face_t(0,10,11),
            face_t(1,5,9),face_t(5,11,4),face_t(11,10,2),face_t(10,7,6),face_t(7,1,8),
            face_t(3,9,4),face_t(3,4,2),face_t(3,2,6),face_t(3,6,8),face_t(3,8,9),
            face_t(4,9,5),face_t(2,4,11),face_t(6,2,10),face_t(8,6,7),face_t(9,8,1)};
        for(int f=0; f<20; f++)
        		divide(Fs[f],recursionLevel,0);
	std::cout << ": " << points.size() << " points, "
		<< meshes.size() << " meshes, "
		<< faces.size() << " faces"<< std::endl;
	assert(points.full());
        	assert(faces.full());
	gen(iterations,smoothing_passes);
	normals.fill(vec_t(0,0,0));
	for(size_t i=0; i<faces.size(); i++) {
		const face_t& f = faces[i];
		const vec_t a = points[f.c]-points[f.b];
		const vec_t b = points[f.a]-points[f.b];
		const vec_t pn = a.cross(b).normalise();
		normals[f.a] += pn;
		normals[f.b] += pn;
		normals[f.c] += pn;
	}
	for(size_t i=0; i<normals.size(); i++) {
		assert(adjacent_faces[i].size() >= 5);
		normals[i] /= adjacent_faces[i].size();
		normals[i].normalise();
	}
}

planet_t::~planet_t() {
	for(int i=meshes.size()-1; i>=0; i--)
		delete meshes[i];
}

void planet_t::divide(const face_t& tri,size_t recursionLevel,size_t depth) {
	if ((recursionLevel-depth) < DIVIDE_THRESHOLD)
		meshes.push_back(new mesh_t(*this,tri,recursionLevel-depth));
	else {
		depth++;
		const GLuint
			a = midpoint(tri.a,tri.b),
			b = midpoint(tri.b,tri.c),
			c = midpoint(tri.c,tri.a);
		divide(face_t(tri.a,a,c),recursionLevel,depth);
		divide(face_t(tri.b,b,a),recursionLevel,depth);
		divide(face_t(tri.c,c,b),recursionLevel,depth);
		divide(face_t(a,b,c),recursionLevel,depth);
	}
}

static float randf() {
	return (float)rand()/RAND_MAX;
}

void planet_t::gen(size_t iterations,size_t smoothing_passes) {
        // http://freespace.virgin.net/hugo.elias/models/m_landsp.htm
        std::cout << ": generating landscape with " << iterations << " iterations" << std::endl;
        srand(time(NULL));
        vec_t n, v;
        fixed_array_t<float> adj(points.size());
        adj.fill(0);
        for(size_t i=0; i<iterations; i++) {
        	do {
			n.x = (randf()-0.5f)*2.0f;
			n.y = (randf()-0.5f)*2.0f;
			n.z = (randf()-0.5f)*2.0f;
		} while(n.magnitude_sqrd() <= 0.0f);
		const int m = (randf() > 0.7)? -1: 1;
		for(size_t p=0; p<points.size(); p++) {
			v = points[p];
			v -= n;
			if(v.dot(n) > 0)
				adj[p] += m;
			else
				adj[p] -= m;
		}
	}
	// rescale all
	float mn = INT_MAX, mx = -INT_MAX;
	for(size_t p=0; p<adj.size(); p++) {
		mn = std::min(mn,adj[p]);
		mx = std::max(mx,adj[p]);
	}
	const float s = mx-mn, t = (1.0f-WATER_LEVEL)*1.5f;
	for(size_t p=0; p<points.size(); p++) {
		const float a = 1.0f - (((adj[p]-mn)/s)*t);
		adj[p] = a;
	}
	// classify it
	const float POLAR = 0.7f;
	for(size_t p=0; p<points.size(); p++) {
		const float y = points[p].y;
		const bool polar = (y < -POLAR || y > POLAR);
		if(adj[p] > WATER_LEVEL) {
			if((adj[p] > MOUNTAIN_LEVEL))
				types.append(MOUNTAIN);
			else if(polar)
				types.append(ICE_FLOW);
			else
				types.append(LAND);	
		} else {
			if(polar)
				types.append(ICE);
			else
				types.append(WATER);
		}	
	}
	// smooth land that isn't mountains
	std::cout << ": smoothing land with " << smoothing_passes << " passes" << std::endl;
	for(size_t i=0; i<smoothing_passes; i++) {
		for(size_t p=0; p<points.size(); p++) {
			if(types[p] == LAND || types[p] == ICE_FLOW) {
				float a = adj[p];
				for(int n=0; n<6; n++)
					if(adjacent_points[p].adj[n] == adjacent_t::EMPTY)
						break;
					else
						a += adj[adjacent_points[p].adj[n]];
				a /= adjacent_points[p].size()+1;
				adj[p] = a;
			}
		}
	}
	// set heights
	for(size_t p=0; p<points.size(); p++)
		if(types[p] == WATER || types[p] == ICE)
			points[p] *= WATER_LEVEL;
		else
			points[p] *= adj[p];
	// colour it
	const rgb_t WATER_COLOUR(0,0,0xff),
		ICE_COLOUR(0xff,0xff,0xff),
		LAND_COLOUR(0,0xff,0),
		ICE_FLOW_COLOUR(0xc0,0xc0,0xc0),
		MOUNTAIN_COLOUR(0xa0,0xa0,0xa0);
	for(size_t p=0; p<points.size(); p++) {
		switch(types[p]) {
		case WATER:
			colours.append(WATER_COLOUR);
			break;
		case ICE:
			colours.append(ICE_COLOUR);
			break;
		case LAND:
			colours.append(LAND_COLOUR);
			break;
		case ICE_FLOW:
			colours.append(ICE_FLOW_COLOUR);
			break;
		case MOUNTAIN:
			colours.append(MOUNTAIN_COLOUR);
			break;
		default:
			assert(false);
		}
	}
}

GLuint planet_t::midpoint(GLuint a,GLuint b) {
	const uint64_t key = (std::min<uint64_t>(a,b) << 32) + std::max(a,b);
	midpoints_t::iterator i = midpoints.find(key);
	if(i!=midpoints.end())
		return i->second;
	const vec_t& p = points[a], q = points[b];
	const vec_t v = ((q+p)/2).normalise();
	const GLuint value = points.append(v);
	midpoints_t::value_type kv(key,value);
	midpoints.insert(kv);
	return value;
}

GLuint planet_t::find_face(GLuint a,GLuint b,GLuint c) {
	adjacent_t set = adjacent_faces[a];
	set -= adjacent_faces[b];
	set -= adjacent_faces[c];
	assert(set.adj[0] != adjacent_t::EMPTY);
	assert(set.adj[1] == adjacent_t::EMPTY);
	return set.adj[0];
}

void planet_t::draw() {
	static const double PI = 3.14159265;
	const double r = fmod(now()/20.0,360)*(PI/180.0);
	const vec_t sun = this->sun.rotate(r,vec_t(0,1,0),vec_t(0,-1,0));
	static float light[4];
	light[0] = sun.x;
	light[1] = sun.y;
	light[2] = sun.z;
	light[3] = 0;
	glLightfv(GL_LIGHT1,GL_POSITION,light);
#if 0 // vertex arrays
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,0,points.ptr());
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,colours.ptr());
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,0,normals.ptr());
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		glDrawRangeElements(GL_TRIANGLES,
			(*i)->mn_point,(*i)->mx_point,
			((*i)->stop-(*i)->start+1)*3,
			GL_UNSIGNED_INT,
			faces.ptr()+(*i)->start);
#else // vertices
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		(*i)->draw();
#endif
}

bool planet_t::intersection(int x,int y,vec_t& pt) {
	return false;
}

terrain_t* gen_planet(size_t recursionLevel,size_t iterations,size_t smoothing_passes) {
	return new planet_t(recursionLevel,iterations,smoothing_passes);
}
