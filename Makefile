CC = g++

#TODO: try -O2
BASE_FLAGS = -O2 -std=c++11

LDFLAGS = -I/usr/local/Cellar/boost/1.58.0/include

LLIBFLAGS = -L/usr/local/Cellar/boost/1.58.0/

LINKFLAGS = -lboost_system

FLAGS = $(BASE_FLAGS) $(LLIBFLAGS) $(LDFLAGS) $(LINKFLAGS)

main: main.cpp
	$(CC) $(FLAGS) -o main.out main.cpp
