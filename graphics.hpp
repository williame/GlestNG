/*
 graphics.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __GRAPHICS_HPP__
#define __GRAPHICS_HPP__

#include <memory>

#define USE_GL

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

class fs_file_t;

class graphics_t {
public:
	class mgr_t {
	public:
		// can add functions and properties here that only the mgr should be able to do
		virtual ~mgr_t();
	private:
		friend class graphics_t;
		mgr_t() {}
	};
	static std::auto_ptr<mgr_t> create();
	static graphics_t* graphics();
	virtual ~graphics_t();
	GLuint alloc_vbo();
	void load_vbo(GLuint id,GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage);
	GLuint alloc_texture(fs_file_t& file); // PREFERRED METHOD, SHARED
	// raw stuff if you know what you're doing
	GLuint alloc_texture();
	void load_texture_2D(GLuint id,SDL_Surface* image);
	SDL_Surface* load_surface(fs_file_t& file);
private:
	graphics_t();
	struct pimpl_t;
	pimpl_t* pimpl;
};

inline graphics_t* graphics() { return graphics_t::graphics(); }

#endif //__GRAPHICS_HPP__


