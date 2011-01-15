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

#include "error.hpp"
#include "world.hpp"
#include "terrain.hpp"
#include "font.hpp"
#include "2d.hpp"

SDL_Surface* screen;
terrain_t* terrain;

perf_t framerate;

void ui() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0,0,screen->w,screen->h);
	gluOrtho2D(0,screen->w,0,screen->h);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glColor3f(1,1,1);
	char fps[12];
	snprintf(fps,sizeof(fps),"%u fps",(unsigned)framerate.per_second(now()));
	font_mgr()->draw(10,10,fps);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

void tick() {
	terrain->draw();
	ui();
	SDL_GL_SwapBuffers();
	SDL_Flip(screen);
	glClearColor(.2,.1,.2,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void click(int x,int y) {
        double mv[16], p[16], a, b, c;
        glGetDoublev(GL_MODELVIEW_MATRIX,mv);
        glGetDoublev(GL_PROJECTION_MATRIX,p);
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT,viewport);
        gluUnProject(x,y,1,mv,p,viewport,&a,&b,&c);
        const vec_t origin(a,b,c);
        gluUnProject(x,y,-1,mv,p,viewport,&a,&b,&c);
        const vec_t dest(a,b+0.1,c);
        ray_t ray(origin,dest-origin);
        std::cout << "click(" << x << "," << y << ") " << ray << std::endl;
        world_t::hits_t hits;
        world()->intersection(ray,world_t::TERRAIN,hits);
        for(world_t::hits_t::iterator i=hits.begin(); i!=hits.end(); i++)
        		std::cout << *i << std::endl; 
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
		screen = SDL_SetVideoMode(1024,768,32,SDL_OPENGL|SDL_HWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);
		if(!screen) {
			fprintf(stderr,"Unable to create SDL screen: %s\n",SDL_GetError());
			return EXIT_FAILURE;
		}
	
		GLenum err = glewInit();
		if(GLEW_OK != err) {
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			return EXIT_FAILURE;
		}
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	
		terrain = gen_planet(5,500,3);
		world()->dump(std::cout);
	
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
		
		bool quit = false;
		SDL_Event event;
		const unsigned start = SDL_GetTicks();
		unsigned last_event = start;
		framerate.reset();
		while(!quit) {
			set_now(SDL_GetTicks()-start);
			// only eat events 10 times a second
			if((now()-last_event) > 1000) {
				while(!quit && SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_MOUSEMOTION:
						/*printf("Mouse moved by %d,%d to (%d,%d)\n", 
						event.motion.xrel, event.motion.yrel,
						event.motion.x, event.motion.y);*/
						break;
					case SDL_MOUSEBUTTONDOWN:
						printf("Mouse button %d pressed at (%d,%d)\n",
						event.button.button, event.button.x, event.button.y);
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
		delete terrain;
		return EXIT_SUCCESS;
	} catch(panic_t* panic) {
		std::cerr << panic << std::endl;
		return EXIT_FAILURE;
	}
}
