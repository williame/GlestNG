#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include <inttypes.h>

void tick() {
}

int main(int argc,char** args) {
	
	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr,"Unable to initialize SDL: %s\n",SDL_GetError());
		return 1;
	}
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_Surface* screen = SDL_SetVideoMode(1024,768,32,SDL_OPENGL/*|SDL_FULLSCREEN*/);
	if(!screen) {
		fprintf(stderr,"Unable to create SDL screen: %s\n",SDL_GetError());
		return 1;
	}
	
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
		SDL_Flip(screen);
        }
	
	SDL_Quit();
	return 0;
}
