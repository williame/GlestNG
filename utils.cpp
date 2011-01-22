/*
 utils.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdio.h>
#include <string>
#include <stdlib.h>

#include "utils.hpp"
#include "error.hpp"

float randf() {
	return (float)rand()/RAND_MAX;
}
