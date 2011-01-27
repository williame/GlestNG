/*
 graphics.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <iostream>
#include <map>

#include "error.hpp"
#include "graphics.hpp"
#include "fs.hpp"

struct graphics_t::pimpl_t {
	typedef std::map<std::string,GLuint> textures_t;
	textures_t textures;
};

static graphics_t* singleton = NULL;

graphics_t::mgr_t::~mgr_t() {
	delete singleton;
}

std::auto_ptr<graphics_t::mgr_t> graphics_t::create() {
	if(singleton) panic("graphics singleton already exists");
	singleton = new graphics_t();
	return std::auto_ptr<mgr_t>(new mgr_t());
}

graphics_t* graphics_t::graphics() {
#ifndef NDEBUG // else let the OS pick up the pieces
	if(!singleton) panic("graphics singleton does not exist");
#endif
	return singleton;
}

graphics_t::~graphics_t() {
	if(this != singleton) panic("WTF this is not the graphics singleton");
	singleton = NULL;
	delete pimpl;
}

graphics_t::graphics_t(): pimpl(new pimpl_t()) {}

GLuint graphics_t::alloc_vbo() {
	GLuint buffer;
	glGenBuffers(1,&buffer);
	if(!buffer) graphics_error("could not get GL to allocate a VBO ID");
	return buffer;
}

void graphics_t::load_vbo(GLuint buffer,GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage) {
	if(!buffer) graphics_error("VBO handle not set");
	glBindBuffer(target,buffer);
	glBufferData(target,size,data,usage);
	glBindBuffer(target,0);
	//#### glCheckErrors
}

GLuint graphics_t::alloc_texture(fs_handle_t& file) {
	pimpl_t::textures_t::const_iterator i = pimpl->textures.find(file.path());
	if(i != pimpl->textures.end())
		return i->second;
	SDL_Surface* surface = load_surface(file);
	GLuint texture = 0;
	try {
		texture = graphics()->alloc_texture();
		load_texture_2D(texture,surface);
		std::cout << "(loaded texture "<<file<<" "<<surface->w<<'x'<<surface->h<<")" << std::endl;
		SDL_FreeSurface(surface); surface = NULL;
		pimpl->textures[file.path()] = texture;
		return texture;
	} catch(...) {
		std::cerr << "Error loading: "<<file<<std::endl;
		SDL_FreeSurface(surface);
		glDeleteTextures(1,&texture);
		throw;
	}
	return texture;
}

GLuint graphics_t::alloc_texture() {
	GLuint texture;
	glGenTextures(1,&texture);
	if(!texture)
		graphics_error("could not allocate a texture handle "<<glGetError());
	return texture;
}
	
void graphics_t::load_texture_2D(GLuint texture,SDL_Surface* image) {
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

SDL_Surface* graphics_t::load_surface(fs_handle_t& file) {
	std::cout << "(loading texture "<<file<<")"<<std::endl;
	if(SDL_Surface* bmp = SDL_LoadBMP(file.path()))
		return bmp;
	if(!strcmp(file.ext(),"tga")) {
		istream_t::ptr_t f(file.reader());
		if(f->byte()) graphics_error("TGA contains an ID: "<<file);
		const int8_t colourMapType __attribute__((unused)) = f->byte();
		const int8_t dataTypeCode = f->byte();
		const int16_t colourMapOrigin __attribute__((unused))= f->uint16();
		const int16_t colourMapLength __attribute__((unused))= f->uint16();
		const int8_t colourMapDepth __attribute__((unused))= f->byte();
		const int16_t xOrigin __attribute__((unused)) = f->uint16();
		const int16_t yOrigin __attribute__((unused)) = f->uint16();
		const int16_t width = f->uint16();
		const int16_t height = f->uint16();
		const int8_t bitsPerPixel = f->byte();
		if((bitsPerPixel!=8)&&(bitsPerPixel!=24)&&(bitsPerPixel!=32))
			graphics_error("unsupported TGA depth: "<<bitsPerPixel<<", "<<file);
		const int components = bitsPerPixel/8;
		const int8_t imageDescriptor __attribute__((unused)) = f->byte();
		const size_t bytes = width*height*components;
		enum type_t { UN_RGB=2, UN_BW=3 };
		if(UN_RGB == dataTypeCode) {
			SDL_Surface* tga = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,bitsPerPixel,0,0,0,0);
			if(!tga) graphics_error("could not create surface for "<<file<<", "<<SDL_GetError());
			f->read(tga->pixels,bytes);
			return tga;
		} else
			graphics_error("TGA type "<<dataTypeCode<<" not yet supported: "<<file);
	}
	graphics_error("Unsupported type: "<<file << " ("<<file.ext()<<')');
}

