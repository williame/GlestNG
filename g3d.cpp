/*
 g3d.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include "g3d.hpp"
#include "error.hpp"

struct model_g3d_t::mesh_t {
	mesh_t(std::string n,uint32_t index_count,bool has_tex);
	~mesh_t();
	const std::string name;
	struct frame_t {
		frame_t(uint32_t vertex_count);
		fixed_array_t<vec_t> vertices, normals;
		void load_vertices_normals(file_stream_t& in);
	};
	typedef std::vector<frame_t*> frames_t;
	frames_t frames;
	fixed_array_t<face_t> indices;
	struct tex_coord_t {
		float x, y;
	};
	fixed_array_t<tex_coord_t> tex_coords;
	void load_tex_coords(file_stream_t& in);
	void load_indices(file_stream_t& in);
};

model_g3d_t::mesh_t::mesh_t(std::string n,uint32_t index_count,bool has_tex):
	name(n),
	indices(index_count),
	tex_coords(has_tex?index_count:0)
{}

model_g3d_t::mesh_t::~mesh_t() {
	for(frames_t::iterator i=frames.begin(); i!=frames.end(); i++)
		delete *i;
}

void model_g3d_t::mesh_t::load_tex_coords(file_stream_t& in) {
	while(!tex_coords.full()) {
		tex_coord_t c = { in.r_float32(), in.r_float32() };
		tex_coords.append(c);
	}
}

void model_g3d_t::mesh_t::load_indices() {
	while(!indices.full()) {
		face_t f = { in.r_uint32(), in.r_uint32(), in.r_uint32() };
		indices.append(f);
	}
}

model_g3d_t::mesh_t::frame_t::frame_t(uint32_t vertex_count):
	vertices(vertex_count),
	normals(vertex_count)
{}

void model_g3d_t::mesh_t::frame_t::load_vertices_normals(file_stream_t& in) {
	while(!
}

model_g3d_t::model_g3d_t(file_stream_t& in) {
	const uint32_t ver = in.r_uint32();
	// note the endian here is little endian
	if(((ver&0xff)!='G')||(((ver>>8)&0xff)!='3')||(((ver>>16)&0xff)!='D'))
		data_error(in << " (" <<std::hex << ver << ") is not a G3D model");
	switch(ver>>24) {
	case 3: load_v3(in); break;
	case 4: load_v4(in); break;
	default: data_error(in << " is not a supported G3D model version (" << (ver&0xff));
	}
}

model_g3d_t::~model_g3d_t() {
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		delete *i;
}

void model_g3d_t::load_v3(file_stream_t& in) {
        data_error(in << " G3D v3 not implemented");
}

void model_g3d_t::load_v4(file_stream_t& in) {
        const uint16_t mesh_count = in.r_uint16();
        if(in.r_byte()) data_error(in << " is not a G3D mtMorphMesh");
        for(int16_t i=0; i<mesh_count; i++) {
        		const std::string name = in.r_fixed_str<64>();
        		const uint32_t frame_count = in.r_uint32(),
        			vertex_count = in.r_uint32(),
        			index_count = in.r_uint32();
        		in.r_skip(8*4);
        		const uint32_t properties = in.r_uint32(),
        			textures = in.r_uint32();
		for(int t=0; t<5; t++)
			if((1<<t)&textures) {
				std::cout << in << "\n\t:skipping " << t << ": " << in.r_fixed_str<64>() << std::endl;
			}
        		mesh_t* mesh = new mesh_t(name,index_count,textures);
        		meshes.push_back(mesh);
        		for(int f=0; f<frame_count; f++) {
        			mesh_t::frame_t* frame = new mesh_t::frame_t(
        		}
        		if(textures)
        			mesh->load_tex_coords(in);
        		mesh->load_indices(in);
        }
}

