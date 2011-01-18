/*
 error.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <sstream>

class panic_t: public std::stringstream {
public:
	panic_t(): std::stringstream("GlestNG PANIC: ",ios_base::out|ios_base::ate) {}
};

#define panic(X) { panic_t* panic = new panic_t(); \
	*panic << __FILE__ << '#' << __LINE__<<" "<<X; \
	throw panic; }
	
class data_error_t: public std::stringstream {
public:
	data_error_t(): std::stringstream("An error occurred: ",ios_base::out|ios_base::ate) {}
};

#define data_error(X) { data_error_t* data_error = new data_error_t(); \
	*data_error << '(' << __FILE__ << '#' << __LINE__<<") "<<X; \
	throw data_error; }
	
inline std::ostream& operator<<(std::ostream& out,std::stringstream* p) {
	out << p->str();
	return out;
}

#endif //__ERROR_HPP__

