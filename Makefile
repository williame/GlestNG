# Makefile for Shiny
# William Edwards 20100903
# (c) MashMobile AB 2010; all rights reserved

CC = gcc
CPP = g++
LD = g++

HYGIENE = -g3 -Wall #-pedantic-errors -std=c++98 -Wno-long-long -fdiagnostics-show-option
DEBUG = -O0
OPTIMISATIONS = # -O9 -fomit-frame-pointer -fno-rtti -march=native # etc -fprofile-generate/-fprofile-use

# default flags
CFLAGS = ${HYGIENE} ${DEBUG} ${OPTIMISATIONS} ${C_EXT_FLAGS}
CPPFLAGS = ${CFLAGS}
LDFLAGS = ${HYGIENE} ${DEBUG} ${OPTIMISATIONS}

#target binary names

TRG_SHINY = shiny

OBJ_SHINY_CPP = \
	shiny.opp \
	task.opp \
	out.opp \
	error.opp \
	time.opp \
	listener.opp \
	console.opp \
	dr.opp \
	monitor2.opp

OBJ_SHINY_C = 
		
OBJ_CPP = ${OBJ_SHINY_CPP}

OBJ_C = ${OBJ_SHINY_C}

OBJ = ${OBJ_CPP}  ${OBJ_C}

TARGETS = ${TRG_SHINY}

all:	${TARGETS}

${TRG_SHINY}:	${OBJ_SHINY_CPP} ${OBJ_SHINY_C}
	${LD} ${CPPFLAGS} -o $@ $^ ${LDFLAGS}

# compile c files
	
%.o:	%.c
	${CC} ${CFLAGS} -c $< -MD -MF $(<:%.c=%.dep) -o $@

# compile c++ files
	
%.opp:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -MD -MF $(<:%.cpp=%.dep) -o $@
#misc

.PHONY:	clean all
clean:
	rm -f ${TARGETS}
	rm -f *.[hc]pp~ Makefile~ core
	rm -f ${OBJ}
	rm -f $(OBJ_C:%.o=%.dep) $(OBJ_CPP:%.opp=%.dep)

-include $(OBJ_C:%.o=%.dep) $(OBJ_CPP:%.opp=%.dep)

