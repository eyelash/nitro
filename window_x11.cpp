#include "atmosphere.hpp"
#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdio>

static Display *display;
static Window window;
static EGLDisplay egl_display;
static EGLSurface surface;

static void set_time () {
	struct timespec ts;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	atmosphere::Animation::set_time (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

atmosphere::Window::Window (int width, int height, const char* title) {
	display = XOpenDisplay (NULL);
	egl_display = eglGetDisplay (display);
	eglInitialize (egl_display, NULL, NULL);

	// setup EGL
	EGLint attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		//EGL_SAMPLE_BUFFERS, 1,
		//EGL_SAMPLES, 4,
	EGL_NONE};
	EGLConfig config;
	EGLint num_configs_returned;
	eglChooseConfig (egl_display, attribs, &config, 1, &num_configs_returned);

	// get the visual from the EGL config
	EGLint visual_id;
	eglGetConfigAttrib (egl_display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
	XVisualInfo visual_template;
	visual_template.visualid = visual_id;
	int num_visuals_returned;
	XVisualInfo *visual = XGetVisualInfo (display, VisualIDMask, &visual_template, &num_visuals_returned);

	// create a window
	XSetWindowAttributes window_attributes;
	window_attributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;
	window_attributes.colormap = XCreateColormap (display, RootWindow(display,DefaultScreen(display)), visual->visual, AllocNone);
	window = XCreateWindow (
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
	//eglBindAPI (EGL_OPENGL_API);
	//EGLint context_attribs[] = {EGL_NONE};
	EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	EGLContext context = eglCreateContext (egl_display, config, EGL_NO_CONTEXT, context_attribs);
	if (context == EGL_NO_CONTEXT) fprintf (stderr, "eglCreateContext error\n");
	surface = eglCreateWindowSurface (egl_display, config, window, NULL);
	eglMakeCurrent (egl_display, surface, surface, context);
	eglSwapInterval (egl_display, 1);
	glViewport  (0, 0, width, height);
	glClearColor (0, 0, 0, 1);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_BLEND);
	//glEnable (0x809D); // GL_MULTISAMPLE

	XFree (visual);

	XStoreName (display, window, title);

	set_time ();
}

void atmosphere::Window::dispatch_events () {
	XEvent event;
	while (XPending (display)) {
		XNextEvent (display, &event);
		if (event.type == ConfigureNotify) {
			glViewport (0, 0, event.xconfigure.width, event.xconfigure.height);
			draw_context.projection = GLES2::glOrtho (0, event.xconfigure.width, 0, event.xconfigure.height, -1, 1);
			draw_context.clipping = GLES2::scale (0.f, 0.f);
		}
	}
}

void atmosphere::Window::run () {
	XMapWindow (display, window);
	while (true) {
		dispatch_events ();
		set_time ();
		Animation::apply_all ();
		glClear (GL_COLOR_BUFFER_BIT);
		root_node.draw (draw_context);
		//glFlush ();
		eglSwapBuffers (egl_display, surface);
	}
}

void atmosphere::Window::add_child (Node* node) {
	root_node.add_child (node);
}
