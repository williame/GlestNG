/*
 graphics.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __GRAPHICS_HPP__
#define __GRAPHICS_HPP__

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

class graphics_mgr_t {
public:
	static graphics_mgr_t* mgr();
	GLuint alloc_vbo(GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage);
private:
	graphics_mgr_t();
};

inline graphics_mgr_t* graphics_mgr() { return graphics_mgr_t::mgr(); }

#endif //__GRAPHICS_HPP__


