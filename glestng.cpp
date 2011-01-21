/*
 glestng.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "graphics.hpp"

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>

#include "error.hpp"
#include "world.hpp"
#include "terrain.hpp"
#include "font.hpp"
#include "2d.hpp"
#include "g3d.hpp"

SDL_Surface* screen;

perf_t framerate;
uint64_t frame_count = 0;

bool selection = false;
vec_t selected_point;
int visible_objects = 0;

void caret(const vec_t& pos,float scale,float rx,float ry,float rz) {
	glPushMatrix();		
	glTranslatef(pos.x,pos.y,pos.z);
	glScalef(scale,scale,scale);
	glRotatef(360.0/rx,1,0,0);
	glRotatef(360.0/ry,0,1,0);
	glRotatef(360.0/rz,0,0,1);
	glBegin(GL_TRIANGLES);		
		//NeHe lesson 5 pyramid		
		glVertex3f( 0.0f, 1.0f, 0.0f);		
		glVertex3f(-1.0f,-1.0f, 1.0f);		
		glVertex3f( 1.0f,-1.0f, 1.0f);		
		glVertex3f( 0.0f, 1.0f, 0.0f);		
		glVertex3f( 1.0f,-1.0f, 1.0f);		
		glVertex3f( 1.0f,-1.0f, -1.0f);		
		glVertex3f( 0.0f, 1.0f, 0.0f);		
		glVertex3f( 1.0f,-1.0f, -1.0f);		
		glVertex3f(-1.0f,-1.0f, -1.0f);		
		glVertex3f( 0.0f, 1.0f, 0.0f);		
		glVertex3f(-1.0f,-1.0f,-1.0f);		
		glVertex3f(-1.0f,-1.0f, 1.0f);		
	glEnd();					
	glPopMatrix();
}

void ui() {
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	if(selection) {
		glColor3f(1,0,0);
		caret(selected_point,0.03,0,0,0);
	}
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0,0,screen->w,screen->h);
	gluOrtho2D(0,screen->w,0,screen->h);
	glColor3f(1,1,1);
	static char fps[128];
	snprintf(fps,sizeof(fps),"%u fps, %d visible objects",(unsigned)framerate.per_second(now()),visible_objects);
	font_mgr()->draw(10,10,fps);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

struct test_t: public object_t {
	enum { MIN_AGE = 60*5, MAX_AGE = 60*8, };
	static const float SZ;
	test_t(): object_t(UNIT), age(MIN_AGE+(rand()%(MAX_AGE-MIN_AGE))),
		r(128+(rand()%128)), g(128+(rand()%128)), b(128+(rand()%128)),
		rx(randf()), ry(randf()), rz(randf())
	{
		set_pos(vec_t(randf()-SZ,randf()-SZ,randf()-SZ));
		bounds_include(vec_t(-SZ,-SZ,-SZ));
		bounds_include(vec_t(SZ,SZ,SZ));
		bounds_fix();
		world()->add(this);
	}
	bool tick() {
		return (age-- <= 0);
	}
	void draw(float) {
		glColor3ub(r,g,b);
		caret(get_pos(),SZ,rx,ry,rz);
	}
	bool refine_intersection(const ray_t&, vec_t&) { return false; }
	int age;
	const uint8_t r,g,b;
	const float rx, ry, rz;
};
const float test_t::SZ = 0.05;

void spatial_test() {
	enum { MIN_OBJS = 100, MAX_OBJS = 200, };
	static std::vector<test_t*> objs;
	for(int i=objs.size()-1; i>=0; i--) {
		test_t* obj = objs[i];
		if(obj->tick()) {
			objs.erase(objs.begin()+i);
			delete obj;
		}
	}
	if(!objs.size() < MIN_OBJS) {
		const size_t n = MIN_OBJS+(rand()%(MAX_OBJS-MIN_OBJS));
		while(objs.size()<n) {
			objs.push_back(new test_t());
		}
	}
}

void tick() {
	frame_count++;
	spatial_test();
	const world_t::hits_t& visible = world()->visible();
	visible_objects = visible.size();
	if(visible_objects) {
		bool in_terrain = (terrain() && visible[0].type == TERRAIN);
		if(in_terrain) terrain()->draw_init();
		for(world_t::hits_t::const_iterator v=visible.begin(); v!=visible.end(); v++) {
			if(in_terrain && (v->type != TERRAIN)) {
				terrain()->draw_done();
				in_terrain = false;
			}
			v->obj->draw(v->d);
		}
		if(in_terrain)
			terrain()->draw_done();
	}
	ui();
	SDL_GL_SwapBuffers();
	SDL_Flip(screen);
	glClearColor(.2,.1,.2,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void click(int x,int y) {
	uint64_t start = high_precision_time();
	double mv[16], p[16], a, b, c, d, e, f;
	glGetDoublev(GL_MODELVIEW_MATRIX,mv);
	glGetDoublev(GL_PROJECTION_MATRIX,p);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT,viewport);
	gluUnProject(x,viewport[3]-y,-1,mv,p,viewport,&a,&b,&c);
	const vec_t origin(a,b,c);
	gluUnProject(x,viewport[3]-y,1,mv,p,viewport,&d,&e,&f);
	const vec_t dir(d-a,e-b,f-c);
	ray_t ray(origin,dir);
	world_t::hits_t hits;
	world()->intersection(ray,TERRAIN,hits);
	uint64_t ns = high_precision_time()-start;
	std::cout << std::endl << "click(" << x << "," << y << ") " << ray << " (" << ns << " ns)"<< std::endl;
	selection = false;
	for(world_t::hits_t::iterator i=hits.begin(); i!=hits.end(); i++) {
		vec_t pt;
		start = high_precision_time();
		bool hit = i->obj->refine_intersection(ray,pt);
		ns = high_precision_time()-start;
		if(hit) {
			std::cout << "hit " << pt << " ";
			if(!selection || (pt.distance_sqrd(ray.o)<selected_point.distance_sqrd(ray.o))) {
				selection = true;
				selected_point = pt;
				std::cout << "BEST ";
			}
		} else
			std::cout << "miss ";
		std::cout << *i << " (" << ns << " ns)" << std::endl;
	}
	if(selection) std::cout << "SELECTION: " << selected_point << std::endl;
	// the slow way
	terrain_t::test_hits_t test;
	start = high_precision_time();
	terrain()->intersection(ray,test);
	ns = high_precision_time()-start;
	std::cout << "(slow check: " << ns << " ns)" << std::endl;
	for(terrain_t::test_hits_t::iterator i=test.begin(); i!=test.end(); i++)
		std::cout << "TEST " <<
			(i->obj->sphere_t::intersects(ray)?"+":"-") << 
			(i->obj->aabb_t::intersects(ray)?"+":"-") <<
			*i->obj << i->hit << std::endl;
	vec_t surface;
	if(selection && terrain()->surface_at(selected_point,surface))
		std::cout << "(surface_at " << surface << " - " << selected_point << " = " << (selected_point-surface) << ")" << std::endl;
}

struct v4_t {
	v4_t(float a,float b,float c,float d) {
		v[0] = a; v[1] = b; v[2] = c; v[3] = d;
	}
	float v[4];
};

int main(int argc,char** args) {
	
	try {
	
		if (SDL_Init(SDL_INIT_VIDEO)) {
			fprintf(stderr,"Unable to initialize SDL: %s\n",SDL_GetError());
			return EXIT_FAILURE;
		}
		atexit(SDL_Quit);
		
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
		screen = SDL_SetVideoMode(1024,768,32,SDL_OPENGL/*|SDL_FULLSCREEN*/);
		if(!screen) {
			fprintf(stderr,"Unable to create SDL screen: %s\n",SDL_GetError());
			return EXIT_FAILURE;
		}
		SDL_WM_SetCaption("GlestNG","GlestNG");
	
		GLenum err = glewInit();
		if(GLEW_OK != err) {
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			return EXIT_FAILURE;
		}
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
		
		//terrain_t::gen_planet(5,500,3);
		//world()->dump(std::cout);
	
		v4_t light_amb(0,0,0,1), light_dif(1.,1.,1.,1.), light_spec(1.,1.,1.,1.), light_pos(1.,1.,-1.,0.),
			mat_amb(.7,.7,.7,1.), mat_dif(.8,.8,.8,1.), mat_spec(1.,1.,1.,1.);
		glLightfv(GL_LIGHT0,GL_AMBIENT,light_amb.v);
		glLightfv(GL_LIGHT0,GL_DIFFUSE,light_dif.v);
		glLightfv(GL_LIGHT0,GL_SPECULAR,light_spec.v);
		glLightfv(GL_LIGHT0,GL_POSITION,light_pos.v);
		glLightfv(GL_LIGHT1,GL_AMBIENT,light_amb.v);
		glLightfv(GL_LIGHT1,GL_DIFFUSE,light_dif.v);
		glLightfv(GL_LIGHT1,GL_SPECULAR,light_spec.v);
		glMaterialfv(GL_FRONT,GL_AMBIENT,mat_amb.v);
		glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_dif.v);
		glMaterialfv(GL_FRONT,GL_SPECULAR,mat_spec.v);
		glMaterialf(GL_FRONT,GL_SHININESS,100.0);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHT1);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glAlphaFunc(GL_GREATER,0.4);
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_RESCALE_NORMAL);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_NORMALIZE);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glViewport(0,0,screen->w,screen->h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		const float left = -(float)screen->w/(float)screen->h;
		glOrtho(left,-left,-1,1,10,-10);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		matrix_t projection, modelview;
		glGetDoublev(GL_MODELVIEW_MATRIX,projection.d);
		glGetDoublev(GL_PROJECTION_MATRIX,modelview.d);
		world()->set_frustum(projection,modelview);
		
		bool quit = false;
		SDL_Event event;
		const unsigned start = SDL_GetTicks();
		unsigned last_event = start;
		framerate.reset();
		while(!quit) {
			set_now(SDL_GetTicks()-start);
			// only eat events 10 times a second
			if((now()-last_event) > 100) {
				while(!quit && SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_MOUSEMOTION:
						/*printf("Mouse moved by %d,%d to (%d,%d)\n", 
						event.motion.xrel, event.motion.yrel,
						event.motion.x, event.motion.y);*/
						break;
					case SDL_MOUSEBUTTONDOWN:
						click(event.button.x,event.button.y);
						break;
					case SDL_KEYDOWN:
						switch(event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quit = true;
							break;
						default:;
						}
						break;
					case SDL_QUIT:
						quit = true;
						break;
					}
				}
				last_event = now();
			}
			framerate.tick(now());
			tick();
		}
		return EXIT_SUCCESS;
	} catch(data_error_t* de) {
		std::cerr << de << std::endl;
		return EXIT_FAILURE;
	} catch(panic_t* panic) {
		std::cerr << panic << std::endl;
		return EXIT_FAILURE;
	}
}
