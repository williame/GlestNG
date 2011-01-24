/*
 fs.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __FS_HPP__
#define __FS_HPP__

#include <ostream>
#include <vector>
#include <memory>
#include <inttypes.h>

class istream_t {
public:
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
private:
	template<typename T> T _r();
};

class fs_mgr_t {
public:
	static fs_mgr_t* get_fs();
	static void create(const std::string& data_directory);
	virtual ~fs_mgr_t() {}
	virtual bool is_file(const std::string& path) const = 0;
	virtual bool is_dir(const std::string& path) const = 0;
	virtual std::string join(const std::string& path,const std::string& sub) const = 0;
	virtual std::auto_ptr<istream_t> open(const std::string& path) = 0;
	typedef std::vector<std::string> list_t;
	virtual list_t list_dirs(const std::string& path) = 0;
	virtual list_t list_files(const std::string& path) = 0;
protected:
	fs_mgr_t() {}
};

inline fs_mgr_t* fs() { return fs_mgr_t::get_fs(); }

inline std::ostream& operator<<(std::ostream& out,const istream_t& repr) {
	return repr.repr(out);
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

#endif //__FS_HPP__

