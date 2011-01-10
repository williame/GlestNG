/*
 3d.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __3D_HPP__
#define __3D_HPP__

typedef float vec_t[3];

typedef float matrix_t[16];

inline vec_t& operator*(vec_t& v,const matrix_t& m) {
	return v;
}

#endif //__3D_HPP__

