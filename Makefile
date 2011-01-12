# Makefile is part of the GlestNG RTS game engine.
# Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE version 3
# See LICENSE for details
# (c) William Edwards, 2011; all rights reserved

CC = gcc
CPP = g++
LD = g++

HYGIENE = -g3 -Wall #-pedantic-errors -std=c++98 -Wno-long-long -fdiagnostics-show-option
DEBUG = -O0
OPTIMISATIONS = # -O9 -fomit-frame-pointer -fno-rtti -march=native # etc -fprofile-generate/-fprofile-use

ifeq ($(shell uname),windows32)
	LIB_CFLAGS = `sdl-config --cflags`
	LIB_LDFLAGS = -static -lglew32s `sdl-config --libs` -dynamic -lopengl32 -lglu32 
	EXE_EXT = .exe
else
	LIB_CFLAGS = `pkg-config --cflags sdl SDL_image`
	LIB_LDFLAGS = `pkg-config --lib sdl SDL_image`
	EXE_EXT =
endif

# default flags
CFLAGS = ${HYGIENE} ${DEBUG} ${OPTIMISATIONS} ${C_EXT_FLAGS} ${LIB_CFLAGS}
CPPFLAGS = ${CFLAGS}
LDFLAGS = ${HYGIENE} ${DEBUG} ${OPTIMISATIONS} ${LIB_LDFLAGS}

#target binary names
	
TRG_GLEST_NG = glestng${EXE_EXT}
	
OBJ_GLEST_NG_CPP = \
	glestng.opp \
	planet.opp \
	graphics.opp \
	font.opp \
	3d.opp

OBJ_GLEST_NG_C = 
		
OBJ_CPP = ${OBJ_GLEST_NG_CPP}

OBJ_C = ${OBJ_GLEST_NG_C}

OBJ = ${OBJ_CPP}  ${OBJ_C}

TARGETS = ${TRG_GLEST_NG}

.PHONY:	clean all check_env

all:	check_env ${TARGETS}

${TRG_GLEST_NG}: ${OBJ_GLEST_NG_CPP} ${OBJ_GLEST_NG_C}
	${LD} ${CPPFLAGS} -o $@ $^ ${LDFLAGS}

# compile c files
	
%.o:	%.c
	${CC} ${CFLAGS} -c $< -MD -MF $(<:%.c=%.dep) -o $@

# compile c++ files
	
%.opp:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -MD -MF $(<:%.cpp=%.dep) -o $@
#misc

clean:
	rm -f ${TARGETS}
	rm -f \*.?pp\~ Makefile\~ core
	rm -f ${OBJ}
	rm -f $(OBJ_C:%.o=%.dep) $(OBJ_CPP:%.opp=%.dep)

check_env:
ifeq ($(shell uname),windows32)
	@echo "You are using windows; good luck!"
else
	`pkg-config --exists sdl gl`
endif

-include $(OBJ_C:%.o=%.dep) $(OBJ_CPP:%.opp=%.dep)

