sources = scene_graph.cpp gles2.cpp animation.cpp text.cpp window_x11.cpp 3rdparty/3rdparty.o
headers = atmosphere.hpp animation.hpp gles2.hpp
CC = gcc
CXX = g++
CPPFLAGS = -I. -I3rdparty -I/usr/include/freetype2
CFLAGS = -O2 -fPIC
CXXFLAGS = -std=c++11 -O2
LDFLAGS = -L.
LDLIBS = -lX11 -lEGL -lGLESv2 -lfreetype

all: demo

libatmosphere.so: $(sources) $(headers)
	$(CXX) -shared -olibatmosphere.so -fPIC $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(sources) $(LDLIBS)

demo: demo.cpp libatmosphere.so
	$(CXX) -odemo $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) demo.cpp -latmosphere

clean:
	rm -f test libatmosphere.so

.PHONY: all clean
