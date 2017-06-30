sources = scene_graph.cpp gles2.cpp animation.cpp text.cpp window_x11.cpp 3rdparty/3rdparty.o
headers = atmosphere.hpp animation.hpp gles2.hpp
shaders = $(wildcard shaders/*.glsl)
CC = gcc
CXX = g++
CPPFLAGS = -I. -Ishaders -I3rdparty -I/usr/include/harfbuzz -I/usr/include/freetype2
CFLAGS = -O2 -fPIC
CXXFLAGS = -std=c++11 -O2
LDFLAGS = -L.
LDLIBS = -lX11 -lEGL -lGLESv2 -licuuc -lharfbuzz-icu -lfreetype -lfontconfig

all: demo

glsl2h: glsl2h.c
	$(CC) -o $@ glsl2h.c

%.glsl.h: %.glsl glsl2h
	./glsl2h $< $@

libatmosphere.so: $(sources) $(headers) $(shaders:.glsl=.glsl.h)
	$(CXX) -shared -o $@ -fPIC $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(sources) $(LDLIBS)

demo: demo.cpp libatmosphere.so
	$(CXX) -o $@ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) demo.cpp -latmosphere

clean:
	rm -f demo libatmosphere.so 3rdparty/3rdparty.o glsl2h $(shaders:.glsl=.glsl.h)

.PHONY: all clean
