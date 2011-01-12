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

GLuint graphics_mgr_t::alloc_2D(SDL_Surface* image) {
	if(image->w&(image->w-1)) {
		fprintf(stderr,"error: image width %d is not a power of 2\n",image->w);
		return 0;
	}
	if(image->h&(image->h-1)) {
		fprintf(stderr,"error: image height %d is not a power of 2\n",image->h);
		return 0;
	}
        GLenum texture_format;
        switch(image->format->BytesPerPixel) {
        case 4:
        		texture_format = (image->format->Rmask==0xff)? GL_RGBA: GL_BGRA;
        		break;
        	case 3:
        		texture_format = (image->format->Rmask==0xff)? GL_RGB: GL_BGR;
        		break;
        	default:
                fprintf(stderr,"error: the image is not truecolor (%d Bpp)\n",image->format->BytesPerPixel);
                return 0;
        }
        GLuint texture;
	glGenTextures(1,&texture);
	if(!texture) {
		fprintf(stderr,"error: could not allocate a texture handle %d\n",glGetError());
		return 0;
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
	return texture;
}

graphics_mgr_t::graphics_mgr_t() {}

