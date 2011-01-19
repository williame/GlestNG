/*
 g3d.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "g3d.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "world.hpp"

struct model_g3d_t::mesh_t {
	mesh_t(std::string name,GLuint index_count);
	~mesh_t();
	void draw(int frame);
	const std::string name;
	std::vector<GLuint> vertices, normals, tex_coords;
	GLuint index_count;
	GLuint indices;
	struct tex_coord_t {
		tex_coord_t() {}
		tex_coord_t(float x_,float y_): x(x_), y(y_) {}
		float x, y;
	};
};

model_g3d_t::mesh_t::mesh_t(std::string n,GLuint i):
	name(n), index_count(i), indices(0)
{}

model_g3d_t::mesh_t::~mesh_t() {
	//### free VBOs
}

void model_g3d_t::mesh_t::draw(int frame) {
        glBindBuffer(GL_ARRAY_BUFFER,vertices[frame]);
        glVertexPointer(3,GL_FLOAT,0,NULL);
        glBindBuffer(GL_ARRAY_BUFFER,normals[frame]);
        glNormalPointer(GL_FLOAT,0,NULL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices);
        glDrawElements(GL_TRIANGLES,index_count,GL_UNSIGNED_INT,NULL);
}

model_g3d_t::model_g3d_t(istream_t& in) {
	const uint32_t ver = in.uint32();
	// note the endian here is little endian
	if(((ver&0xff)!='G')||(((ver>>8)&0xff)!='3')||(((ver>>16)&0xff)!='D'))
		data_error(in << " (" <<std::hex << ver << ") is not a G3D model");
	switch(ver>>24) {
	case 3: load_v3(in); break;
	case 4: load_v4(in); break;
	default: data_error(in << " is not a supported G3D model version (" << (ver&0xff) << ")");
	}
        bounds.bounds_fix();
	std::cout << in << " " << bounds << std::endl;
}

model_g3d_t::~model_g3d_t() {
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		delete *i;
}

void model_g3d_t::load_v3(istream_t& in) {
        data_error(in << " G3D v3 not implemented");
}

void model_g3d_t::load_v4(istream_t& in) {
        const uint16_t mesh_count = in.uint16();
        if(!mesh_count) data_error(in << " has no meshes");
        if(in.byte()) data_error(in << " is not a G3D mtMorphMesh");
        for(int16_t i=0; i<mesh_count; i++) {
        		const std::string name = in.fixed_str<64>();
        		const uint32_t frame_count = in.uint32(),
        			vertex_count = in.uint32(),
        			index_count = in.uint32();
        		if(index_count%3) data_error(in << " bad number of indices: " << index_count);
        		if(meshes.size() && (frame_count != meshes[0]->vertices.size()))
        			data_error(in << " has meshes this differing frame-counts");
        		mesh_t* mesh = new mesh_t(name,index_count);
        		meshes.push_back(mesh);
        		in.skip(8*4);
        		const uint32_t properties = in.uint32(),
        			textures = in.uint32();
		for(int t=0; t<5; t++)
			if((1<<t)&textures) {
				std::cout << in << "\n\t:skipping " << t << ": " << in.fixed_str<64>() << std::endl;
			}
		fixed_array_t<vec_t> vec(vertex_count);
		for(int pass=0; pass<2; pass++)
			for(uint32_t f=0; f<frame_count; f++) {
				vec.clear();
				while(!vec.full())
					vec.append(vec_t(in.float32(),in.float32(),in.float32()));
				GLuint vbo = graphics_mgr()->alloc_vbo();
				graphics_mgr()->load_vbo(vbo,
					GL_ARRAY_BUFFER,
					vertex_count*sizeof(vec_t),
					vec.ptr(),
					GL_STATIC_DRAW);
				if(pass)
					mesh->normals.push_back(vbo);
				else {
					for(uint32_t v=0; v<vertex_count; v++)
						bounds.bounds_include(vec[v]);
					mesh->vertices.push_back(vbo);
				}
			}
        		if(textures) {
        			fixed_array_t<mesh_t::tex_coord_t> tex(vertex_count);
			for(uint32_t f=0; f<frame_count; f++) {
				while(!tex.full())
					tex.append(mesh_t::tex_coord_t(in.float32(),in.float32()));
        				GLuint vbo = graphics_mgr()->alloc_vbo();
        				graphics_mgr()->load_vbo(vbo,
						GL_ARRAY_BUFFER,
						vertex_count*sizeof(mesh_t::tex_coord_t),
						tex.ptr(),
						GL_STATIC_DRAW);
        				mesh->tex_coords.push_back(vbo);
        			}
        		}
        		fixed_array_t<face_t> faces(index_count/3);
        		while(!faces.full())
        			faces.append(face_t(in.uint32(),in.uint32(),in.uint32()));
        		mesh->indices = graphics_mgr()->alloc_vbo();
        		graphics_mgr()->load_vbo(mesh->indices,
        			GL_ELEMENT_ARRAY_BUFFER,
        			index_count*sizeof(face_t),
        			faces.ptr(),
        			GL_STATIC_DRAW);
        }
}

void model_g3d_t::draw(float dist_from_camera) {
	glColor3b(127,0,0);
	const int frame = (now()/100)%meshes[0]->vertices.size();
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		(*i)->draw(frame);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
}


