/*

Copyright (c) 2016-2019, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <X11/Xlib.h>
#include <epoxy/egl.h>
#include <poll.h>
#include <cstdio>

static Display* display;
static Window window;
static EGLDisplay egl_display;
static EGLSurface surface;
static bool running;
static Atom XA_WM_DELETE_WINDOW;

nitro::Window::Window(int width, int height, const char* title): draw_context(gles2::project(width, height)), needs_redraw(false) {
	display = XOpenDisplay(nullptr);
	egl_display = eglGetDisplay(display);
	eglInitialize(egl_display, nullptr, nullptr);

	// setup EGL
	constexpr EGLint attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		//EGL_SAMPLE_BUFFERS, 1,
		//EGL_SAMPLES, 4,
	EGL_NONE};
	EGLConfig config;
	EGLint num_configs_returned;
	eglChooseConfig(egl_display, attribs, &config, 1, &num_configs_returned);

	// get the visual from the EGL config
	EGLint visual_id;
	eglGetConfigAttrib(egl_display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
	XVisualInfo visual_template;
	visual_template.visualid = visual_id;
	int num_visuals_returned;
	XVisualInfo *visual = XGetVisualInfo(display, VisualIDMask, &visual_template, &num_visuals_returned);

	// create a window
	XSetWindowAttributes window_attributes;
	window_attributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;
	window_attributes.colormap = XCreateColormap(display, RootWindow(display,DefaultScreen(display)), visual->visual, AllocNone);
	window = XCreateWindow(
		display,
		RootWindow(display, DefaultScreen(display)),
		0, 0,
		width, height,
		0, // border width
		visual->depth, // depth
		InputOutput, // class
		visual->visual, // visual
		CWEventMask|CWColormap, // attribute mask
		&window_attributes // attributes
	);

	// EGL context and surface
	//eglBindAPI(EGL_OPENGL_API);
	//constexpr EGLint context_attribs[] = {EGL_NONE};
	constexpr EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	EGLContext context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext error\n");
	}
	//constexpr EGLint window_attribs[] = {EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB, EGL_NONE};
	surface = eglCreateWindowSurface(egl_display, config, window, nullptr);
	eglMakeCurrent(egl_display, surface, surface, context);
	eglSwapInterval(egl_display, 1);
	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	//glEnable(0x809D); // GL_MULTISAMPLE

	XFree(visual);

	XStoreName(display, window, title);
	XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &XA_WM_DELETE_WINDOW, 1);
}

void nitro::Window::layout() {
	Bin::layout();
	request_redraw();
}

void nitro::Window::request_redraw() {
	needs_redraw = true;
}

void nitro::Window::dispatch_events() {
	XEvent event;
	while (XPending(display)) {
		XNextEvent(display, &event);
		switch (event.type) {
		case Expose:
			needs_redraw = true;
			break;
		case MotionNotify:
			mouse_motion(Point(event.xmotion.x, get_height() - event.xmotion.y));
			break;
		case ButtonPress:
			mouse_button_press(Point(event.xbutton.x, get_height() - event.xbutton.y), event.xbutton.button);
			break;
		case ButtonRelease:
			mouse_button_release(Point(event.xbutton.x, get_height() - event.xbutton.y), event.xbutton.button);
			break;
		case EnterNotify:
			mouse_enter();
			break;
		case LeaveNotify:
			mouse_leave();
			break;
		case ConfigureNotify:
			set_size(event.xconfigure.width, event.xconfigure.height);
			glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
			draw_context.projection = gles2::project(event.xconfigure.width, event.xconfigure.height);
			break;
		case ClientMessage:
			if ((Atom)event.xclient.data.l[0] == XA_WM_DELETE_WINDOW) {
				running = false;
			}
			break;
		}
	}
}

void nitro::Window::run() {
	XMapWindow(display, window);
	running = true;
	while (running) {
		dispatch_events();
		Animation::advance_time(1.f / 60.f);
		Animation::apply_all();
		if (needs_redraw) {
			prepare_draw();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, get_width(), get_height());
			glClear(GL_COLOR_BUFFER_BIT);
			draw(draw_context);
			needs_redraw = false;
			//glFlush();
			eglSwapBuffers(egl_display, surface);
		}
		else {
			struct pollfd fd = {ConnectionNumber(display), POLLIN};
			poll(&fd, 1, 15);
		}
	}
}
