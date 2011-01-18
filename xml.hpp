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

class xml_parser_t {
public:
	struct token_t;
	xml_parser_t(const char* title,const char* xml); // makes a copy
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
		type_t type() const;
		bool next();
		bool ok() const { return tok; }
		std::string str() const;
		friend class xml_parser_t;
	private:
		walker_t(const token_t* tok);
		const token_t* tok;
	};
	walker_t walker() const;
private:
	const char* const title;
	const char* const buf;
	token_t *doc;
};

std::ostream& operator<<(std::ostream& out,xml_parser_t::type_t type);

#endif //__XML_HPP__

