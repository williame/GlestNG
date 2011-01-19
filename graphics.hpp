/*
 graphics.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __GRAPHICS_HPP__
#define __GRAPHICS_HPP__

#define USE_GL

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

#include <map>

class art_mgr_t {
public:
	static art_mgr_t* mgr();
	GLuint attach_texture(const char* base,const char* filename);
	void detach_texture(GLuint id);
private:
	struct pimpl_t;
	pimpl_t* pimpl;
	art_mgr_t();
};

inline art_mgr_t* art_mgr() { return art_mgr_t::mgr(); }

class graphics_mgr_t {
public:
	static graphics_mgr_t* mgr();
	GLuint alloc_vbo();
	void load_vbo(GLuint id,GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage);
	GLuint alloc_texture();
	void load_texture_2D(GLuint id,SDL_Surface* image);
private:
	graphics_mgr_t();
};

inline graphics_mgr_t* graphics_mgr() { return graphics_mgr_t::mgr(); }

#endif //__GRAPHICS_HPP__


