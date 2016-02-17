SOURCES = test.cpp scene_graph.cpp gles2.cpp window_x11.cpp 3rdparty/3rdparty.c
OBJECTS = $(addsuffix .o,$(basename $(SOURCES)))
CC = gcc
CXX = g++
CPPFLAGS = -I. -I3rdparty
CFLAGS = -O2
CXXFLAGS = -std=c++11 -O2

test: $(OBJECTS)
	$(CXX) -otest $(OBJECTS) -lX11 -lEGL -lGLESv2

clean:
	rm *.o test

.PHONY: clean
