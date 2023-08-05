CXX=g++
CC=gcc

CXXFLAGS=-std=c++11 -pedantic -Wall

LK=g++
LKFLAGS=
LIBS=-lvulkan -lglfw

OBJS=main.o logger.o vkCtx.o

PROG=module1

$(PROG) : $(OBJS)
	$(LK) $(LKFLAGS) $(OBJS) $(LIBS) -o $(PROG)

all : clean $(PROG)

main.o : main.cpp
	$(CXX) -c -g $(CXXFLAGS) main.cpp -o main.o

logger.o : logger.cpp logger.h
	$(CXX) -c -g $(CXXFLAGS) logger.cpp -o logger.o

vkCtx.o : vkCtx.cpp vkCtx.h
	$(CXX) -c -g $(CXXFLAGS) vkCtx.cpp -o vkCtx.o

clean:
	rm -f *.o
	rm -f *.*~
	rm -f *~
	rm -f $(PROG)
