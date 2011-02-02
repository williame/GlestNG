/*
 xml.hpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#ifndef __XML_HPP__
#define __XML_HPP__

#include <ostream>
#include <string>

class istream_t;

class xml_parser_t {
public:
	struct token_t;
	xml_parser_t(const char* title,istream_t& in);
	xml_parser_t(const char* title,const char* xml); // makes a copy
	void parse();
	~xml_parser_t();
	enum type_t {
		IGNORE,
		OPEN,
		CLOSE,
		KEY,
		VALUE,
		DATA,
		ERROR,
		NUM_TYPES
	};
	class walker_t {
	public:
		// depth-first traversal; don't mix with navigation API unless you know the inner workings
		bool next();
		bool ok() const { return tok; }
		// navigation API; preferred way of extracting things
		void check(const char* tag);
		walker_t& get_child(const char* tag);
		bool get_child(const char* tag,size_t i);
		walker_t& up();
		// extract attributes
		float value_float(const char* key = "value");
		std::string value_string(const char* key = "value");
		int value_int(const char* key = "value");
		std::string get_data_as_string();
		// query current node
		type_t type() const;
		size_t ofs() const;
		size_t len() const;
		std::string str() const;
		const char* error_str() const;
		bool visited() const;
		friend class xml_parser_t;
	private:
		walker_t(xml_parser_t& parser,const token_t* tok);
		xml_parser_t& parser;
		const token_t* tok;
		void get_key(const char* key);
		void get_tag();
	};
	walker_t walker();
	void describe_xml(std::ostream& out);
	const char* get_title() const { return title; }
	const char* get_buf() const { return buf; }
private:
	const char* title;
	const char* buf;
	token_t *doc;
};

class xml_loadable_t {
public:
	virtual ~xml_loadable_t();
	const std::string name;
	bool load_xml(const std::string& buf);
	bool load_xml(istream_t& in);
	bool load_xml(xml_parser_t* xml);
	bool is_inited() const { return inited; }
	void check_inited() const; // panics if not inited
	xml_parser_t* get_xml() { return xml; }
protected:
	xml_loadable_t(const std::string& name);
private:
	virtual void _load_xml(xml_parser_t::walker_t& xml) = 0;
	xml_parser_t* xml;
	bool inited;
};

std::ostream& operator<<(std::ostream& out,xml_parser_t::type_t type);

#endif //__XML_HPP__

