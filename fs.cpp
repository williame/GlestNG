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

#define c_error(X) data_error(X << " (" << errno << ": " << strerror(errno))

class FILE_stream_t: public istream_t {
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
	if(!f) c_error("could not open "<<filename<<" for "<<mode);
}

FILE_stream_t::~FILE_stream_t() {
	fclose(f);
}

void FILE_stream_t::read(void* dest,size_t bytes) {
	if(bytes != fread(dest,1,bytes,f))
		c_error("could not read "<<bytes<<" from "<<filename);
}
	
void FILE_stream_t::write(const void* src,size_t bytes) {
	if(bytes != fwrite(src,1,bytes,f))
		c_error("could not write "<<bytes<<" to "<<filename);
}

std::ostream& FILE_stream_t::repr(std::ostream& out) const {
	return out << filename;
}

class fs_t: public fs_mgr_t {
public:
	fs_t(const std::string& data_directory);
	bool is_file(const std::string& path) const;
	bool is_dir(const std::string& path) const;
	std::string canocial(const std::string& path) const;
	std::string join(const std::string& path,const std::string& sub) const;
	std::auto_ptr<istream_t> open(const std::string& path);
	list_t list_dirs(const std::string& path);
	list_t list_files(const std::string& path);
private:
	const std::string data_directory;
};

fs_t::fs_t(const std::string& d_d): data_directory(d_d)
{}

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

std::string fs_t::canocial(const std::string& path) const {
	//### tidy up and .. and check its not breaking root
	if(path.find(data_directory)==0)
		return path;
	return data_directory+'/'+path;
}

std::string fs_t::join(const std::string& path,const std::string& sub) const {
	return canocial(path+'/'+sub);
}

std::auto_ptr<istream_t> fs_t::open(const std::string& path) {
	return std::auto_ptr<istream_t>(new FILE_stream_t(canocial(path).c_str(),"r"));
}

static int _one(const struct dirent64 *d) { return 1; }

fs_mgr_t::list_t fs_t::list_dirs(const std::string& path) {
	struct dirent64 **eps;
	fs_mgr_t::list_t dirs;
	if(int n = scandir64(canocial(path).c_str(),&eps,_one,alphasort64)) {
		if(-1 == n) c_error("list_dirs("<<path<<")");
		for(int i=0; i<n; i++)
			if((eps[i]->d_name[0] != '.') && is_dir(join(path,eps[i]->d_name)))
				dirs.push_back(eps[i]->d_name);
		free(eps);
	}
	return dirs;
}

fs_mgr_t::list_t fs_t::list_files(const std::string& path) {
	struct dirent64 **eps;
	fs_mgr_t::list_t files;
	if(int n = scandir64(canocial(path).c_str(),&eps,_one,alphasort64)) {
		if(-1 == n) c_error("list_files("<<path<<")");
		for(int i=0; i<n; i++)
			if((eps[i]->d_name[0] != '.') && is_file(join(path,eps[i]->d_name)))
				files.push_back(eps[i]->d_name);
		free(eps);
	}
	return files;
}

static fs_mgr_t* _fs = NULL;

void fs_mgr_t::create(const std::string& data_directory) {
	if(_fs) panic("file system already created");
	_fs = new fs_t(data_directory);
}

fs_mgr_t* fs_mgr_t::get_fs() {
	if(!_fs) panic("file system not created");
	return _fs;
}

