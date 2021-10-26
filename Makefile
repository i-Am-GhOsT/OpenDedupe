# g++ -ggdb -Wall -Wno-sign-compare -std=c++11 -I. -c rehydrate.cpp -o rehydrate.o -lpthread
# g++ -ggdb -Wall -Wno-sign-compare -std=c++11 -I. -c dedupe.cpp -o dedupe.o -lpthread
# g++ -ggdb -Wall -std=c++11 -I. dedupe.o rehydrate.o main.cpp -o main -lpthread

CC=gcc
CPP=g++
CPPFLAGS=-std=c++11 -fPIC -Wall -Wextra -Wno-sign-compare -ggdb
INCLUDE=-I.
LDFLAGS=-lpthread

OBJ = dedupe.o rehydrate.o

all: clean dedupe.o rehydrate.o main

%.0 : %.c
	$(CPP) $(CPPFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

main: main.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDE) dedupe.o rehydrate.o main.cpp -o main  $(LDFLAGS)

clean:
	rm -f *.o *~ main
