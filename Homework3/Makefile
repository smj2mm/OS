# Steven Jenny
# smj2mm

CXX=g++
CXXFLAGS=-Wall -O2
FLAGS= -g

OFILES = hw3.o

max: $(OFILES)
	$(CXX) -o max $(CXXFLAGS) $(OFILES) $(FLAGS)

clean: 
	/bin/rm -f *.o *~

.SUFFIXES: .o .c

hw3.o: hw3_final.cpp
