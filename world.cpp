/*
 world.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "world.hpp"

perf_t::perf_t() {
	reset();
}

void perf_t::reset() {
	for(int i=0; i<NUM_SLOTS; i++)
		slot[i] = 0;
	idx = 0;
}

void perf_t::tick(unsigned now) {
	if(++idx == NUM_SLOTS)
		idx = 0;
	slot[idx] = now;
}
	
double perf_t::per_second(unsigned now) const {
	enum { WINDOW = MAX_SECONDS*1000 };
	const unsigned first = slot[idx<(NUM_SLOTS-1)?idx+1:0];
	const unsigned len = (now-first);
	int count = 0;
	for(int i=0; i<NUM_SLOTS; i++)
		if(slot[i])
			count++;
	const double multi = len/1000.0;
	return count/multi;
}

static unsigned _now;

unsigned now() { return _now; }

void set_now(unsigned now) {
	_now = now;
}
