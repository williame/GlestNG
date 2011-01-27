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
	mesh_t(model_g3d_t& g3d,std::string name,GLuint index_count);
	~mesh_t();
	void load_vn(istream_t& in,uint32_t frame_count,uint32_t vertex_count);
	void load_t(istream_t& in,uint32_t frame_count,uint32_t vertex_count);
	void load_i(istream_t& in,uint32_t index_count);
	void draw(int frame);
	model_g3d_t& g3d;
	const std::string name;
	std::vector<GLuint> vertices, normals, tex_coords;
	GLuint index_count;
	GLuint indices;
	struct tex_coord_t {
		tex_coord_t() {}
		tex_coord_t(float x_,float y_): x(x_), y(y_) {}
		float x, y;
	};
	enum texture_t {
		DIFFUSE,
		SPECULAR,
		NORMAL,
		REFLECTION,
		COLORMASK,
		TEXTURE_COUNT
	};
	GLuint textures[TEXTURE_COUNT];
};

model_g3d_t::mesh_t::mesh_t(model_g3d_t& g3d_,std::string n,GLuint i):
	g3d(g3d_), name(n), index_count(i), indices(0) {
	memset(textures,0,sizeof(textures));
}

model_g3d_t::mesh_t::~mesh_t() {
	//### free VBOs and release (shared) textures
}

void model_g3d_t::mesh_t::load_vn(istream_t& in,uint32_t frame_count,uint32_t vertex_count) {
	fixed_array_t<vec_t> vec(vertex_count);
	for(int pass=0; pass<2; pass++)
		for(uint32_t f=0; f<frame_count; f++) {
			vec.clear();
			while(!vec.full())
				vec.append(vec_t(in.float32(),in.float32(),in.float32()));
			GLuint vbo = graphics()->alloc_vbo();
			graphics()->load_vbo(vbo,
				GL_ARRAY_BUFFER,
				vertex_count*sizeof(vec_t),
				vec.ptr(),
				GL_STATIC_DRAW);
			if(pass)
				normals.push_back(vbo);
			else {
				for(uint32_t v=0; v<vertex_count; v++)
					g3d.bounds.bounds_include(vec[v]);
				vertices.push_back(vbo);
			}
		}
}

void model_g3d_t::mesh_t::load_t(istream_t& in,uint32_t frame_count,uint32_t vertex_count) {
	fixed_array_t<mesh_t::tex_coord_t> tex(vertex_count);
	for(uint32_t f=0; f<frame_count; f++) {
		while(!tex.full())
			tex.append(mesh_t::tex_coord_t(in.float32(),in.float32()));
		GLuint vbo = graphics()->alloc_vbo();
		graphics()->load_vbo(vbo,
			GL_ARRAY_BUFFER,
			vertex_count*sizeof(mesh_t::tex_coord_t),
			tex.ptr(),
			GL_STATIC_DRAW);
		tex_coords.push_back(vbo);
	}
}

void model_g3d_t::mesh_t::load_i(istream_t& in,uint32_t index_count) {
	fixed_array_t<face_t> faces(index_count/3);
	while(!faces.full())
		faces.append(face_t(in.uint32(),in.uint32(),in.uint32()));
	indices = graphics()->alloc_vbo();
	graphics()->load_vbo(indices,
		GL_ELEMENT_ARRAY_BUFFER,
		index_count*sizeof(face_t),
		faces.ptr(),
		GL_STATIC_DRAW);
}

void model_g3d_t::mesh_t::draw(int frame) {
	glBindBuffer(GL_ARRAY_BUFFER,vertices[frame]);
	glVertexPointer(3,GL_FLOAT,0,NULL);
	glBindBuffer(GL_ARRAY_BUFFER,normals[frame]);
	glNormalPointer(GL_FLOAT,0,NULL);
	if(textures[DIFFUSE]) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER,(frame<(int)tex_coords.size()? tex_coords[frame]: tex_coords[0]));
		glTexCoordPointer(2,GL_FLOAT,0,NULL);
		glBindTexture(GL_TEXTURE_2D,textures[DIFFUSE]);
		glColor4f(1,1,1,1);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices);
	glDrawElements(GL_TRIANGLES,index_count,GL_UNSIGNED_INT,NULL);
	if(textures[DIFFUSE]) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D,0);
	}
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
	std::cout << in << " is V3!" << std::endl;
	const uint32_t mesh_count = in.uint32();
	for(unsigned m=0; m<mesh_count; m++) {
		const uint32_t frame_count = in.uint32();
		const uint32_t normal_count = in.uint32();
		const uint32_t texCoord_count = in.uint32();
		const uint32_t color_count = in.uint32();
		const uint32_t vertex_count = in.uint32();
		const uint32_t index_count = in.uint32();
		const uint32_t properties = in.uint32();
		const std::string texture = fs()->join(in.path(),in.fixed_str<64>());
		if(normal_count != vertex_count) 
			std::cerr << in << " has "<<normal_count<<" normals but "<<vertex_count<<" vertices" << std::endl;
		if(index_count%3) data_error(in << " bad number of indices: " << index_count);
		if(meshes.size() && (frame_count != meshes[0]->vertices.size()))
			data_error(in << " has meshes this differing frame-counts");
		mesh_t* mesh = new mesh_t(*this,"",index_count);
		meshes.push_back(mesh);
		const bool has_textures = (0==(properties&1)); 
		if(has_textures) {
			mesh->textures[mesh_t::DIFFUSE] = graphics()->alloc_texture(texture.c_str());
			if(texture.size()>4) {
				std::string norm(texture,0,texture.size()-4);
				norm += "_normal";
				norm += texture.c_str()+texture.size()-4;
				try {
					if(fs()->is_file(norm))
						mesh->textures[mesh_t::NORMAL] = graphics()->alloc_texture(norm.c_str());
				} catch(data_error_t* de) {
					delete de;
				}
			}
		}
		mesh->load_vn(in,frame_count,vertex_count);
		if(has_textures) {
			if(texCoord_count != frame_count)
				std::cerr << in << " has "<<texCoord_count<<"text coords but "<<frame_count<<" frames" << std::endl;
			mesh->load_t(in,texCoord_count,vertex_count);
		}
		in.skip(16);
		in.skip(16*(color_count-1));
		mesh->load_i(in,index_count);  
	}
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
		mesh_t* mesh = new mesh_t(*this,name,index_count);
		meshes.push_back(mesh);
		in.skip(8*4);
		const uint32_t properties __attribute__((unused)) = in.uint32(),
			textures = in.uint32();
		for(int t=0; t<mesh_t::TEXTURE_COUNT; t++)
		if((1<<t)&textures)
			mesh->textures[t] = graphics()->alloc_texture(
				fs()->join(in.path(),in.fixed_str<64>()).c_str());
		mesh->load_vn(in,frame_count,vertex_count);
		if(textures)
			mesh->load_t(in,frame_count,vertex_count);
		mesh->load_i(in,index_count);
	}
}

void model_g3d_t::draw(float dist_from_camera) {
	const int frame = (now()/100)%meshes[0]->vertices.size();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	for(meshes_t::iterator i=meshes.begin(); i!=meshes.end(); i++)
		(*i)->draw(frame);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}


