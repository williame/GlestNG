/*
 xml.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
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
		visit(false), error(NULL),
		parent(NULL), first_child(NULL), next_peer(NULL) {}
	~token_t() { free(error); delete first_child; delete next_peer; }
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
	mutable bool visit;
	mutable char* error;
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
	std::string path() const {
		std::string ret = str();
		for(const token_t* p=parent; p; p = p->parent)
			ret = p->str() + '/' + ret;
		return ret;
	}
	bool equals(const char* s) const {
		const char* me = start;
		while(*s)
			if(*s++!=*me++)
				return false;
		return true;
	}
	bool equals(const token_t* t) const {
		if(t->len != len) return false;
		for(size_t i=0; i<len; i++)
			if(start[i] != t->start[i])
				return false;
		return true;
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
	title(strdup(t)),
	buf(strdup(xml)), // nearly RAII; if strdup() throws, title not freed
	doc(NULL) {}
	
xml_parser_t::xml_parser_t(const char* t,istream_t& in):
	title(strdup(t)),
	buf(in.read_allz()),
	doc(NULL) {}
	
void xml_parser_t::parse() {
	if(doc) return;
	const char *ch = buf;
	token_t* tok = NULL;
	try {
		ch = eat_whitespace(ch);
		if(*ch!='<')
			data_error("malformed XML, expecting <");
		const char *prev = 0;
		while(*ch) {
			if(prev == ch)
				data_error("internal error parsing "<<ch);
			prev = ch;
			if('<' == *ch) {
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
						token_t* open = tok;
						if(tok->type != OPEN) {
							if(tok->type != DATA)
								data_error("expecting closing tag to be after data");
							open = tok->parent;
						}
						tok = open->add_peer(CLOSE,ch);
						ch = eat_name(ch);
						tok->len = ch - tok->start;
						if(!tok->equals(open))
							data_error(tok->str()<<" mismatches "<<open->str());
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
		}
	} catch(data_error_t* de) {
		std::cerr << "Error parsing " << title << " @" << (ch-buf) << ": " << de << std::endl;
		if(!doc)
			tok = doc = new token_t(ERROR,ch);
		else
			tok = tok->add_peer(ERROR,ch);
		tok->len = (buf+strlen(buf))-ch;
		tok->error = strdup(de->str().c_str());
		throw;
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

void xml_parser_t::walker_t::get_tag() {
	if(!ok()) panic("no token");
	if(KEY == tok->type)
		tok = tok->parent;
	if(OPEN != tok->type)
		panic("was expecting an open tag, got "<<tok->repr());
}

void xml_parser_t::walker_t::get_key(const char* key) {
	get_tag();
	for(token_t* child = tok->first_child; child; child = child->next_peer)
		if((KEY == child->type) && child->equals(key)) {
			tok = child;
			tok->visit = true;
			return;
		}
	data_error(key << " not found in " << tok->str() << " tag");
}

xml_parser_t::walker_t& xml_parser_t::walker_t::get_child(const char* tag) {
	get_tag();
	for(token_t* child = tok->first_child; child; child = child->next_peer)
		if((OPEN == child->type) && child->equals(tag)) {
			tok = child;
			tok->visit = true;
			return *this;
		}
	data_error(tok->str()<<" tag has no child tag called "<<tag);
}

xml_parser_t::walker_t& xml_parser_t::walker_t::up() {
	get_tag();
	if(!tok->parent)
		panic("cannot go up from root");
	tok = tok->parent;
	return *this;
}

void xml_parser_t::walker_t::check(const char* tag) {
	get_tag();
	if(strncmp(tag,tok->start,strlen(tag)))
		data_error("expecting "<<tag<<" tag, got "<<tok->str());
	tok->visit = true;
}

float xml_parser_t::walker_t::value_float(const char* key) {
	get_key(key);
	try {
		if(!tok->first_child || (VALUE != tok->first_child->type))
			panic("expecting key "<<tok->path()<<" to have a value child");
		tok = tok->first_child;
		errno = 0;
		char* endptr;
		const float val = strtof(tok->start,&endptr);
		if(errno) data_error("could not convert "<<tok->path()<<" to float: "<<tok->str()<<" ("<<errno<<": "<<strerror(errno));
		if(endptr != (tok->start+tok->len)) data_error(tok->path()<<" is not a float: "<<tok->str());
		if(!isnormal(val)) data_error(tok->path()<<" is not a valid float: "<<tok->str());
		tok->visit = true;
		tok = tok->parent;
		return val;
	} catch(data_error_t* de) {
		tok->error = strdup(de->str().c_str());
		throw;
	}
}

std::string xml_parser_t::walker_t::str() const {
	if(!ok()) panic("no token");
	return tok->str();
}

const char* xml_parser_t::walker_t::error_str() const {
	if(!ok()) panic("no token");
	return tok->error;
}

bool xml_parser_t::walker_t::visited() const {
	if(!ok()) panic("no token");
	return tok->visit;
}

xml_parser_t::walker_t::walker_t(const token_t* t): tok(t) {}

xml_parser_t::walker_t xml_parser_t::walker() {
	if(!doc) parse();
	return walker_t(doc);
}

static std::ostream& indent(std::ostream& out,int n) {
	out << std::endl;
	while(n-- >0)
		out << "  ";
	return out;
}

static std::string html_escape(std::string s,bool visited) {
	std::string safe;
	if(visited)
		safe += "<b>";
	for(std::string::const_iterator ch=s.begin(); ch!=s.end(); ch++)
		switch(*ch) {
		case '&': safe += "&amp;"; break;
		case '<': safe += "&lt;"; break;
		case '>': safe += "&gt;"; break;
		default: safe += *ch;
		}
	if(visited)
		safe += "</b>";
	return safe;
}

void xml_parser_t::describe_xml(std::ostream& out) {
	out << "<html><head><title>" << title << "</title>" <<
		"<style type=\"text/css\">" <<
		"body {color:gray;}" <<
		"b {color:black;}" <<
		"#E {color:red;}" <<
		"</style>" <<
		"</head><body bgcolor=white><pre>";
	int depth=0;
	bool in_tag = false;
	for(walker_t node = walker(); node.ok(); node.next()) {
		type_t type = node.type();
		if(node.tok->error) type = ERROR;
		switch(type) {
		case OPEN:
			if(in_tag)
				out << "&gt;";
			indent(out,depth++)<<"&lt;"<<html_escape(node.str(),node.tok->visit);
			in_tag = true;
			break;
		case CLOSE:
			depth--;
			if(in_tag) {
				out<<"/&gt;";
				in_tag = false;
			} else
				indent(out,depth)<<"&lt;/"<<html_escape(node.str(),node.tok->visit)<<"&gt;";
			break;
		case KEY:
			out<<" "<<html_escape(node.str(),node.tok->visit);
			break;
		case VALUE:
			out<<"=\""<<html_escape(node.str(),node.tok->visit)<<"\"";
			break;
		case DATA:
			in_tag = false;
			out<<"&gt;";
			indent(out,depth)<<html_escape(node.str(),node.tok->visit);
			break;
		case ERROR:
			out<<" <span id=\"E\">"<<(node.tok->error?node.tok->error:"")<<std::endl<<
				html_escape(node.str(),node.tok->visit)<<"</span> "<<std::endl;
			break;
		}
	}
	out << "</pre></body></html>" << std::endl;
}


