/*

Copyright (c) 2018-2020, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <libinput.h>
#include <xkbcommon/xkbcommon.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <epoxy/egl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>

static libinput* libinput;
static xkb_context* xkb_context;
static xkb_keymap* keymap;
static xkb_state* xkb_state;
static int device;
static uint32_t connector_id;
static drmModeModeInfo mode_info;
static drmModeCrtc* crtc;
static gbm_surface* gbm_surface;
static EGLDisplay egl_display;
static EGLSurface surface;

static drmModeConnector* find_connector(drmModeRes* resources) {
	for (int i = 0; i < resources->count_connectors; ++i) {
		drmModeConnector* connector = drmModeGetConnector(device, resources->connectors[i]);
		// pick the first connected connector
		if (connector->connection == DRM_MODE_CONNECTED) {
			return connector;
		}
		drmModeFreeConnector(connector);
	}
	return nullptr;
}

static drmModeEncoder* find_encoder(drmModeRes* resources, drmModeConnector* connector) {
	if (connector->encoder_id) {
		return drmModeGetEncoder(device, connector->encoder_id);
	}
	return nullptr;
}

static drmModeCrtc* find_crtc(drmModeRes* resources, drmModeEncoder* encoder) {
	if (encoder->crtc_id) {
		return drmModeGetCrtc(device, encoder->crtc_id);
	}
	return nullptr;
}

static int open_restricted(const char* path, int flags, void* user_data) {
	return open(path, flags);
}

static void close_restricted(int fd, void* user_data) {
	close(fd);
}

nitro::WindowDRM::WindowDRM(int width, int height, const char* title): Window(width, height) {
	udev* udev = udev_new();

	const libinput_interface interface = {&open_restricted, &close_restricted};
	libinput = libinput_udev_create_context(&interface, nullptr, udev);
	libinput_udev_assign_seat(libinput, "seat0");

	xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	keymap = xkb_keymap_new_from_names(xkb_context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
	xkb_state = xkb_state_new(keymap);

	device = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	drmModeRes* resources = drmModeGetResources(device);
	drmModeConnector* connector = find_connector(resources);
	connector_id = connector->connector_id;
	mode_info = connector->modes[0];
	printf("resolution: %ux%u\n", mode_info.hdisplay, mode_info.vdisplay);
	drmModeEncoder* encoder = find_encoder(resources, connector);
	crtc = find_crtc(resources, encoder);

	drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);

	gbm_device* gbm_device = gbm_create_device(device);
	egl_display = eglGetDisplay(gbm_device);
	eglInitialize(egl_display, nullptr, nullptr);

	// setup EGL
	constexpr EGLint attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 8,
		//EGL_SAMPLE_BUFFERS, 1,
		//EGL_SAMPLES, 4,
	EGL_NONE};
	EGLConfig config;
	EGLint num_configs_returned;
	eglChooseConfig(egl_display, attribs, &config, 1, &num_configs_returned);

	gbm_surface = gbm_surface_create(gbm_device, mode_info.hdisplay, mode_info.vdisplay, GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);

	// EGL context and surface
	//eglBindAPI(EGL_OPENGL_API);
	//constexpr EGLint context_attribs[] = {EGL_NONE};
	constexpr EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	EGLContext context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext error\n");
	}
	//constexpr EGLint window_attribs[] = {EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB, EGL_NONE};
	surface = eglCreateWindowSurface(egl_display, config, gbm_surface, nullptr);
	eglMakeCurrent(egl_display, surface, surface, context);
	eglSwapInterval(egl_display, 1);
	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	//glEnable(0x809D); // GL_MULTISAMPLE

	set_size(mode_info.hdisplay, mode_info.vdisplay);
}

void nitro::WindowDRM::draw(const DrawContext& draw_context) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, get_width(), get_height());
	glClear(GL_COLOR_BUFFER_BIT);
	Bin::draw(draw_context);
	eglSwapBuffers(egl_display, surface);
	gbm_bo* bo = gbm_surface_lock_front_buffer(gbm_surface);
	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t pitch = gbm_bo_get_stride(bo);
	uint32_t fb;
	drmModeAddFB(device, mode_info.hdisplay, mode_info.vdisplay, 24, 32, pitch, handle, &fb);
	drmModeSetCrtc(device, crtc->crtc_id, fb, 0, 0, &connector_id, 1, &mode_info);

	static gbm_bo* previous_bo = nullptr;
	static uint32_t previous_fb;
	if (previous_bo) {
		drmModeRmFB(device, previous_fb);
		gbm_surface_release_buffer(gbm_surface, previous_bo);
	}
	previous_bo = bo;
	previous_fb = fb;
}

int nitro::WindowDRM::get_fd() {
	return libinput_get_fd(libinput);
}

void nitro::WindowDRM::dispatch_events() {
	libinput_dispatch(libinput);
	while (libinput_event* event = libinput_get_event(libinput)) {
		const int type = libinput_event_get_type(event);
		switch (type) {
		case LIBINPUT_EVENT_KEYBOARD_KEY:
			libinput_event_keyboard* keyboard_event = libinput_event_get_keyboard_event(event);
			const uint32_t key = libinput_event_keyboard_get_key(keyboard_event);
			const libinput_key_state state = libinput_event_keyboard_get_key_state(keyboard_event);
			xkb_state_update_key(xkb_state, key + 8, static_cast<xkb_key_direction>(state));
			if (state == LIBINPUT_KEY_STATE_PRESSED && xkb_state_key_get_utf32(xkb_state, key + 8) == 'q') {
				quit();
			}
			break;
		}
		libinput_event_destroy(event);
	}
}
