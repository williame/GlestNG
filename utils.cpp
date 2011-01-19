/*
 utils.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdio.h>
#include <string>

#include "utils.hpp"
#include "error.hpp"

class FILE_stream_t: private istream_t {
	friend class istream_t;
public:
	FILE_stream_t(const char* filename,const char* mode);
	~FILE_stream_t();
	void read(void* dest,size_t bytes);
	void write(const void* src,size_t bytes);
	std::ostream& repr(std::ostream& out) const;
private:
	const std::string filename;
	FILE* const f;
};

FILE_stream_t::FILE_stream_t(const char* filename_,const char* mode):
	filename(filename_),
	f(fopen(filename_,mode))
{
	if(!f) data_error("could not open "<<filename<<" for "<<mode);
}

FILE_stream_t::~FILE_stream_t() {
	fclose(f);
}

void FILE_stream_t::read(void* dest,size_t bytes) {
	if(bytes != fread(dest,1,bytes,f))
		data_error("could not read "<<bytes<<" from "<<filename);
}
	
void FILE_stream_t::write(const void* src,size_t bytes) {
	if(bytes != fwrite(src,1,bytes,f))
		data_error("could not write "<<bytes<<" to "<<filename);
}

std::ostream& FILE_stream_t::repr(std::ostream& out) const {
	return out << filename;
}

istream_t* istream_t::open_file(const char* filename) {
	return new FILE_stream_t(filename,"r");
}

