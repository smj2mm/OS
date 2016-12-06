# Steven Jenny
# smj2mm

CXX=g++
#CXXFLAGS=-Wall -O2
FLAGS= -g

OFILES = homework4.o

fat: $(OFILES)
	$(CXX) -o my_ftpd $(OFILES) $(FLAGS)

clean: 
	/bin/rm -f *.o *~

.SUFFIXES: .o .c

homework4.o: homework4.cpp
