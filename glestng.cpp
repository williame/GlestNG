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
#include "utils.hpp"
#include "ui.hpp"
#include "ui_xml_editor.hpp"
#include "ui_list.hpp"
#include "unit.hpp"
#include "techtree.hpp"
#include "faction.hpp"
#include "mod_ui.hpp"

SDL_Surface* screen;

perf_t framerate;
uint64_t frame_count = 0;

bool selection = false;
vec_t selected_point;
int visible_objects = 0;

std::auto_ptr<techtree_t> techtree;
std::auto_ptr<unit_type_t> unit_type;
std::auto_ptr<model_g3d_t> model;

void caret(const vec_t& pos,float scale,float rx,float ry,float rz) {
	glPushMatrix();		
	glTranslatef(pos.x,pos.y,pos.z);
	glScalef(scale,scale,scale);
	if(rx) glRotatef(360.0/rx,1,0,0);
	if(ry) glRotatef(360.0/ry,0,1,0);
	if(rz) glRotatef(360.0/rz,0,0,1);
	if(model.get()) {
		model->draw(0);
	}
	glColor4ub(0xff,0xff,0xff,0x15);
	glBegin(GL_QUADS);	
	// classic NeHe	
	// Front Face
	glNormal3f( 0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer
	glVertex3f(-1.0f, -1.0f,  1.0f);	// Point 1 (Front)
	glVertex3f( 1.0f, -1.0f,  1.0f);	// Point 2 (Front)
	glVertex3f( 1.0f,  1.0f,  1.0f);	// Point 3 (Front)
	glVertex3f(-1.0f,  1.0f,  1.0f);	// Point 4 (Front)
	// Back Face
	glNormal3f( 0.0f, 0.0f,-1.0f); // Normal Pointing Away From Viewer
	glVertex3f(-1.0f, -1.0f, -1.0f);	// Point 1 (Back)
	glVertex3f(-1.0f,  1.0f, -1.0f);	// Point 2 (Back)
	glVertex3f( 1.0f,  1.0f, -1.0f);	// Point 3 (Back)
	glVertex3f( 1.0f, -1.0f, -1.0f);	// Point 4 (Back)
	// Top Face
	glNormal3f( 0.0f, 1.0f, 0.0f); // Normal Pointing Up
	glVertex3f(-1.0f,  1.0f, -1.0f);	// Point 1 (Top)
	glVertex3f(-1.0f,  1.0f,  1.0f);	// Point 2 (Top)
	glVertex3f( 1.0f,  1.0f,  1.0f);	// Point 3 (Top)
	glVertex3f( 1.0f,  1.0f, -1.0f);	// Point 4 (Top)
	// Bottom Face
	glNormal3f( 0.0f,-1.0f, 0.0f); // Normal Pointing Down
	glVertex3f(-1.0f, -1.0f, -1.0f);	// Point 1 (Bottom)
	glVertex3f( 1.0f, -1.0f, -1.0f);	// Point 2 (Bottom)
	glVertex3f( 1.0f, -1.0f,  1.0f);	// Point 3 (Bottom)
	glVertex3f(-1.0f, -1.0f,  1.0f);	// Point 4 (Bottom)
	// Right face
	glNormal3f( 1.0f, 0.0f, 0.0f); // Normal Pointing Right
	glVertex3f( 1.0f, -1.0f, -1.0f);	// Point 1 (Right)
	glVertex3f( 1.0f,  1.0f, -1.0f);	// Point 2 (Right)
	glVertex3f( 1.0f,  1.0f,  1.0f);	// Point 3 (Right)
	glVertex3f( 1.0f, -1.0f,  1.0f);	// Point 4 (Right)
	// Left Face
	glNormal3f(-1.0f, 0.0f, 0.0f); // Normal Pointing Left
	glVertex3f(-1.0f, -1.0f, -1.0f);	// Point 1 (Left)
	glVertex3f(-1.0f, -1.0f,  1.0f);	// Point 2 (Left)
	glVertex3f(-1.0f,  1.0f,  1.0f);	// Point 3 (Left)
	glVertex3f(-1.0f,  1.0f, -1.0f);	// Point 4 (Left)
	glEnd();	
	glPopMatrix();
}

struct test_t: public object_t {
	enum { MIN_AGE = 60*5, MAX_AGE = 60*8, };
	static const float SZ, MARGIN, SPEED;
	static bounds_t legal;
	test_t(): object_t(UNIT), age(MIN_AGE+(rand()%(MAX_AGE-MIN_AGE))),
		r(128+(rand()%128)), g(128+(rand()%128)), b(128+(rand()%128)),
		rx(randf()), ry(randf()), rz(randf()),
		dir(randf(),randf(),randf()),
		drawn(frame_count)
	{
		bounds_include(vec_t(-SZ,-SZ,-SZ));
		bounds_include(vec_t(SZ,SZ,SZ));
		bounds_fix();
		set_pos(vec_t(randf()-MARGIN,randf()-MARGIN,randf()-MARGIN));
		world()->add(this);
		dir.normalise();
		dir *= SPEED;
	}
	bool tick() {
		if(age-- <= 0) return false;
		vec_t p = get_pos();
		p.x += dir.x; if((p.x<legal.a.x)||(p.x>legal.b.x)) { dir.x = -dir.x; p.x += dir.x; }
		p.y += dir.y; if((p.y<legal.a.y)||(p.y>legal.b.y)) { dir.y = -dir.y; p.y += dir.y; }
		p.z += dir.z; if((p.z<legal.a.z)||(p.z>legal.b.z)) { dir.z = -dir.z; p.z += dir.z; }
		set_pos(p);
		return true;
	}
	void draw(float) {
		drawn = frame_count;
		//glColor3ub(r,g,b);
		//caret(get_pos(),SZ,rx,ry,rz);
	}
	void draw_bad() {
		drawn = frame_count;
		glColor3ub(0xff,0,0);
		caret(get_pos(),SZ,rx,ry,rz);
	}
	bool refine_intersection(const ray_t&, vec_t& I) { 
		I = centre;
		return true;
	}
	int age;
	const uint8_t r,g,b;
	const float rx, ry, rz;
	vec_t dir;
	uint64_t drawn;
};
const float test_t::SZ = 0.05, test_t::MARGIN = test_t::SZ*2, test_t::SPEED = 0.01;
bounds_t test_t::legal(vec_t(-1.0+MARGIN,-1.0+MARGIN,-1.0+MARGIN),
	vec_t(1.0-MARGIN,1.0-MARGIN,1.0-MARGIN));
typedef std::vector<test_t*> tests_t;
tests_t objs;

void ui() {
	if(selection) {
		glColor3f(1,0,0);
		caret(selected_point,0.03,0,0,0);
	}
	static char fps[128];
	snprintf(fps,sizeof(fps),"%u fps, %d visible objects (of %u)",(unsigned)framerate.per_second(now()),visible_objects,(unsigned)objs.size());
	static ui_label_t* label = new ui_label_t("this is a test");
	//label->set_pos(vec2_t(10,10));
	label->set_text(fps);
	ui_mgr()->draw();
}

void spatial_test() {
	enum { MIN_OBJS = 150, MAX_OBJS = 170, };
	glDisable(GL_TEXTURE_2D);
	for(int i=objs.size()-1; i>=0; i--) {
		test_t* obj = objs[i];
		glColor3ub(0xff,0,0);
		if(obj->is_visible()) {
			if(!world()->is_visible(*obj))
				std::cerr << *obj << " thinks it is visible but it isn't" << std::endl;
			else if(obj->drawn != frame_count)
				std::cerr << *obj << " thinks it is visible but wasn't drawn" << std::endl;
			else
				glColor3ub(0,0xff,0);
		} else {
			if(world()->is_visible(*obj))
				std::cerr << *obj << " thinks it is invisible but it is" << std::endl;
			else if(obj->drawn == frame_count)
				std::cerr << *obj << " is invisible but was drawn" << std::endl;
			else
				glColor3ub(0,0,0xff);
		}
		caret(obj->get_pos(),obj->SZ,obj->rx,obj->ry,obj->rz);
		if(!obj->tick()) {
			objs.erase(objs.begin()+i);
			delete obj;
		}
	}
	glEnable(GL_TEXTURE_2D);
//	if(bad)
//		std::cerr << "("<<bad<<" objects were not drawn)" << std::endl;
	if(!objs.size() < MIN_OBJS) {
		const size_t n = MIN_OBJS+(rand()%(MAX_OBJS-MIN_OBJS));
		while(objs.size()<n) {
			objs.push_back(new test_t());
		}
	}
}

void tick() {
	spatial_test();
	world()->check();
	frame_count++;
	const world_t::hits_t& visible = world()->visible();
	visible_objects = visible.size();
//#define EXPLAIN // useful for seeing if it does draw front-to-back
#ifdef EXPLAIN
	for(size_t MAX_OBJS=1; MAX_OBJS<visible_objects; MAX_OBJS++) {
#else
	const size_t MAX_OBJS = visible_objects;
#endif
	if(visible_objects) {
		bool in_terrain = (terrain() && visible[0].type == TERRAIN);
		if(in_terrain) terrain()->draw_init();
		size_t i = 0;
		for(world_t::hits_t::const_iterator v=visible.begin(); v!=visible.end() && i<MAX_OBJS; v++, i++) {
			if(in_terrain && (v->type != TERRAIN)) {
				terrain()->draw_done();
				in_terrain = false;
			} else if((v->type == TERRAIN) && !in_terrain)
				panic("was not expecting "<<*v);
			v->obj->draw(v->d);
		}
		if(in_terrain)
			terrain()->draw_done();
	} else {
		for(tests_t::iterator i=objs.begin(); i!=objs.end(); i++)
			(*i)->draw(0);
	}
	ui();
	SDL_GL_SwapBuffers();
	SDL_Flip(screen);
	glClearColor(.2,.1,.2,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#ifdef EXPLAIN
	}
#endif
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
	world()->intersection(ray,~0,hits);
	uint64_t ns = high_precision_time()-start;
	std::cout << std::endl << "click(" << x << "," << y << ") " << ray << " (" << ns << " ns) "<<hits.size()<< std::endl;
	selection = false;
	for(world_t::hits_t::iterator i=hits.begin(); i!=hits.end(); i++) {
		vec_t pt;
		start = high_precision_time();
		bool hit = i->obj->refine_intersection(ray,pt);
		ns = high_precision_time()-start;
		if(hit) {
			std::cout << "hit " << pt << " ";
			if(!selection ||
				(pt.distance_sqrd(ray.o)<selected_point.distance_sqrd(ray.o))) {
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
	if(terrain()) {
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
}

struct v4_t {
	v4_t(float a,float b,float c,float d) {
		v[0] = a; v[1] = b; v[2] = c; v[3] = d;
	}
	float v[4];
};

float zoom = 60;

void camera() {
	std::cout << "zoom="<<zoom<<std::endl;
	glViewport(0,0,screen->w,screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	const float wh = (float)screen->w/(float)screen->h;
//	glOrtho(-wh,wh,-1,1,-1,1);
	gluPerspective(zoom,wh,1,10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0,0,-3);
//	glRotatef(90,0,1,0);
//	glPushMatrix();
	matrix_t projection, modelview;
	glGetFloatv(GL_MODELVIEW_MATRIX,projection.f);
	glGetFloatv(GL_PROJECTION_MATRIX,modelview.f);
	world()->set_frustum(vec_t(0,0,-3),projection*modelview);
//	glPopMatrix();
}

void load(fs_t& fs) {
	// this is just some silly test code - find a random model
	std::auto_ptr<techtrees_t> techtrees(new techtrees_t(fs));
	const strings_t techtrees_ = techtrees->get_techtrees();
	for(strings_t::const_iterator i=techtrees_.begin(); i!=techtrees_.end(); i++)
		std::cout << "techtree "<<*i<<std::endl;
	const std::string techtree_ = techtrees_[rand()%techtrees_.size()];
	techtree.reset(new techtree_t(fs,techtree_));
	const strings_t factions = techtree->get_factions();
	for(strings_t::const_iterator i=factions.begin(); i!=factions.end(); i++)
		std::cout << "faction "<<*i<<std::endl;
	const std::string faction_ = factions[rand()%factions.size()];
	faction_t& faction = techtree->get_faction(faction_);
	const strings_t units = fs.list_dirs(faction.path+"/units");
	const std::string unit_ = units[rand()%units.size()];
	const std::string unit = faction.path+"/units/"+unit_;
	const std::string xml_name = unit+"/"+unit_+".xml";
	const strings_t models = fs.list_files(unit+"/models");
	std::string g3d;
	for(strings_t::const_iterator i=models.begin(); i!=models.end(); i++)
		if(i->find(".g3d") == (i->size()-4)) {
			g3d = unit + "/models/" + *i;
			break;
		}
	if(!g3d.size()) data_error("no G3D models in "<<unit<<"/models");
	// and load it
	std::cout << "loading "<<xml_name<<std::endl;
	fs_file_t::ptr_t xml_file(fs.get(xml_name));
	istream_t::ptr_t xstream(xml_file->reader());
	unit_type = std::auto_ptr<unit_type_t>(new unit_type_t(unit));
	unit_type->load_xml(*xstream);
	//new ui_xml_editor_t(*unit_type);
	std::cout << "loading "<<g3d<<std::endl;
	fs_file_t::ptr_t g3d_file(fs.get(g3d));
	istream_t::ptr_t gstream(g3d_file->reader());
	model = std::auto_ptr<model_g3d_t>(new model_g3d_t(*gstream));
}

int main(int argc,char** args) {
	
	srand(time(NULL));
	
	try {
		
		if (SDL_Init(SDL_INIT_VIDEO)) {
			fprintf(stderr,"Unable to initialize SDL: %s\n",SDL_GetError());
			return EXIT_FAILURE;
		}
		atexit(SDL_Quit);
		
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
#if 0
		screen = SDL_SetVideoMode(1440,900,32,SDL_OPENGL|SDL_FULLSCREEN);
#else
		screen = SDL_SetVideoMode(1024,768,32,SDL_OPENGL/*|SDL_FULLSCREEN*/);
#endif
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

		// we have a GL context so we can go ahead and init all the singletons
		std::auto_ptr<graphics_t::mgr_t> graphics_mgr(graphics_t::create());
		std::auto_ptr<fonts_t> fonts(fonts_t::create());
		std::auto_ptr<fs_t> fs(fs_t::create("data/Glest"));
		std::auto_ptr<ui_mgr_t> ui_(ui_mgr());

		load(*fs);
		
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
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
		//glEnable(GL_NORMALIZE);
		//glFrontFace(GL_CW);
		camera();
		bool quit = false;
		SDL_Event event;
		SDL_EnableKeyRepeat(200,20);
		SDL_EnableUNICODE(true);
		const unsigned start = SDL_GetTicks();
		framerate.reset();
		while(!quit) {
			set_now(SDL_GetTicks()-start);
			while(!quit && SDL_PollEvent(&event)) {
				if(ui_mgr()->offer(event))
					continue;
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
					case SDLK_PLUS:
						zoom += 1;
						camera();
						break;
					case SDLK_MINUS:
						zoom -= 1;
						camera();
						break;
					case SDLK_ESCAPE:
						quit = true;
						break;
					case SDLK_m: // MODDING MODE
						if(mod_ui_is_shown())
							hide_mod_ui();
						else
							show_mod_ui(ref_t(*techtree,TECHTREE,techtree->name));
						break;
					default:
						std::cout << "Ignoring key " << 
							(int)event.key.keysym.scancode << "," <<
							event.key.keysym.sym << "," <<
							event.key.keysym.mod << "," <<
							event.key.keysym.unicode << std::endl;
					}
					break;
				case SDL_QUIT:
					quit = true;
					break;
				}
			}
			framerate.tick(now());
			tick();
		}
		for(tests_t::iterator i=objs.begin(); i!=objs.end(); i++)
			delete *i;
		return EXIT_SUCCESS;
	} catch(data_error_t* de) {
		std::cerr << "Oh! " << de << std::endl;
	} catch(graphics_error_t* ge) {
		std::cerr << "Oh! " << ge << std::endl;
	} catch(panic_t* panic) {
		std::cerr << "Oh! " << panic << std::endl;
	}
	return EXIT_FAILURE;
}
