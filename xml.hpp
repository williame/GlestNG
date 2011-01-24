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

#include "fs.hpp"

class xml_parser_t {
public:
	struct token_t;
	xml_parser_t(const char* title,istream_t& in);
	xml_parser_t(const char* title,const char* xml); // makes a copy
	void parse();
	~xml_parser_t();
	enum type_t {
		OPEN,
		CLOSE,
		KEY,
		VALUE,
		DATA,
		ERROR,
	};
	class walker_t {
	public:
		// depth-first traversal; don't mix with navigation API unless you know the inner workings
		bool next();
		bool ok() const { return tok; }
		// navigation API; preferred way of extracting things
		void check(const char* tag);
		walker_t& get_child(const char* tag);
		walker_t& up();
		// extract attributes
		float value_float(const char* key = "value");
		// query current node
		type_t type() const;
		std::string str() const;
		const char* error_str() const;
		bool visited() const;
		friend class xml_parser_t;
	private:
		walker_t(const token_t* tok);
		const token_t* tok;
		void get_key(const char* key);
		void get_tag();
	};
	walker_t walker();
	void describe_xml(std::ostream& out);
private:
	const char* const title;
	const char* const buf;
	token_t *doc;
};

std::ostream& operator<<(std::ostream& out,xml_parser_t::type_t type);

#endif //__XML_HPP__

