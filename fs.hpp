/*
 fs.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __FS_HPP__
#define __FS_HPP__

/** the idea behind this abstraction is that we can create a unionfs that
  can load content out of zipfiles.  At the moment files are identified by
  path, but with network games they will also be identified by ZIP CRCs
  and multiple versions distinct by CRC will be supported.
*/

#include <ostream>
#include <vector>
#include <memory>
#include <inttypes.h>

class fs_t;
class fs_file_t;

class istream_t {
public:
	typedef std::auto_ptr<istream_t> ptr_t;
	virtual ~istream_t() {}
	inline uint8_t byte();
	inline uint16_t uint16();
	inline uint32_t uint32();
	inline float float32();
	inline void skip(size_t n);
	static istream_t* open_file(const char* filename);
	virtual void read(void* dest,size_t bytes) = 0;
	template<int N> std::string fixed_str();
	virtual char* read_allz() = 0;
	virtual std::ostream& repr(std::ostream& out) const = 0;
	fs_file_t& file() { return _file; }
	const fs_file_t& file() const { return _file; }
	inline fs_t& fs();
	inline const fs_t& fs() const;
protected:
	istream_t(fs_file_t& file): _file(file) {}
private:
	fs_file_t& _file;
	template<typename T> T _r();
};

class fs_handle_t {
public:
	fs_t& fs() { return _fs; }
	const fs_t& fs() const { return _fs; }
protected:
	fs_handle_t(fs_t& fs): _fs(fs) {}
private:
	fs_t& _fs;
};

class fs_file_t: public fs_handle_t {
public:
	typedef std::auto_ptr<fs_file_t> ptr_t; // are you sure?
	virtual ~fs_file_t() {}
	virtual istream_t::ptr_t reader() = 0;
	virtual const char* path() const = 0;
	virtual const char* name() const = 0;
	virtual const char* ext() const = 0;
	inline std::string rel(const std::string& rel) const;
protected:
	fs_file_t(fs_t& fs): fs_handle_t(fs) {}
};

class fs_t {
public:
	static fs_t* create(const std::string& data_directory);
	virtual ~fs_t();
	bool is_file(const std::string& path) const;
	bool is_dir(const std::string& path) const;
	std::string canocial(const std::string& path) const;
	std::string join(const std::string& path,const std::string& sub) const;
	std::string parent_directory(const std::string& path) const;
	fs_file_t* get(const std::string& path);
	fs_file_t* get(const istream_t& parent,const std::string& rel);
	fs_file_t* get(const std::string& parent,const std::string& rel);
	typedef std::vector<std::string> list_t;
	list_t list_dirs(const std::string& path);
	list_t list_files(const std::string& path);
private:
	fs_t(const std::string& data_directory);
	struct pimpl_t;
	friend struct pimpl_t;
	pimpl_t* pimpl;
};

inline std::ostream& operator<<(std::ostream& out,const istream_t& repr) {
	return repr.repr(out);
}

inline std::ostream& operator<<(std::ostream& out,const fs_file_t& repr) {
	return out << repr.path();
}

template<int N> std::string istream_t::fixed_str() {
	char v[N+1];
	read(v,N);
	v[N] = 0;
	return std::string(v);
}

template<typename T> T istream_t::_r() {
	T v;
	read(&v,sizeof(T));
	return v;
}

// all source data and target platforms are little-endian
inline uint8_t istream_t::byte() { return _r<uint8_t>(); }
inline uint16_t istream_t::uint16() { return _r<uint16_t>(); }
inline uint32_t istream_t::uint32() { return _r<uint32_t>(); }
inline float istream_t::float32() { return _r<float>(); }

inline void istream_t::skip(size_t n) {
	while(n--) byte();
}

inline fs_t& istream_t::fs() { return file().fs(); }
inline const fs_t& istream_t::fs() const { return file().fs(); }

inline std::string fs_file_t::rel(const std::string& rel) const {
	return fs().join(path(),rel);
}

inline fs_file_t* fs_t::get(const istream_t& parent,const std::string& rel) {
	return get(parent.file().path(),rel);
}

inline fs_file_t* fs_t::get(const std::string& parent,const std::string& rel) {
	return get(join(parent,rel));
}

#endif //__FS_HPP__

