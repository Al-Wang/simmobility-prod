# Copyright Singapore-MIT Alliance for Research and Technology

CXX = g++
CXXFLAGS = -DDEBUG=1 -I. -W -Wall

OBJS = data_mgr.o big_brother.o

TARGETS = main

all: $(TARGETS)

main: main.o $(OBJS)
	$(CXX) -o main main.o $(OBJS)

clean:	tidy
	-rm -f $(TARGETS) main.o $(OBJS)

tidy:
	-rm -f *~
