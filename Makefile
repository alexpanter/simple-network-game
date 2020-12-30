GCC=g++ -O3 -g -Wall -Wextra -pedantic -std=c++17
LD_FLAGS= -pthread
GL_LD_FLAGS=-lGLEW -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lXi -ldl -lXinerama -lXcursor
GRAPHICS_LIB=system.hpp fileIO.hpp shaders.hpp window.hpp

all: csapp.o client server

csapp.o: csapp.cpp csapp.h
	$(GCC) -c $< -o $@

client: client.cpp csapp.o $(GRAPHICS_LIB)
	$(GCC) $< *.o -o $@ $(GL_LD_FLAGS) $(LD_FLAGS)

server: server.cpp csapp.o
	$(GCC) $(CFLAGS) $< *.o -o $@ $(LD_FLAGS)

zip: ../src.zip

../src.zip: clean
	cd .. && zip -r src.zip src/Makefile src/*.c src/*.h

clean:
	rm -rf *.o client server
