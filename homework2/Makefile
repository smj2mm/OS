# Steven Jenny
# smj2mm

CXX=g++
CXXFLAGS=-Wall -O2
FLAGS= -lpthread

OFILES = barrier.o homework2.o

max: $(OFILES)
	$(CXX) -o max $(CXXFLAGS) $(OFILES) $(FLAGS)

clean: 
	/bin/rm -f *.o *~

.SUFFIXES: .o .c

barrier.o: barrier.cpp barrier.h
homework2.o: homework2.cpp
