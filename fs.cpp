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
#include <iostream>
#include <map>

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
	std::string read_all();
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

std::string FILE_stream_t::read_all() {
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
	return ret;
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
	pimpl_t(fs_t& fs_,const std::string& dd): fs(fs_), data_directory(dd) {}
	fs_t& fs;
	const std::string data_directory;
	typedef std::map<std::string,std::string> resolve_t;
	resolve_t resolved;
	std::string resolve(const std::string& path);
	strings_t list(const std::string& path,bool (*test)(const char*));
};

static bool _starts_with(const std::string& str,const std::string& sub) {
	return (str.find(sub)==0);
}

static std::string _path_tok(const std::string& path,size_t start) {
	size_t end = start;
	bool term = false;
	for(; end<path.size() && !term; end++)
		term = ((path[end]=='/')||(path[end]=='\\'));
	if(term)
		return std::string(path,start,end-start-1)+'/'; // standard terminator
	return std::string(path,start,end-start);
}

std::string fs_t::pimpl_t::resolve(const std::string& path) {
	if(!_starts_with(path,data_directory))
		panic("expecting "<<path<<" to be in "<<data_directory);
	resolve_t::iterator i=resolved.find(path);
	data_error(path << " does not exist!");
}

#ifndef WIN32
static int _one(const struct dirent64 *d) { return 1; }
#endif

strings_t fs_t::pimpl_t::list(const std::string& path,bool (*test)(const char*)) {
	strings_t list;
#ifdef WIN32
	if(DIR *dir = opendir(path.c_str())) {
		while(dirent* entry = readdir(dir)) {
			if((entry->d_name[0] != '.') && test(fs.join(path,entry->d_name).c_str()))
				list.push_back(entry->d_name);
		}
		closedir(dir);
	} else
		data_error("list_dirs("<<path<<"): cannot open");
#else
	struct dirent64 **eps;
	if(int n = scandir64(path.c_str(),&eps,_one,alphasort64)) {
		if(-1 == n) c_error("list_dirs("<<path<<")");
		for(int i=0; i<n; i++) {
			if((eps[i]->d_name[0] != '.') && test(fs.join(path,eps[i]->d_name).c_str()))
				list.push_back(eps[i]->d_name);
			free(eps[i]);
		}
		free(eps);
	}
#endif
	return list;
}

std::string fs_t::canocial(const std::string& path) const {
	std::string src, cano;
	if(_starts_with(path,pimpl->data_directory))
		src = std::string(path,pimpl->data_directory.size());
	else
		src = path;
	// tidy up and .. and check its not breaking root
	size_t start = 0;
	std::vector<size_t> parts;
	while(true) {
		std::string part = _path_tok(src,start);
		start += part.size();
		if(part == "") break;
		if(part == "/") {
			std::cerr << "in "<<path<<", skipping /" << std::endl;
			continue;
		} else if(part == "../") {
			//### find a test case and we can support it
			data_error(".. not supported in "<<path);
		} else if(part[0] == '.')
			data_error("it is not policy to support hidden files in "<<path);
		parts.push_back(start-part.size()); 
		cano += part;
	}
	return pimpl->data_directory+cano;
}

static bool _stat(const char* path,struct stat& s) {
	std::string tmp;
	if(path && *path && (path[strlen(path)-1] == '/')) { // trim any trailing / - win32 requires this
		tmp = path;
		tmp[strlen(path)-1] = 0;
		path = tmp.c_str();
	}
	if(stat(path,&s)) {
		if((ENOENT != errno)&&(ENOTDIR != errno))
			c_error("stat("<<path<<")");
		c_error("stat("<<path<<")");
		return false;
	}
	return true;
}

static bool _exists(const char* path) {
	struct stat s;
	return _stat(path,s);
}

bool fs_t::exists(const std::string& path) const { return _exists(canocial(path).c_str()); }

static bool _is_file(const char* path) {
	struct stat s;
	return _stat(path,s) && S_ISREG(s.st_mode);
}

bool fs_t::is_file(const std::string& path) const { return _is_file(canocial(path).c_str()); }

static bool _is_dir(const char* path) {
	struct stat s;
	return _stat(path,s) && S_ISDIR(s.st_mode);
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

std::string fs_t::get_body(const std::string& path) {
	fs_file_t::ptr_t file(get(path));
	istream_t::ptr_t istream(file->reader());
	return istream->read_all();
}

strings_t fs_t::list_dirs(const std::string& path) {
	return pimpl->list(canocial(path),_is_dir);
}

strings_t fs_t::list_files(const std::string& path) {
	return pimpl->list(canocial(path),_is_file);
}

fs_t::fs_t(const std::string& data_directory): pimpl(new pimpl_t(*this,data_directory)) {}

fs_t::~fs_t() { delete pimpl; }

fs_t* fs_t::create(const std::string& dd) {
	std::string data_directory;
	if(!dd.size())
		data_directory = ".";
	else if((dd[dd.size()-1] == '/') || (dd[dd.size()-1] == '\\'))
		data_directory = std::string(dd,0,dd.size()-1);
	else
		data_directory = dd;
	data_directory += '/';
	if(!_is_dir(data_directory.c_str())) data_error(dd << " is not a directory");
	return new fs_t(data_directory);
}

fs_t* fs_t::settings = NULL; // owned by auto_ptr in glestng.cpp

