/*
 g3d.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __G3D_HPP__
#define __G3D_HPP__

#include <vector>

#include "3d.hpp"
#include "utils.hpp"

class model_g3d_t {
public:
	model_g3d_t(file_stream_t& in);
	virtual ~model_g3d_t();
	const bounds_t& get_bounds() const { return bounds; }
private:
	struct mesh_t;
	typedef std::vector<mesh_t*> meshes_t;
	meshes_t meshes;
	bounds_t bounds;
	void load_v3(file_stream_t& in);
	void load_v4(file_stream_t& in);
};

#endif //__G3D_HPP__

