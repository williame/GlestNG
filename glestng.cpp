/*
 glestng.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include <inttypes.h>

#include "terrain.hpp"

SDL_Surface* screen;
terrain_t* terrain;

void tick() {
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	terrain->draw();
	SDL_GL_SwapBuffers();
	SDL_Flip(screen);
}

float* v4(float a,float b,float c,float d) {
	static float r[4] = {a,b,c,d};
	return r;
}

int main(int argc,char** args) {
	
	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr,"Unable to initialize SDL: %s\n",SDL_GetError());
		return EXIT_FAILURE;
	}
	
	printf("terraforming...\n");
	terrain = gen_planet(2);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	screen = SDL_SetVideoMode(1024,768,32,SDL_OPENGL/*|SDL_FULLSCREEN*/);
	if(!screen) {
		fprintf(stderr,"Unable to create SDL screen: %s\n",SDL_GetError());
		return EXIT_FAILURE;
	}
	glLightfv(GL_LIGHT0,GL_AMBIENT,v4(0,0,0,1));
        glLightfv(GL_LIGHT0,GL_DIFFUSE,v4(1.,1.,1.,1.));
        glLightfv(GL_LIGHT0,GL_SPECULAR,v4(1.,1.,1.,1.));
        glLightfv(GL_LIGHT0,GL_POSITION,v4(1.,1.,1.,0.));
        glMaterialfv(GL_FRONT,GL_AMBIENT,v4(.7,.7,.7,1.));
        glMaterialfv(GL_FRONT,GL_DIFFUSE,v4(.8,.8,.8,1.));
        glMaterialfv(GL_FRONT,GL_SPECULAR,v4(1.,1.,1.,1.));
        glMaterialf(GL_FRONT,GL_SHININESS,100.0);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_COLOR_MATERIAL);
	
	bool quit = false;
	SDL_Event event;
	while(!quit) {
		while(!quit && SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_MOUSEMOTION:
				printf("Mouse moved by %d,%d to (%d,%d)\n", 
				event.motion.xrel, event.motion.yrel,
				event.motion.x, event.motion.y);
				break;
			case SDL_MOUSEBUTTONDOWN:
				printf("Mouse button %d pressed at (%d,%d)\n",
				event.button.button, event.button.x, event.button.y);
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
		tick();
       }
        
        delete terrain;
	
	SDL_Quit();
	return EXIT_SUCCESS;
}
