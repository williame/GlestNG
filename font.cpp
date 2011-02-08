/*
 font.cpp is part of the GlestNG RTS game engine.
 Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
 See LICENSE for details
 (c) William Edwards, 2011; all rights reserved
*/

#include <algorithm>
#include <vector>

#include "graphics.hpp"
#include "font.hpp"
#include "3d.hpp"
#include "xml.hpp"
#include "fs.hpp"

template<typename T> int binary_search(const std::vector<T>& vec, int start, int end, const T& key)
{ // http://en.wikibooks.org/wiki/Algorithm_Implementation/Search/Binary_search#C.2B.2B
    // surely there is something standard somewhere?
    if(start > end) return -1; // not found
    const unsigned middle = (start + ((end - start) / 2));
    if(vec[middle] == key) return middle;
    if(vec[middle] < key) binary_search(vec, middle + 1, end, key);
    return binary_search(vec, start, middle - 1, key);
}

struct code_t {
	enum { INVALID = -1 };
	code_t(int c): code(c) {}
	code_t(): code(INVALID) {}
	int code;
	bool operator==(const code_t& o) const { return code == o.code; }
	bool operator<(const code_t& o) const { return code < o.code; }
};

class font_angel_t: public font_t {
public: // bitmap fonts created from TTF using AngelCode's BMFont
	font_angel_t(const std::string& filename);
	vec2_t measure(int ch);
	vec2_t measure(const char* msg,int count);
	int draw(int x,int y,int ch);
	int draw(int x,int y,const char* msg,int count);
	int kerning(int first,int second);
private:
	int baseline, lineheight;
	struct glyph_t: public code_t {
		glyph_t(int c): code_t(c) {}
		glyph_t() {}
		short x,y,w,h,ofs_x,ofs_y,advance;
	};
	enum { BASE_START = 32, BASE_STOP = 128 };
	glyph_t base[BASE_STOP-BASE_START], invalid;
	typedef std::vector<glyph_t> x_glyphs_t;
	x_glyphs_t x_glyphs;
	struct kerning_t: public code_t {
		kerning_t(int c): code_t(c) {}
		kerning_t() {}
		struct second_t: public code_t {
			second_t(int c,int a): code_t(c), amount(a) {}
			second_t() {}
			int amount;
		};
		std::vector<second_t> seconds;
		int get(int code) const {
			if(!seconds.size()) return 0;
			const int idx = binary_search(seconds,0,seconds.size(),second_t(code,0));
			if(idx == -1) return 0;
			return seconds[idx].amount;
		}
	};
	typedef std::vector<kerning_t> kernings_t;
	kernings_t kernings;
	GLuint texture;
	vec2_t texture_size;
	int gl_draw(int x,int y,int ch) const;
	const glyph_t& get(int code) const; // unicode
};

font_angel_t::font_angel_t(const std::string& filename) {
	std::auto_ptr<fs_t> fs(fs_t::create("data"));
	fs_file_t::ptr_t data_file(fs->get(filename+".fnt"));
	istream_t::ptr_t data_stream(data_file->reader());
	xml_parser_t data_parser(filename.c_str(),*data_stream);
	xml_parser_t::walker_t xml(data_parser.walker());
	xml.check("font");
	xml.get_child("common");
	lineheight = xml.value_int("lineHeight");
	baseline = xml.value_int("base");
	texture_size = vec2_t(xml.value_int("scaleW"),xml.value_int("scaleH"));
	if(xml.value_int("pages") != 1) data_error("font "<<*data_file<<" isn\'t only one page: "<<xml.value_int("pages"));
	xml.up().get_child("pages").get_child("page");
	fs_file_t::ptr_t tex_handle(fs->get(xml.value_string("file")));
	texture = graphics()->alloc_texture(*tex_handle);
	xml.up().up().get_child("chars").get_child("char");
	memset(&invalid,0,sizeof(invalid));
	for(size_t i=0; xml.up().get_child("char",i); i++) {
		const int code = xml.value_int("id");
		glyph_t glyph(code);
		glyph.x = xml.value_int("x");
		glyph.y = xml.value_int("y");
		glyph.w = xml.value_int("width");
		glyph.h = xml.value_int("height");
		glyph.ofs_x = xml.value_int("xoffset");
		glyph.ofs_y = xml.value_int("yoffset");
		glyph.advance = xml.value_int("xadvance");
		if(code == code_t::INVALID)
			invalid = glyph;
		else if((code >= BASE_START) && (code < BASE_STOP))
			base[code-BASE_START] = glyph;
		else
			x_glyphs.push_back(glyph);
	}
	std::sort(x_glyphs.begin(),x_glyphs.end());
	xml.up();
	if(xml.has_child("kernings")) {
		xml.get_child("kernings");
		for(size_t i=0; xml.get_child("kerning",i); i++) {
			kernings.push_back(kerning_t(xml.value_int("first")));
			kernings.back().seconds.push_back(kerning_t::second_t(xml.value_int("second"),xml.value_int("amount")));
			xml.up();
		}
		for(kernings_t::iterator i=kernings.begin(); i!=kernings.end(); i++)
			std::sort(i->seconds.begin(),i->seconds.end());
		std::sort(kernings.begin(),kernings.end());
		std::cout << "(" << kernings.size() << " kernings)" << std::endl;
	}
}

const font_angel_t::glyph_t& font_angel_t::get(int code) const {
	if(code == code_t::INVALID) return invalid;
	if(code>=BASE_START && code<BASE_STOP) {
		if(base[code-BASE_START].code == code_t::INVALID) return invalid;
		return base[code-BASE_START];
	}
	if(!x_glyphs.size()) return invalid;
	const int idx = binary_search(x_glyphs,0,x_glyphs.size(),glyph_t(code));
	if(idx == -1) return invalid;
	return x_glyphs[idx];
}

int font_angel_t::kerning(int first,int second) {
	if(!kernings.size()) return 0;
	const int idx = binary_search(kernings,0,kernings.size(),kerning_t(first));
	if(idx == -1) return 0;
	const int amount = kernings[idx].get(second);
	return amount;
}
	
vec2_t font_angel_t::measure(int ch) {
	return vec2_t(get(ch).advance,lineheight);
}

vec2_t font_angel_t::measure(const char* msg,int count) {
	vec2_t sz(0,lineheight);
	for(int m=0; m<count; m++) {
		sz.x += measure(msg[m]).x;
		if(m)
			sz.x += kerning(msg[m-1],msg[m]);
	}
	return sz;
}

int font_angel_t::draw(int x,int y,int ch) {
	glBindTexture(GL_TEXTURE_2D,texture);
	const int advance = gl_draw(x,y,ch);
	glBindTexture(GL_TEXTURE_2D,0);
	return advance;
}

int font_angel_t::gl_draw(int x,int y,int ch) const {
	const glyph_t& g = get(ch);
	const float
		tx0 = (float)g.x/texture_size.x,
		ty0 = (float)g.y/texture_size.y,
		tx1 = (float)(g.x+g.w)/texture_size.x,
		ty1 = (float)(g.y+g.h)/texture_size.y;
	const float
		x0 = x+g.ofs_x,
		y0 = y+g.ofs_y,
		x1 = x0+g.w,
		y1 = y0+g.h;
	glBegin(GL_QUADS);
	glTexCoord2f(tx0,ty0);
	glVertex2f(x0,y0);
	glTexCoord2f(tx1,ty0);
	glVertex2f(x1,y0);
	glTexCoord2f(tx1,ty1);
	glVertex2f(x1,y1);
	glTexCoord2f(tx0,ty1);
	glVertex2f(x0,y1);
	glEnd();
	return g.advance;
}

int font_angel_t::draw(int x,int y,const char* msg,int count) {
	const int start = x;
	glBindTexture(GL_TEXTURE_2D,texture);
	for(int m=0; m<count; m++) {
		x += gl_draw(x,y,msg[m]);
		if(m)
			x += kerning(msg[m-1],msg[m]);
	}
	glBindTexture(GL_TEXTURE_2D,0);
	return x-start;
}

static fonts_t* singleton = NULL;

struct fonts_t::pimpl_t {
	pimpl_t(): sans(new font_angel_t("bitstream_vera_sans")) {}
	std::auto_ptr<font_angel_t> sans;
};

fonts_t* fonts_t::create() {
	if(singleton) panic("font manager already created");
	singleton = new fonts_t();
	return singleton;
}

fonts_t* fonts_t::fonts() {
	if(!singleton) panic("font manager not created");
	return singleton;
}

fonts_t::fonts_t(): pimpl(new pimpl_t()) {}

fonts_t::~fonts_t() {
	if(this != singleton) panic("font manager is not singleton");
	singleton = NULL;
	delete pimpl;
}

font_t* fonts_t::get(logical_t face) {
	switch(face) {
	case SANS: return pimpl->sans.get();
	default: panic("font "<<face<<" not supported");
	}
}

