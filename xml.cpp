/*
 xml.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "error.hpp"
#include "xml.hpp"

std::ostream& operator<<(std::ostream& out,xml_parser_t::type_t type) {
	switch(type) {
	case xml_parser_t::OPEN: out << "OPEN"; break;
	case xml_parser_t::CLOSE: out << "CLOSE"; break;
	case xml_parser_t::KEY: out << "KEY"; break;
	case xml_parser_t::VALUE: out << "VALUE"; break;
	case xml_parser_t::DATA: out << "DATA"; break;
	case xml_parser_t::ERROR: out << "ERROR"; break;
	default: out << "<unknown XML token type("<<(int)type<<")";
	}
	return out;
}

struct xml_parser_t::token_t {
	token_t(type_t t,const char* s): 
		type(t), start(s), len(0),
		parent(NULL), first_child(NULL), next_peer(NULL) {}
	~token_t() { delete first_child; delete next_peer; }
	token_t* add_child(type_t t,const char* s) {
		if(first_child)
			return first_child->add_peer(t,s);
		first_child = new token_t(t,s);
		first_child->parent = this;
		return first_child;
	}
	token_t* add_peer(type_t t,const char* s) {
		if(!next_peer) {
			next_peer = new token_t(t,s);
			next_peer->parent = parent;
			return next_peer;
		} else {
			token_t* peer = next_peer;
			while(peer->next_peer)
				peer = peer->next_peer;
			peer = peer->next_peer = new token_t(t,s);
			peer->parent = parent;
			return peer;
		}
	}
	const type_t type; 
	const char* const start;
	size_t len;
	token_t *parent, *first_child, *next_peer;
	std::string str() const {
		std::string ret;
		for(size_t i=0; i<len; i++)
			ret += start[i];
		return ret;
	}
	std::string repr() const {
		std::stringstream ret("XML_Token<",std::ios_base::out|std::ios_base::ate);
		ret << type << ",";
		for(size_t i=0; i<len; i++)
			ret << start[i];
		ret <<">";
		return ret.str();
	}
};

static const char* eat_whitespace(const char* ch) { while(*ch && *ch <= ' ') ch++; return ch; }
static const char* eat_name(const char* ch) { while((*ch>' ')&&!strchr("/>=",*ch)) ch++; return ch; }
static bool starts_with(const char* str,const char* pre) {
	while(*pre)
		if(*pre++!=*str++) return false;
	return true;
}

xml_parser_t::xml_parser_t(const char* t,const char* xml): // copies data into internal buffer
	title(strdup(t)), // nearly RAII; if strdup() throws, title not freed
	buf(strdup(xml)),
	doc(NULL) {
	// parse it, writing control-tokens into it
	const char *ch = eat_whitespace(buf);
	if(*ch!='<')
		data_error("malformed XML, expecting <");
	token_t* tok = NULL;
	const char *prev = 0;
	while(*ch) {
		try {
			if(prev == ch)
				data_error("internal error parsing "<<ch);
			prev = ch;
/*			if(tok)
				std::cout << "parsing "<<tok->repr();
			else
				std::cout << "NO TOKEN";
			std::cout << " examining "<<*ch<<std::endl;
*/			if('<' == *ch) {
				if(tok && (DATA == tok->type)) {
					if(eat_whitespace(tok->start) == ch)
						data_error("unexpected empty token "<<tok->repr());
					tok->len = (ch-tok->start)-1;
					tok = tok->parent;
				}
				if(starts_with(ch,"<!--")) {
					if(const char* t = strstr(ch,"-->"))
						ch = eat_whitespace(t+3);
					else data_error("unclosed comment");
				} else if(starts_with(ch,"<?")) {
					if(const char* t = strstr(ch,"?>"))
						ch = eat_whitespace(t+2);
					else data_error("unclosed processing tag");
				} else {
					ch = eat_whitespace(ch+1);
					if('/' == *ch) {
						if(!tok) data_error("unexpected close of tag");
						ch = eat_whitespace(ch+1);
						if(tok->type == OPEN)
							tok = tok->add_peer(CLOSE,ch);
						else {
							if(tok->type != DATA)
								data_error("expecting closing tag to be after data");
							tok = tok->parent->add_peer(CLOSE,ch);
						}
						ch = eat_name(ch);
						tok->len = ch - tok->start;
						ch = eat_whitespace(ch);
						if('>'!=*ch) data_error("unclosed close tag "<<*ch);
						const char* peek = eat_whitespace(++ch);
						if(*peek == '<')
							ch = peek;
						else if(!tok->parent) {
							if(*peek)
								data_error("unexpected content at top level: "<<peek);
							break; // all done
						}
						if(OPEN!=tok->parent->type)
							data_error("unexpected "<<tok->repr()<<" after "<<tok->parent->repr());
						tok = tok->parent;
					} else {
						if(!tok)
							doc = tok = new token_t(OPEN,ch);
						else if(DATA == tok->type)
							tok = tok->add_peer(OPEN,ch);
						else if(OPEN == tok->type)
							tok = tok->add_child(OPEN,ch);
						else data_error("was not expecting a new tag after "<<tok->repr());
						ch = eat_name(ch);
						tok->len = ch - tok->start;
						ch = eat_whitespace(ch);
					}
				}
			} else if(!tok) {
				data_error("expecting <");
			} else if(DATA == tok->type) {
				ch++;
			} else if('>' == *ch) {
				if(OPEN == tok->type) {
					// skip insignificant whitespace
					const char* peek = eat_whitespace(++ch);
					if(*peek != '<')
						tok = tok->add_child(DATA,ch);
					else
						ch = peek;
				} else if(KEY != tok->type)
					tok = tok->parent;
			} else if('=' == *ch) {
				if(KEY != tok->type)
					data_error("was not expecting = after "<<tok->repr());
				if(tok->first_child)
					data_error("was not expecting = after "<<tok->first_child->repr());
				ch = eat_whitespace(ch+1);
				if('\"' != *ch)
					data_error("was expecting \" after "<<tok->repr());
				ch++;
				tok = tok->add_child(VALUE,ch);
				ch = strchr(ch,'\"');
				if(!ch) data_error("unclosed attribute "<<tok->parent->repr());
				tok->len = (ch - tok->start);
				tok = tok->parent->parent;
				ch = eat_whitespace(ch+1);
			} else if('/' == *ch) {
				if(OPEN != tok->type)
					data_error("not expecting / after "<<tok->repr());
				token_t* close = tok->add_peer(CLOSE,tok->start);
				close->len = tok->len;
				tok = tok->parent;
				ch = eat_whitespace(ch+1);
				if('>' != *ch)
					data_error("not expecting "<<*ch<<" after "<<close->repr());
				const char* peek = eat_whitespace(++ch);
				if(*peek == '<')
					ch = peek;
				else
					tok = tok->add_child(DATA,ch++);
			} else if(OPEN == tok->type) {
				tok = tok->add_child(KEY,ch);
				ch = eat_name(ch);
				tok->len = (ch - tok->start);
				ch = eat_whitespace(ch);
			} else 
				data_error("did not understand "<<*ch<<" after"<<tok->repr());
		} catch(data_error_t* de) {
			std::cerr << de << std::endl;
			if(!doc)
				tok = doc = new token_t(ERROR,ch);
			else
				tok = tok->add_peer(ERROR,ch);
			tok->len = (buf+strlen(buf))-ch;
			break;
		}
	}
}
	
xml_parser_t::~xml_parser_t() {
	free(const_cast<char*>(title));
	free(const_cast<char*>(buf));
	delete doc;
}

xml_parser_t::type_t xml_parser_t::walker_t::type() const {
	if(!ok()) panic("no token");
	return tok->type;
}

bool xml_parser_t::walker_t::next() {
	if(!ok()) panic("no token");
	if(tok->first_child) {
		tok = tok->first_child;
		return true;
	}
	if(tok->next_peer) {
		tok = tok->next_peer;
		return true;
	}
	while(true) {
		tok = tok->parent;
		if(!tok) return false;
		if(tok->next_peer) {
			tok = tok->next_peer;
			return true;
		}
	}
}

std::string xml_parser_t::walker_t::str() const {
	if(!ok()) panic("no token");
	return tok->str();
}

xml_parser_t::walker_t::walker_t(const token_t* t): tok(t) {}

xml_parser_t::walker_t xml_parser_t::walker() const {
	if(!doc)
		data_error("XML file "<<title<<" has no document");
	return walker_t(doc);
}

