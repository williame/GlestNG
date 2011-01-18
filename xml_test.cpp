
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <stdlib.h>
#include <iostream>

#include "xml.hpp"

static std::ostream& indent(std::ostream& out,int n) {
	out << std::endl;
	while(n-- >0)
		out << "  ";
	return out;
}

static bool parse(const char* filename) {
	FILE* f = fopen(filename,"rb");
	if(!f) {
		fprintf(stderr,"could not open %s: %d %s\n",
			filename,errno,strerror(errno));
		return false;
	}
	fseek(f,0,SEEK_END);
	const long len = ftell(f);
	fseek(f,0,SEEK_SET);
	char *body = new char[len+1];
	fread(body,1,len,f);
	body[len] = 0;
	fclose(f);
	printf("parsing %s\n",filename);
	xml_parser_t xml(filename,body);
	int depth=0;
	bool in_tag = false;
	for(xml_parser_t::walker_t node = xml.walker(); node.ok(); node.next()) {
		switch(node.type()) {
		case xml_parser_t::OPEN:
			if(in_tag)
				std::cout << ">";
			indent(std::cout,depth++)<<"<"<<node.str();
			in_tag = true;
			break;
		case xml_parser_t::CLOSE:
			depth--;
			if(in_tag) {
				std::cout<<"/>";
				in_tag = false;
			} else
				indent(std::cout,depth)<<"</"<<node.str()<<">";
			break;
		case xml_parser_t::KEY:
			std::cout<<" "<<node.str();
			break;
		case xml_parser_t::VALUE:
			std::cout<<"=\""<<node.str()<<"\"";
			break;
		case xml_parser_t::DATA:
			in_tag = false;
			std::cout<<">";
			indent(std::cout,depth)<<node.str();
			break;
		case xml_parser_t::ERROR:
			std::cout<<" ERROR "<<node.str()<<std::endl;
			return false;
		}
	}
	std::cout << std::endl;
	return true;
}

char path[1024];
bool path_ends_with(const char* tail) {
	const size_t path_len = strlen(path), tail_len = strlen(tail);
	return ((path_len>tail_len)&&!strcmp(path+path_len-tail_len,tail));
}

static void walk() {
	if(DIR *dp = opendir(path)) {
		const size_t path_len = strlen(path);
		path[path_len] = '/';
		struct dirent *ep;		
		while(ep = readdir(dp)) {
			if(ep->d_name[0] == '.') continue;
			if(path_len+strlen(ep->d_name)+1 >= sizeof(path))
				printf("cannot walk %s/%s, path too long\n",path,ep->d_name);
			else {
				strcpy(path+path_len+1,ep->d_name);
				walk();
			}
		}
		closedir(dp);
	} else if(path_ends_with(".xml"))
		if(!parse(path))
			exit(1);
}

int main(int argc,char** args) {
	for(int i=1; i<argc; i++) {
		strncpy(path,args[i],sizeof(path));
		walk();
	}
	return 0;
}

