/*

Copyright (c) 2016-2018, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <vector>
#include <png.h>
#include <cstring>

// Texture
nitro::Texture::Texture() {

}
nitro::Texture::Texture(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord): texture(texture), texcoord(texcoord) {

}
nitro::Texture nitro::Texture::create_from_data(int width, int height, int depth, const unsigned char* data, bool mirror_y) {
	return Texture(std::make_shared<gles2::Texture>(width, height, depth, data), mirror_y ? Quad(0, 1, 1, 0) : Quad(0, 0, 1, 1));
}
nitro::Texture nitro::Texture::create_from_file(const char* file_name, int& width, int& height) {
	FILE* file = fopen(file_name, "rb");
	if (file == nullptr) {
		fprintf(stderr, "error opening file %s\n", file_name);
		return Texture(nullptr, Quad());
	}
	unsigned char signature[8];
	fread(signature, 1, 8, file);
	if (png_sig_cmp(signature, 0, 8)) {
		fprintf(stderr, "file %s is not a valid PNG file\n", file_name);
		return Texture(nullptr, Quad());
	}
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info = png_create_info_struct(png);
	png_init_io(png, file);
	png_set_sig_bytes(png, 8);
	png_read_info(png, info);
	width = png_get_image_width(png, info);
	height = png_get_image_height(png, info);
	if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}
	if (png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png);
	}
	if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png, info) < 8) {
		png_set_expand_gray_1_2_4_to_8(png);
	}
	if (png_get_bit_depth(png, info) == 16) {
		png_set_scale_16(png);
	}
	png_read_update_info(png, info);
	int channels = png_get_channels(png, info);
	int rowbytes = png_get_rowbytes(png, info);
	std::vector<unsigned char> data(rowbytes * height);
	for (int i = 0; i < height; ++i) {
		png_read_row(png, data.data() + i * rowbytes, nullptr);
	}
	png_destroy_read_struct(&png, &info, nullptr);
	fclose(file);
	return create_from_data(width, height, channels, data.data(), true);
}
nitro::Texture::operator bool() const {
	return texture != nullptr;
}
nitro::Texture nitro::Texture::operator *(const Quad& t) const {
	return Texture(texture, texcoord * t);
}

// Node
nitro::Node::Node(): parent(nullptr), x(0.f), y(0.f), width(0.f), height(0.f), scale_x(1.f), scale_y(1.f), mouse_inside(false) {

}
nitro::Node::~Node() {

}
nitro::Node* nitro::Node::get_child(std::size_t index) {
	return nullptr;
}
void nitro::Node::prepare_draw() {
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->prepare_draw();
	}
}
void nitro::Node::draw(const DrawContext& draw_context) {
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->draw(DrawContext(draw_context.projection * node->get_transformation().get_matrix()));
	}
}
void nitro::Node::layout() {

}
void nitro::Node::mouse_enter() {
	mouse_inside = true;
}
void nitro::Node::mouse_leave() {
	mouse_inside = false;
	for (int i = 0; Node* child = get_child(i); ++i) {
		if (child->is_mouse_inside()) child->mouse_leave();
	}
}
void nitro::Node::mouse_motion(const Point& point) {
	for (int i = 0; Node* child = get_child(i); ++i) {
		const Point child_point = child->get_transformation().get_inverse() * point;
		if (mouse_inside) {
			if (child_point.x >= 0.f && child_point.x < child->get_width() && child_point.y >= 0.f && child_point.y < child->get_height()) {
				if (!child->is_mouse_inside()) child->mouse_enter();
			}
			else {
				if (child->is_mouse_inside()) child->mouse_leave();
			}
		}
		child->mouse_motion(child_point);
	}
}
void nitro::Node::mouse_button_press(const Point& point, int button) {
	for (int i = 0; Node* child = get_child(i); ++i) {
		const Point child_point = child->get_transformation().get_inverse() * point;
		child->mouse_button_press(child_point, button);
	}
}
void nitro::Node::mouse_button_release(const Point& point, int button) {
	for (int i = 0; Node* child = get_child(i); ++i) {
		const Point child_point = child->get_transformation().get_inverse() * point;
		child->mouse_button_release(child_point, button);
	}
}
void nitro::Node::request_redraw() {
	if (parent) {
		parent->request_redraw();
	}
}
void nitro::Node::set_parent(Node* parent) {
	this->parent = parent;
}
nitro::Transformation nitro::Node::get_transformation() const {
	return Transformation(-width/2.f*scale_x+width/2.f+x, -height/2.f*scale_y+height/2.f+y, scale_x, scale_y);
}
float nitro::Node::get_location_x() const {
	return x;
}
void nitro::Node::set_location_x(float x) {
	if (x == this->x) {
		return;
	}
	this->x = x;
	request_redraw();
}
float nitro::Node::get_location_y() const {
	return y;
}
void nitro::Node::set_location_y(float y) {
	if (y == this->y) {
		return;
	}
	this->y = y;
	request_redraw();
}
float nitro::Node::get_width() const {
	return width;
}
void nitro::Node::set_width(float width) {
	if (width == this->width) {
		return;
	}
	this->width = width;
	layout();
}
float nitro::Node::get_height() const {
	return height;
}
void nitro::Node::set_height(float height) {
	if (height == this->height) {
		return;
	}
	this->height = height;
	layout();
}
float nitro::Node::get_scale_x() const {
	return scale_x;
}
void nitro::Node::set_scale_x(float scale_x) {
	this->scale_x = scale_x;
}
float nitro::Node::get_scale_y() const {
	return scale_y;
}
void nitro::Node::set_scale_y(float scale_x) {
	this->scale_y = scale_y;
}
void nitro::Node::set_location(float x, float y) {
	if (x == this->x && y == this->y) {
		return;
	}
	this->x = x;
	this->y = y;
	request_redraw();
}
void nitro::Node::set_size(float width, float height) {
	if (width == this->width && height == this->height) {
		return;
	}
	this->width = width;
	this->height = height;
	layout();
}
void nitro::Node::set_scale(float scale_x, float scale_y) {
	this->scale_x = scale_x;
	this->scale_y = scale_y;
}
bool nitro::Node::is_mouse_inside() const {
	return mouse_inside;
}
nitro::Property<float> nitro::Node::position_x() {
	return create_property<float, Node, &Node::get_location_x, &Node::set_location_x>(this);
}
nitro::Property<float> nitro::Node::position_y() {
	return create_property<float, Node, &Node::get_location_y, &Node::set_location_y>(this);
}

// Bin
nitro::Bin::Bin(): child(nullptr) {

}
nitro::Node* nitro::Bin::get_child(std::size_t index) {
	return index == 0 ? child : nullptr;
}
void nitro::Bin::layout() {
	if (child) {
		child->set_size(get_width(), get_height());
	}
}
nitro::Node* nitro::Bin::get_child() {
	return child;
}
void nitro::Bin::set_child(Node* child) {
	if (child == this->child) {
		return;
	}
	this->child = child;
	if (child) {
		child->set_parent(this);
	}
	layout();
}

// SimpleContainer
nitro::SimpleContainer::SimpleContainer() {

}
nitro::Node* nitro::SimpleContainer::get_child(std::size_t index) {
	return index < children.size() ? children[index] : nullptr;
}
void nitro::SimpleContainer::add_child(Node* node) {
	children.push_back(node);
	node->set_parent(this);
}

// Window
nitro::Window::Window(int width, int height): draw_context(gles2::project(width, height)), needs_redraw(false) {

}
void nitro::Window::layout() {
	draw_context.projection = gles2::project(get_width(), get_height());
	Bin::layout();
	request_redraw();
}
void nitro::Window::request_redraw() {
	needs_redraw = true;
}
void nitro::Window::quit() {
	running = false;
}
