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

#define traceback(fmt,...) VALGRIND_PRINTF_BACKTRACE(fmt,__VA_ARGS__)

class glest_exception_t: public std::stringstream {
public:
	const std::string file;
	const int line;
	const std::string type;
protected:
	glest_exception_t(const char* f,int l,const char* t): 
		std::stringstream("",ios_base::out|ios_base::ate),
		file(f), line(l),
		type(t) {}
};

class panic_t: public glest_exception_t {
public:
	panic_t(const char* file,int line): glest_exception_t(file,line,"PANIC") {}
};

#define panic(X) { panic_t* panic = new panic_t(__FILE__,__LINE__); \
	*panic << X; \
	traceback("%s\n",panic->str().c_str()); \
	raise(SIGINT); \
	throw panic; }
	
#define assert(expr) if(!(expr)) panic(#expr);
	
class data_error_t: public glest_exception_t {
public:
	data_error_t(const char* file,int line): glest_exception_t(file,line,"DATA ERROR") {}
};

#define data_error(X) { data_error_t* data_error = new data_error_t(__FILE__,__LINE__); \
	*data_error << X; \
	traceback("%s\n",data_error->str().c_str()); \
	throw data_error; }
	
class graphics_error_t: public glest_exception_t {
public:
	graphics_error_t(const char* file,int line): glest_exception_t(file,line,"GRAPHICS ERROR") {}
};

#define graphics_error(X) { graphics_error_t* graphics_error = new graphics_error_t(__FILE__,__LINE__); \
	*graphics_error << X; \
	traceback("%s\n",graphics_error->str().c_str()); \
	throw graphics_error; }

inline std::ostream& operator<<(std::ostream& out,const glest_exception_t& e) {
	return out << '(' << e.type << ' ' << e.file << '#' << e.line << "): " << e.str();
}

inline std::ostream& operator<<(std::ostream& out,const glest_exception_t* e) {
	return out << *e;
}

#endif //__ERROR_HPP__

