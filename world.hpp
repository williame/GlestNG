/*
 world.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include <inttypes.h>

/*** GAME TIME
 Game time is in milliseconds since game-start */

class perf_t {
public:
	perf_t();
	void reset();
	void tick(unsigned now);
	double per_second(unsigned now) const;
private:
	enum { MAX_SECONDS = 3, NUM_SLOTS = 64 };
	unsigned slot[NUM_SLOTS];
	int idx;
};

unsigned now();
void set_now(unsigned now);

#endif //__WORLD_HPP__

