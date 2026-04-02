SHELL = /bin/sh
CC = g++

CXXFLAGS_DEBUG = -g -O0 -fno-omit-frame-pointer -rdynamic -fsanitize=address,undefined
CXXFLAGS_RELEASE = -funroll-loops -O3 -DNDEBUG -march=native -std=c++17 -pthread

INCLUDES = -Iinclude -Isdsl-lite/include -I$(HOME)/include
LDFLAGS = -L$(HOME)/lib
LDLIBS = -lsdsl -ldivsufsort -ldivsufsort64

all: recomp_query

recomp_query:
	$(CC) $(CXXFLAGS_RELEASE) $(INCLUDES) ./src/*.cpp -o recomp_query $(LDFLAGS) $(LDLIBS)

debug:
	$(CC) $(CXXFLAGS_DEBUG) $(INCLUDES) ./src/*.cpp -o recomp_query $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o recomp_query

nuclear:
	rm -f recomp_query  *.o