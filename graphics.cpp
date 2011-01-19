/*
 graphics.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "error.hpp"
#include "graphics.hpp"

graphics_mgr_t* graphics_mgr_t::mgr() {
	static graphics_mgr_t* singleton = NULL;
	if(!singleton)
		singleton = new graphics_mgr_t();
	return singleton;
}

GLuint graphics_mgr_t::alloc_vbo() {
	GLuint buffer;
	glGenBuffers(1,&buffer);
	if(!buffer) graphics_error("could not get GL to allocate a VBO ID");
	return buffer;
}

void graphics_mgr_t::load_vbo(GLuint buffer,GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage) {
	if(!buffer) graphics_error("VBO handle not set");
	glBindBuffer(target,buffer);
	glBufferData(target,size,data,usage);
	glBindBuffer(target,0);
	//#### glCheckErrors
}

GLuint graphics_mgr_t::alloc_texture() {
	GLuint texture;
	glGenTextures(1,&texture);
	if(!texture)
		graphics_error("could not allocate a texture handle "<<glGetError());
	return texture;
}
	
void graphics_mgr_t::load_texture_2D(GLuint texture,SDL_Surface* image) {
	if(!texture)
		graphics_error("texture handle not set");
	if(image->w&(image->w-1))
		graphics_error("image width "<<image->w<<" is not a power of 2");
	if(image->h&(image->h-1))
		graphics_error("image height "<<image->h<<" is not a power of 2");
        GLenum texture_format;
        switch(image->format->BytesPerPixel) {
        case 4:
        		texture_format = (image->format->Rmask==0xff)? GL_RGBA: GL_BGRA;
        		break;
        	case 3:
        		texture_format = (image->format->Rmask==0xff)? GL_RGB: GL_BGR;
        		break;
        	default:
        		graphics_error("the image is not truecolor ("<<image->format->BytesPerPixel<<" Bpp)");
        }
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,
		image->format->BytesPerPixel,
		image->w,image->h,0,
		texture_format,
		GL_UNSIGNED_BYTE,image->pixels);
	glBindTexture(GL_TEXTURE_2D,0);
}

graphics_mgr_t::graphics_mgr_t() {}

