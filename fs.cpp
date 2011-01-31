/*
 fs.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

#include "fs.hpp"
#include "error.hpp"

#define c_error(X) data_error(X << " (" << errno << ": " << strerror(errno) << ')')

class FILE_stream_t: public istream_t {
public:
	FILE_stream_t(fs_file_t& file,const char* filename,const char* mode);
	~FILE_stream_t();
	const char* path() const { return filename.c_str(); }
	void read(void* dest,size_t bytes);
	void write(const void* src,size_t bytes);
	char* read_allz();
	std::ostream& repr(std::ostream& out) const;
private:
	const std::string filename;
	FILE* const f;
};

FILE_stream_t::FILE_stream_t(fs_file_t& file,const char* filename_,const char* mode):
	istream_t(file),
	filename(filename_),
	f(fopen(filename_,mode))
{
	if(!f) c_error("could not open "<<filename<<" for "<<mode);
}

FILE_stream_t::~FILE_stream_t() {
	fclose(f);
}

char* FILE_stream_t::read_allz() {
	std::string ret;
	bool done = false;
	for(;;) {
		char buf[1025];
		const size_t bytes = fread(buf,1,sizeof(buf)-1,f);
		if(bytes < 0)
			c_error("could not read all from "<<filename);
		if(!done) {
			buf[bytes] = 0;
			const size_t prev = ret.size();
			ret += buf;
			done = (prev+bytes != ret.size()); // there was a premature \0
		}
		if(bytes < sizeof(buf)-1)
			break;
	}
	return strdup(ret.c_str());
}

void FILE_stream_t::read(void* dest,size_t bytes) {
	const size_t ret = fread(dest,1,bytes,f);
	if(ret < 0)
		c_error("could not read "<<bytes<<" from "<<filename);
	if(bytes != ret)
		data_error("could not read "<<bytes<<" from "<<filename<<" ("<<ret<<'@'<<ftell(f)<<')');
}
	
void FILE_stream_t::write(const void* src,size_t bytes) {
	const size_t ret = fwrite(src,1,bytes,f);
	if(ret < 0)
		c_error("could not write "<<bytes<<" to "<<filename);
	if(bytes != ret)
		data_error("could not write "<<bytes<<" to "<<filename<<" ("<<ret<<'@'<<ftell(f)<<')');
}

std::ostream& FILE_stream_t::repr(std::ostream& out) const {
	return out << filename;
}

class fs_handle_impl_t: public fs_file_t {
public:
	fs_handle_impl_t(fs_t& fs,const char* path): fs_file_t(fs), p(strdup(path)) {}
	~fs_handle_impl_t() { free(p); }
	std::auto_ptr<istream_t> reader() { return std::auto_ptr<istream_t>(new FILE_stream_t(*this,p,"r")); }
	const char* path() const { return p; }
	const char* name() const {
		if(const char* n = strrchr(p,'/'))
			return n+1; // skip the actual separator
		return p; // consider whole path to be a single name
	}
	const char* ext() const {
		if(const char* e = strrchr(name(),'.'))
			return e+1;
		return ""; // no extension
	}
private:
	char* p;
};


struct fs_t::pimpl_t {
	pimpl_t(const std::string& dd): data_directory(dd) {}
	const std::string data_directory;
};

static bool _starts_with(const std::string& str,const std::string& sub) {
	return (str.find(sub)==0);
}

std::string fs_t::canocial(const std::string& path) const {
	//### tidy up and .. and check its not breaking root
	if(_starts_with(path,pimpl->data_directory))
		return path;
	return pimpl->data_directory+'/'+path;
}

static bool _is_file(const char* path) {
	struct stat s;
	if(stat(path,&s)) c_error("is_file("<<path<<")");
	return S_ISREG(s.st_mode);
}

bool fs_t::is_file(const std::string& path) const { return _is_file(canocial(path).c_str()); }

static bool _is_dir(const char* path) {
	struct stat s;
	if(stat(path,&s)) c_error("is_dir("<<path<<")");
	return S_ISDIR(s.st_mode);
}

bool fs_t::is_dir(const std::string& path) const { return _is_dir(canocial(path).c_str()); }

std::string fs_t::join(const std::string& path,const std::string& sub) const {
	if(_starts_with(sub,pimpl->data_directory))
		return canocial(sub);
	const char* s = sub.c_str();
	while((*s=='\\')||(*s=='/')) s++;
	if(is_dir(path))
		return canocial(path+'/'+s);
	return canocial(parent_directory(path)+'/'+s);
}

std::string fs_t::parent_directory(const std::string& path) const {
	const size_t ofs = path.find_last_of("/\\");
	if(ofs == (size_t)std::string::npos)
		data_error(path<<" has no parent directory");
	return std::string(path,0,ofs);
}

fs_file_t* fs_t::get(const std::string& path) {
	return new fs_handle_impl_t(*this,canocial(path).c_str());
}

static int _one(const struct dirent64 *d) { return 1; }

fs_t::list_t fs_t::list_dirs(const std::string& path) {
	struct dirent64 **eps;
	fs_t::list_t dirs;
	if(int n = scandir64(canocial(path).c_str(),&eps,_one,alphasort64)) {
		if(-1 == n) c_error("list_dirs("<<path<<")");
		for(int i=0; i<n; i++) {
			if((eps[i]->d_name[0] != '.') && is_dir(join(path,eps[i]->d_name)))
				dirs.push_back(eps[i]->d_name);
			free(eps[i]);
		}
		free(eps);
	}
	return dirs;
}

fs_t::list_t fs_t::list_files(const std::string& path) {
	struct dirent64 **eps;
	fs_t::list_t files;
	if(int n = scandir64(canocial(path).c_str(),&eps,_one,alphasort64)) {
		if(-1 == n) c_error("list_files("<<path<<")");
		for(int i=0; i<n; i++) {
			if((eps[i]->d_name[0] != '.') && is_file(join(path,eps[i]->d_name)))
				files.push_back(eps[i]->d_name);
			free(eps[i]);
		}
		free(eps);
	}
	return files;
}

fs_t::fs_t(const std::string& data_directory): pimpl(new pimpl_t(data_directory)) {}

fs_t::~fs_t() { delete pimpl; }

fs_t* fs_t::create(const std::string& data_directory) {
	return new fs_t(data_directory);
}

