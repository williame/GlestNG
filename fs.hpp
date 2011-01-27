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
	typedef std::auto_ptr<istream_t> ptr_t;
	virtual const char* path() const = 0;
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

class fs_handle_t {
public:
	typedef std::auto_ptr<fs_handle_t> ptr_t;
	virtual ~fs_handle_t() {}
	virtual istream_t::ptr_t reader() = 0;
	virtual const char* path() const = 0;
	virtual const char* name() const = 0;
	virtual const char* ext() const = 0;
protected:
	fs_handle_t() {}
};

class fs_t {
public:
	class mgr_t {
	public:
		virtual ~mgr_t();
	private:
		friend class fs_t;
		mgr_t() {}
	};
	static std::auto_ptr<mgr_t> create(const std::string& data_directory);
	static fs_t* fs();
	bool is_file(const std::string& path) const;
	bool is_dir(const std::string& path) const;
	std::string canocial(const std::string& path) const;
	std::string join(const std::string& path,const std::string& sub) const;
	std::string parent_directory(const std::string& path) const;
	fs_handle_t* get(const std::string& path);
	typedef std::vector<std::string> list_t;
	list_t list_dirs(const std::string& path);
	list_t list_files(const std::string& path);
private:
	fs_t(const std::string& data_directory);
	~fs_t();
	struct pimpl_t;
	friend struct pimpl_t;
	pimpl_t* pimpl;
};

inline fs_t* fs() { return fs_t::fs(); }

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

