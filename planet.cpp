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

#include "memcheck.h"
#include "terrain.hpp"
#include "graphics.hpp"

template<typename T> class fixed_array_t {
public:
	fixed_array_t(size_t capacity);
	virtual ~fixed_array_t() { delete[] data; }
	T* ptr() const { return data; }
	size_t append(T t);
	T& operator[](size_t i);
	const T& operator[](size_t i) const;
	size_t size() const { return len; }
	bool full() const { return len==capacity; }
	void fill(const T& t);
	const size_t capacity;
private:
	size_t len;
	T* data;
};

template<typename T> fixed_array_t<T>::fixed_array_t(size_t cap):
	capacity(cap), len(0), data(new T[cap])
{
	VALGRIND_MAKE_MEM_UNDEFINED(data,sizeof(T)*capacity);
}

template<typename T> size_t fixed_array_t<T>::append(T t) {
	assert(len<capacity);
	data[len] = t;
	return len++;
}

template<typename T> T& fixed_array_t<T>::operator[](size_t i) {
	assert(i<len);
	return data[i];
}
	
template<typename T> const T& fixed_array_t<T>::operator[](size_t i) const {
	assert(i<len);
	return data[i];
}

template<typename T> void fixed_array_t<T>::fill(const T& t) {
	for(size_t i=0; i<capacity; i++)
		data[i] = t;
	len = capacity;
}

enum {
	DIVIDE_THRESHOLD = 3,
	FACE_IDX = 18, // how much to shift to get the ID of the mesh the face belongs to?
	FACE_MASK = (1<<FACE_IDX)-1,
};

struct face_t {
	face_t() {}
	face_t(GLuint a_,GLuint b_,GLuint c_): a(a_), b(b_), c(c_) {}
	GLuint a,b,c;
};

struct planet_t;

struct mesh_t {
	mesh_t(planet_t& planet,face_t tri,size_t recursionLevel);
	static size_t num_faces(size_t recursionLevel);
	void draw();
	planet_t& planet;
	const GLuint ID;
	bounds_t bounds;
	fixed_array_t<face_t> faces;
};

struct planet_t: public terrain_t {
	planet_t	(size_t recursionLevel);
	~planet_t();
	static size_t num_points(size_t recursionLevel);
	GLuint midpoint(GLuint a,GLuint b);
	GLuint find_face(GLuint a,GLuint b,GLuint c);
	void draw();
	void divide(const face_t& tri,size_t recursionLevel,size_t depth);
	bool intersection(int x,int y,vec_t& pt);
	typedef std::map<uint64_t,GLuint> midpoints_t;
	midpoints_t midpoints;
	typedef std::vector<mesh_t*> meshes_t;
	meshes_t meshes;
	fixed_array_t<vec_t> points;
	fixed_array_t<vec_t> normals;
	struct adjacent_t {
		adjacent_t();
		void dump(FILE* out=stdout) const;
		GLuint adj[6];
		void add(GLuint neighbour);
		adjacent_t& operator-=(adjacent_t a);
		static const GLuint EMPTY = ~0; 
	};
	fixed_array_t<adjacent_t> adjacent_faces;
	fixed_array_t<adjacent_t> adjacent_points;
};

size_t mesh_t::num_faces(size_t recursionLevel) {
	static const size_t Nf[] = {0,65,64,153,561,2145,8385};
	assert(recursionLevel <= DIVIDE_THRESHOLD);
	assert(recursionLevel<(sizeof(Nf)/sizeof(*Nf)));
	const size_t nf = Nf[recursionLevel];
	printf("num_faces(%zu)=%zu\n",recursionLevel,nf);
	assert(nf);
	return nf;
}

mesh_t::mesh_t(planet_t& p,face_t tri,size_t recursionLevel):
	planet(p),
	ID(p.meshes.size()<<FACE_IDX),
	faces(num_faces(recursionLevel))
{
	faces.append(tri);
	for(size_t i=0; i<=recursionLevel; i++) {
		const size_t prev = faces.size();
		for(size_t j=0; j<prev; j++) {
			const face_t& f = faces[j];
			const GLuint
				a = planet.midpoint(f.a,f.b),
				b = planet.midpoint(f.b,f.c),
				c = planet.midpoint(f.c,f.a);
			faces.append(face_t(f.a,a,c));
			faces.append(face_t(f.b,b,a));
			faces.append(face_t(f.c,c,b));
			faces[j] = face_t(a,b,c);
		}
	}
	for(size_t i=0; i<faces.size(); i++) {
		const face_t& f = faces[i]; 
		printf("%zu = %u,%u,%u\n",i,(unsigned)f.a,(unsigned)f.b,(unsigned)f.c);
		printf("\t%f,%f,%f\n",planet.points[f.a].x,planet.points[f.a].y,planet.points[f.a].z);
	}
	printf("%zu faces (of %zu) at %zu recursion\n",faces.size(),faces.capacity,recursionLevel);
	assert(faces.full());
	for(int i=faces.size()-1; i>=0; i--) {
		const face_t& f = faces[i];
		const GLuint a = f.a, b = f.b, c = f.c;
		printf("adjacency %d = %u,%u,%u\n",i,(unsigned)a,(unsigned)b,(unsigned)c);
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

void mesh_t::draw() {
	glColor3f(1.,0,0);
	glBegin(GL_TRIANGLES);
	for(size_t i=0; i<faces.size(); i++) {
		const face_t& face = faces[i];
		glVertex3f(planet.points[face.a].x,planet.points[face.a].y,planet.points[face.a].z);
		glVertex3f(planet.points[face.b].x,planet.points[face.b].y,planet.points[face.b].z);
		glVertex3f(planet.points[face.c].x,planet.points[face.c].y,planet.points[face.c].z);
	}
	glEnd();
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
	printf("cannot add %u to ",(unsigned)neighbour); dump(); putchar('\n');
	assert(false);
}

void planet_t::adjacent_t::dump(FILE* out) const {
	fputc('[',out);
	for(int i=0; i<6; i++)
		if(adj[i]==EMPTY)
			fputc(',',out);
		else
			fprintf(out,"%u,",(unsigned)adj[i]);
	fputc(']',out);
}

planet_t::adjacent_t::adjacent_t& planet_t::adjacent_t::operator-=(adjacent_t a) {
	dump(); printf("-"); a.dump();
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
		printf(" %x ",intersection);
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
		printf(" empty! ");
		for(int i=0; i<6; i++) {
			if(adj[i] == EMPTY)
				break;
			adj[i] = EMPTY;
		}
	}
	printf("="); dump(); putchar('\n');
	return *this;
}

size_t planet_t::num_points(size_t recursionLevel) {
	assert(recursionLevel);
	return (5*pow(2,2*recursionLevel+3)+2);
}

planet_t::planet_t(size_t recursionLevel):
	points(num_points(recursionLevel)),
	normals(num_points(recursionLevel)),
	adjacent_faces(num_points(recursionLevel)),
	adjacent_points(num_points(recursionLevel))
{
	adjacent_faces.fill(adjacent_t());
	adjacent_points.fill(adjacent_t());
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
        	size_t face_count = 0;
        	for(meshes_t::const_iterator i=meshes.begin(); i!=meshes.end(); i++)
        		face_count += (*i)->faces.size();
        	printf("%zu points, %zu meshes, %zu faces\n",points.size(),meshes.size(),face_count);
	assert(points.full());
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

GLuint planet_t::midpoint(GLuint a,GLuint b) {
	const uint64_t key = (std::min<uint64_t>(a,b) << 32) + std::max(a,b);
	midpoints_t::iterator i = midpoints.find(key);
	if(i!=midpoints.end())
		return i->second;
	const vec_t& p = points[a], q = points[b];
	vec_t v((p.x+q.x)/2.,(p.y+q.y)/2.,(p.z+q.z)/2.);
	v.normalise();
	const GLuint value = points.append(v);
	midpoints_t::value_type kv(key,value);
	midpoints.insert(kv);
	return value;
}

GLuint planet_t::find_face(GLuint a,GLuint b,GLuint c) {
	printf("find_face() a=%u ",(unsigned)a); adjacent_faces[a].dump();
	printf(", b=%u ",(unsigned)b); adjacent_faces[b].dump();
	printf(", c=%u ",(unsigned)c); adjacent_faces[c].dump();
	putchar('\n');
	adjacent_t set = adjacent_faces[a];
	set -= adjacent_faces[b];
	set -= adjacent_faces[c];
	assert(set.adj[0] != adjacent_t::EMPTY);
	assert(set.adj[1] == adjacent_t::EMPTY);
	printf("find_face() returns %u (%u,%u)\n",(unsigned)set.adj[0],(unsigned)set.adj[0]>>FACE_IDX,(unsigned)set.adj[0]&FACE_MASK);
	return set.adj[0];
}

void planet_t::draw() {
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		(*i)->draw();
}

bool planet_t::intersection(int x,int y,vec_t& pt) {
	return false;
}

terrain_t* gen_planet(size_t recursionLevel) {
	return new planet_t(recursionLevel);
}
