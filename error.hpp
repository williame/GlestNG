/*
 error.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <sstream>
#include <csignal>
#include "valgrind.h"

class panic_t: public std::stringstream {
public:
	panic_t(): std::stringstream("GlestNG PANIC: ",ios_base::out|ios_base::ate) {}
};

#define panic(X) { panic_t* panic = new panic_t(); \
	*panic << __FILE__ << '#' << __LINE__<<" "<<X; \
	VALGRIND_PRINTF_BACKTRACE("%s\n",panic->str().c_str()); \
	raise(SIGINT); \
	throw panic; }
	
class data_error_t: public std::stringstream {
public:
	data_error_t(): std::stringstream("An error occurred: ",ios_base::out|ios_base::ate) {}
};

#define data_error(X) { data_error_t* data_error = new data_error_t(); \
	*data_error << '(' << __FILE__ << '#' << __LINE__<<") "<<X; \
	VALGRIND_PRINTF_BACKTRACE("%s\n",data_error->str().c_str()); \
	throw data_error; }
	
class graphics_error_t: public std::stringstream {
public:
	graphics_error_t(): std::stringstream("A graphics error occurred: ",ios_base::out|ios_base::ate) {}
};

#define graphics_error(X) { graphics_error_t* graphics_error = new graphics_error_t(); \
	*graphics_error << '(' << __FILE__ << '#' << __LINE__<<") "<<X; \
	VALGRIND_PRINTF_BACKTRACE("%s\n",graphics_error->str().c_str()); \
	throw graphics_error; }

inline std::ostream& operator<<(std::ostream& out,std::stringstream* p) {
	out << p->str();
	return out;
}

#endif //__ERROR_HPP__

