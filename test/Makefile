SHELL = /bin/sh
CC = g++
#WARNINGS = -Wall -Wextra -pedantic -Wshadow
CFLAGS = -funroll-loops -O3 -DNDEBUG -march=native -std=c++17 -pthread
#CFLAGS = -g2 -std=c++17 -pthread
INCLUDES = -I../include -I../include/utils -I./include/utils -I../sdsl-lite/include -I$(HOME)/include
LDFLAGS = -L$(HOME)/lib
LDLIBS = -lsdsl -ldivsufsort -ldivsufsort64
SOURCES = ../src/recompression_definitions.cpp ../src/2DRangeQuery.cpp ../src/MacroMicroTree.cpp ../src/utils.cpp ./src/*.cpp

.PHONY: all clean nuclear

all: test

test: $(SOURCES) Makefile
	$(CC) $(CFLAGS) $(WARNINGS) $(INCLUDES) $(SOURCES) -o test $(LDFLAGS) $(LDLIBS)

clean:
	/bin/rm -f *.o test

nuclear:
	/bin/rm -f test *.o
