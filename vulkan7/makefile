CXX=g++
CC=gcc

CXXFLAGS=-std=c++17 -pedantic -Wall -I/opt/vulkan/1.3.239/include -I/usr/local/include

GLCL=/opt/vulkan/1.3.239/bin/glslangValidator
GLCLFLAGS=-V

LK=g++
LKFLAGS=-L/usr/local/lib64 -Wl,-rpath=/opt/vulkan/1.3.239/lib -Wl,-rpath=/usr/local/lib64
LIBS=-lvulkan -lglfw -lassimp

OBJS=main.o vkContext.o mesh.o MeshModel.o

SHADERS=vertex.spv frag.spv second_vert.spv second_frag.spv

PROG=vulkan7

$(PROG) : $(SHADERS) $(OBJS) 
	$(LK) $(LKFLAGS) $(OBJS) $(LIBS) -o $(PROG)

all : clean $(PROG)

main.o : main.cpp
	$(CXX) -c -g $(CXXFLAGS) main.cpp -o main.o


vkContext.o : vkContext.cpp vkContext.h utilities.h
	$(CXX) -c -g $(CXXFLAGS) vkContext.cpp -o vkContext.o

mesh.o : mesh.cpp mesh.h
	$(CXX) -c -g $(CXXFLAGS) mesh.cpp -o mesh.o

MeshModel.o : MeshModel.h MeshModel.cpp
	$(CXX) -c -g $(CXXFLAGS) MeshModel.cpp -o MeshModel.o

vertex.spv : Shaders/shader.vert
	$(GLCL) $(GLCLFLAGS) Shaders/shader.vert -o Shaders/vert.spv

frag.spv : Shaders/shader.frag 
	$(GLCL) $(GLCLFLAGS) Shaders/shader.frag -o Shaders/frag.spv

second_vert.spv : Shaders/second.vert
	$(GLCL) $(GLCLFLAGS) Shaders/second.vert -o Shaders/second_vert.spv

second_frag.spv : Shaders/second.frag
	$(GLCL) $(GLCLFLAGS) Shaders/second.frag -o Shaders/second_frag.spv

clean:
	rm -f *.o
	rm -f *.*~
	rm -f *~
	rm -f $(PROG)





