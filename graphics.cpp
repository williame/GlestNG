/*
 graphics.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <assert.h>
#include "graphics.hpp"

graphics_mgr_t* graphics_mgr_t::mgr() {
	static graphics_mgr_t* singleton = NULL;
	if(!singleton)
		singleton = new graphics_mgr_t();
	return singleton;
}

GLuint graphics_mgr_t::alloc_vbo(GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage) {
	GLuint buffer;
	glGenBuffers(1,&buffer);
	assert(buffer);
	glBindBuffer(target,buffer);
	glBufferData(target,size,data,usage);
	return buffer;
}

graphics_mgr_t::graphics_mgr_t() {}

