CXX=g++
CC=gcc

CXXFLAGS=-std=c++17 -pedantic -Wall -I/opt/vulkan/1.3.239/include

GLCL=/opt/vulkan/1.3.239/bin/glslangValidator
GLCLFLAGS=-V

LK=g++
LKFLAGS=-Wl,-rpath=/opt/vulkan/1.3.239/lib
LIBS=-lvulkan -lglfw

OBJS=main.o vkContext.o mesh.o

SHADERS=vertex.spv frag.spv

PROG=triangle

$(PROG) : $(SHADERS) $(OBJS) 
	$(LK) $(LKFLAGS) $(OBJS) $(LIBS) -o $(PROG)

all : clean $(PROG)

main.o : main.cpp
	$(CXX) -c -g $(CXXFLAGS) main.cpp -o main.o


vkContext.o : vkContext.cpp vkContext.h utilities.h
	$(CXX) -c -g $(CXXFLAGS) vkContext.cpp -o vkContext.o

mesh.o : mesh.cpp mesh.h
	$(CXX) -c -g $(CXXFLAGS) mesh.cpp -o mesh.o

vertex.spv : Shaders/shader.vert
	$(GLCL) $(GLCLFLAGS) Shaders/shader.vert -o Shaders/vert.spv

frag.spv : Shaders/shader.frag 
	$(GLCL) $(GLCLFLAGS) Shaders/shader.frag -o Shaders/frag.spv

clean:
	rm -f *.o
	rm -f *.*~
	rm -f *~
	rm -f $(PROG)





