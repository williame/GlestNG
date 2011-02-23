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
std::auto_ptr<model_g3d_t> model, logo;

static void _draw_quad(const vec_t& a,const vec_t& b,const vec_t& c,const vec_t& d) {
	glVertex3f(a.x,a.y,a.z);
	glVertex3f(b.x,b.y,b.z);
	glVertex3f(c.x,c.y,c.z);
	glVertex3f(d.x,d.y,d.z);
}

static void draw_cube(const aabb_t& box) {
	vec_t c[8];
	for(int i=0; i<8; i++)
		c[i] = box.corner(i);
	glBegin(GL_QUADS);
	// left
	glNormal3f(-1,0,0);
	_draw_quad(c[0],c[1],c[2],c[3]);
	// right
	glNormal3f(1,0,0);
	_draw_quad(c[4],c[5],c[6],c[7]);
	// front
	glNormal3f(0,0,0);
	_draw_quad(c[0],c[4],c[7],c[3]);
	// back
	glNormal3f(0,0,-1);
	_draw_quad(c[1],c[5],c[6],c[2]);
	// up
	glNormal3f(0,0,0);
	_draw_quad(c[0],c[1],c[5],c[3]);
	// down
	glNormal3f(0,-1,0);
	_draw_quad(c[3],c[2],c[6],c[7]);
	glEnd();
}

static void caret(const vec_t& pos,float scale,float rx,float ry,float rz,bool mark=false) {
	glPushMatrix();		
	glTranslatef(pos.x,pos.y,pos.z);
	glScalef(scale,scale,scale);
	if(rx) glRotatef(360.0/rx,1,0,0);
	if(ry) glRotatef(360.0/ry,0,1,0);
	if(rz) glRotatef(360.0/rz,0,0,1);
	bounds_t box(vec_t(-1,-1,-1),vec_t(1,1,1));
	if(!mark && model.get()) {
		model->draw(0);
		box = model->get_bounds();
	}
	if(!mark)
		glColor4ub(0xff,0xff,0xff,0x15);
	draw_cube(box);
	glPopMatrix();
	return;
}

struct test_t: public object_t {
	enum { MIN_AGE = 60*5, MAX_AGE = 60*8, };
	static const float SZ, MARGIN, SPEED;
	static bounds_t legal;
	test_t(): object_t(UNIT), age(MIN_AGE+(rand()%(MAX_AGE-MIN_AGE))),
		r(128+(rand()%128)), g(128+(rand()%128)), b(128+(rand()%128)),
		rx(randf()), ry(randf()), rz(randf()),
		dir(randf(),randf(),randf()),
		drawn(false)
	{
		bounds_include(vec_t(0,0,0));
		bounds_include(vec_t(SZ*2,SZ*2,SZ*2));
		bounds_fix();
		bounds_include(bounding_box().a);
		bounds_include(bounding_box().b);
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
		drawn = true;
	}
	bool refine_intersection(const ray_t&, vec_t& I) { 
		I = centre;
		return true;
	}
	int age;
	const uint8_t r,g,b;
	const float rx, ry, rz;
	vec_t dir;
	bool drawn;
};
const float test_t::SZ = 0.05, test_t::MARGIN = test_t::SZ*2, test_t::SPEED = 0.01;
bounds_t test_t::legal(vec_t(-1.0+MARGIN,-1.0+MARGIN,-1.0+MARGIN),
	vec_t(1.0-MARGIN,1.0-MARGIN,1.0-MARGIN));
typedef std::vector<test_t*> tests_t;
tests_t objs;

void ui() {
	glColor3f(1,1,1);
	if(selection) {
		glDisable(GL_DEPTH_TEST);
		glColor3f(1,0,0);
		caret(selected_point,0.03,0,0,0,true);
		glEnable(GL_DEPTH_TEST);
	}
	// draw logo
	{
		glPushMatrix();
		glDisable(GL_LIGHT1);
		glColor3f(1,0,0);
		const bounds_t& b = logo->get_bounds();
		glTranslatef(-b.centre.x,-b.centre.y,b.centre.z);
		glTranslatef(-1.2,0,0);
		glRotatef(90,0,1,0);
		logo->draw(0);
		glEnable(GL_LIGHT1);
		glPopMatrix();
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
			else if(!obj->drawn)
				std::cerr << *obj << " thinks it is visible but wasn't drawn" << std::endl;
			else
				glColor3ub(0,0xff,0);
		} else {
			if(world()->is_visible(*obj))
				std::cerr << *obj << " thinks it is invisible but it is" << std::endl;
			else if(obj->drawn)
				std::cerr << *obj << " is invisible but was drawn" << std::endl;
			else
				glColor3ub(0,0,0xff);
		}
		obj->drawn = false;
		caret(obj->get_pos(),obj->SZ,obj->rx,obj->ry,obj->rz);
		if(!obj->tick()) {
			objs.erase(objs.begin()+i);
			delete obj;
		}
	}
	glEnable(GL_TEXTURE_2D);
	if(!objs.size() < MIN_OBJS) {
		const size_t n = MIN_OBJS+(rand()%(MAX_OBJS-MIN_OBJS));
		while(objs.size()<n) {
			objs.push_back(new test_t());
		}
	}
}

void tick() {
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
	spatial_test();
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
	matrix_t _mv, _p;
	glGetFloatv(GL_MODELVIEW_MATRIX,_mv.f);
	glGetFloatv(GL_PROJECTION_MATRIX,_p.f);
	const matrix_t inv = (_p*_mv).inverse();
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT,viewport);
	gluUnProject(x,viewport[3]-y,0,mv,p,viewport,&a,&b,&c);
	const vec_t origin(vec_t(a,b,c)*inv);
	gluUnProject(x,viewport[3]-y,1,mv,p,viewport,&d,&e,&f);
	const vec_t dest(vec_t(d,e,f)*inv);
	ray_t ray(origin,dest-origin);
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
	const std::string techtree_ = techtrees_[rand()%techtrees_.size()];
	techtree.reset(new techtree_t(fs,techtree_));
	const strings_t factions = techtree->get_factions();
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
	std::cout << "loading "<<g3d<<std::endl;
	fs_file_t::ptr_t g3d_file(fs.get(g3d));
	istream_t::ptr_t gstream(g3d_file->reader());
	model = std::auto_ptr<model_g3d_t>(new model_g3d_t(*gstream));
	// and load it
	std::cout << "loading "<<xml_name<<std::endl;
	fs_file_t::ptr_t xml_file(fs.get(xml_name));
	istream_t::ptr_t xstream(xml_file->reader());
	unit_type = std::auto_ptr<unit_type_t>(new unit_type_t(faction,unit_));
	unit_type->load_xml(*xstream);
	//new ui_xml_editor_t(*unit_type);
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
		std::auto_ptr<fs_t> fs_settings(fs_t::create("data/"));
		fs_t::settings = fs_settings.get(); // set it globally
		std::auto_ptr<xml_parser_t> xml_settings;
		{
			fs_file_t::ptr_t ui_settings_file(fs_settings->get("ui_settings.xml"));
			istream_t::ptr_t ui_settings_stream(ui_settings_file->reader());
			xml_settings.reset(new xml_parser_t("UI Settings",ui_settings_stream->read_all()));
			xml_settings->set_as_settings();
		}
		std::auto_ptr<graphics_t::mgr_t> graphics_mgr(graphics_t::create());
		std::auto_ptr<fonts_t> fonts(fonts_t::create());
		std::auto_ptr<fs_t> fs(fs_t::create("data/Glest"));
		std::auto_ptr<ui_mgr_t> ui_(ui_mgr());
		std::auto_ptr<mod_ui_t> mod_ui(mod_ui_t::create());
		{
			fs_file_t::ptr_t logo_file(fs_settings->get("logo.g3d"));
			istream_t::ptr_t logostream(logo_file->reader());
			logo = std::auto_ptr<model_g3d_t>(new model_g3d_t(*logostream));
		}

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
					if(selection)
						std::cout << "drag" << std::endl;
					/*printf("Mouse moved by %d,%d to (%d,%d)\n", 
					event.motion.xrel, event.motion.yrel,
					event.motion.x, event.motion.y);*/
					break;
				case SDL_MOUSEBUTTONDOWN:
					click(event.button.x,event.button.y);
					if(selection)
						std::cout << "selection: "<<selected_point<<std::endl;
					break;
				case SDL_MOUSEBUTTONUP:
					if(selection)
						std::cout << "selection stopped"<<std::endl;
					selection = false;
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
						if(mod_ui->is_shown())
							mod_ui->hide();
						else
							mod_ui->show(ref_t(*techtree,TECHTREE,techtree->name));
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
