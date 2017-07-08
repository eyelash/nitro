/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <vector>
#include <stb_image.h>
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <cstring>

#include <color.fs.glsl.h>
#include <color.vs.glsl.h>
#include <texture.fs.glsl.h>
#include <texture.vs.glsl.h>
#include <color_mask.fs.glsl.h>
#include <color_mask.vs.glsl.h>
#include <texture_mask.fs.glsl.h>
#include <texture_mask.vs.glsl.h>

using namespace gles2;

// Node
nitro::Node::Node(): parent(nullptr), x(0.f), y(0.f), width(0.f), height(0.f), scale_x(1.f), scale_y(1.f), mouse_inside(false) {

}
nitro::Node::~Node() {

}
nitro::Node* nitro::Node::get_child(size_t index) {
	return nullptr;
}
void nitro::Node::prepare_draw() {
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->prepare_draw();
	}
}
void nitro::Node::draw(const DrawContext& draw_context) {
	for (int i = 0; Node* node = get_child(i); ++i) {
		DrawContext child_draw_context;
		child_draw_context.projection = draw_context.projection * node->get_transformation().get_matrix();
		node->draw(child_draw_context);
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
void nitro::Node::set_parent(Node* parent) {
	this->parent = parent;
}
nitro::Transformation nitro::Node::get_transformation() const {
	return Transformation {-width/2.f*scale_x+width/2.f+x, -height/2.f*scale_y+height/2.f+y, scale_x, scale_y};
}
float nitro::Node::get_location_x() const {
	return x;
}
void nitro::Node::set_location_x(float x) {
	this->x = x;
}
float nitro::Node::get_location_y() const {
	return y;
}
void nitro::Node::set_location_y(float y) {
	this->y = y;
}
float nitro::Node::get_width() const {
	return width;
}
void nitro::Node::set_width(float width) {
	this->width = width;
	layout();
}
float nitro::Node::get_height() const {
	return height;
}
void nitro::Node::set_height(float height) {
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
	set_location_x(x);
	set_location_y(y);
}
void nitro::Node::set_size(float width, float height) {
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
nitro::Bin::Bin(): child(nullptr), padding(0.f) {

}
nitro::Node* nitro::Bin::get_child(size_t index) {
	return index == 0 ? child : nullptr;
}
void nitro::Bin::layout() {
	if (child) {
		child->set_location(padding, padding);
		child->set_size(get_width() - 2.f * padding, get_height() - 2.f * padding);
	}
}
void nitro::Bin::set_child(Node* node) {
	child = node;
	if (child) {
		child->set_parent(this);
	}
	Bin::layout();
}
void nitro::Bin::set_padding(float padding) {
	this->padding = padding;
	Bin::layout();
}

// SimpleContainer
nitro::SimpleContainer::SimpleContainer() {

}
nitro::Node* nitro::SimpleContainer::get_child(size_t index) {
	return index < children.size() ? children[index] : nullptr;
}
void nitro::SimpleContainer::add_child(Node* node) {
	children.push_back(node);
}

static Program* get_color_program() {
	static Program program {color_vs_glsl, color_fs_glsl};
	return &program;
}
static Program* get_texture_program() {
	static Program program {texture_vs_glsl, texture_fs_glsl};
	return &program;
}
static Program* get_color_mask_program() {
	static Program program {color_mask_vs_glsl, color_mask_fs_glsl};
	return &program;
}
static Program* get_texture_mask_program() {
	static Program program {texture_mask_vs_glsl, texture_mask_fs_glsl};
	return &program;
}

// ColorNode
nitro::ColorNode::ColorNode() {

}
void nitro::ColorNode::draw(const DrawContext& draw_context) {
	if (get_width() <= 0.f || get_height() <= 0.f) return;

	Program* program = get_color_program();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		UniformMat4(program->get_uniform_location("projection"), draw_context.projection),
		AttributeVec4(program->get_attribute_location("color"), _color.unpremultiply()),
		AttributeArray(program->get_attribute_location("vertex"), 2, GL_FLOAT, vertices.get_data())
	);
}
const nitro::Color& nitro::ColorNode::get_color() const {
	return _color;
}
void nitro::ColorNode::set_color(const Color& color) {
	_color = color;
}
nitro::Property<nitro::Color> nitro::ColorNode::color() {
	return Property<Color> {this, [](ColorNode* rectangle) {
		return rectangle->_color;
	}, [](ColorNode* rectangle, Color color) {
		rectangle->_color = color;
	}};
}

// TextureNode
static std::shared_ptr<Texture> create_texture_from_file(const char* file_name, int& width, int& height) {
	std::shared_ptr<Texture> texture;
	if (!strcmp(file_name+strlen(file_name)-4, ".svg")) {
		NSVGimage* svg_image = nsvgParseFromFile(file_name, "px", 96);
		width = svg_image->width;
		height = svg_image->height;
		unsigned char* data = (unsigned char*) malloc(width * height * 4);
		NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
		nsvgRasterize(rasterizer, svg_image, 0.f, 0.f, 1.f, data, width, height, width * 4);
		nsvgDeleteRasterizer(rasterizer);
		nsvgDelete(svg_image);
		texture = std::make_shared<Texture>(width, height, 4, data);
		free(data);
	}
	else {
		int depth;
		unsigned char* data = stbi_load(file_name, &width, &height, &depth, 0);
		texture = std::make_shared<Texture>(width, height, depth, data);
		stbi_image_free(data);
	}
	return texture;
}
nitro::TextureNode::TextureNode(): _alpha(1.f) {

}
nitro::TextureNode nitro::TextureNode::create_from_file(const char* file_name, float x, float y) {
	int width, height;
	std::shared_ptr<Texture> texture = create_texture_from_file(file_name, width, height);
	TextureNode node;
	node.set_size(width, height);
	node.set_texture(texture, Quad(0.f, 1.f, 1.f, 0.f));
	node.set_location(x, y);
	return node;
}
void nitro::TextureNode::draw(const DrawContext& draw_context) {
	if (texture == nullptr) return;

	Program* program = get_texture_program();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		TextureState(texture.get(), GL_TEXTURE0, program->get_uniform_location("texture")),
		UniformMat4(program->get_uniform_location("projection"), draw_context.projection),
		UniformFloat(program->get_uniform_location("alpha"), _alpha),
		AttributeArray(program->get_attribute_location("vertex"), 2, GL_FLOAT, vertices.get_data()),
		AttributeArray(program->get_attribute_location("texcoord"), 2, GL_FLOAT, texcoord.get_data())
	);
}
std::shared_ptr<Texture> nitro::TextureNode::get_texture() const {
	return texture;
}
void nitro::TextureNode::set_texture(const std::shared_ptr<Texture>& texture, const Quad& texcoord) {
	this->texture = texture;
	this->texcoord = texcoord;
}
float nitro::TextureNode::get_alpha() const {
	return _alpha;
}
void nitro::TextureNode::set_alpha(float alpha) {
	_alpha = alpha;
}
nitro::Property<float> nitro::TextureNode::alpha() {
	return create_property<float, TextureNode, &TextureNode::get_alpha, &TextureNode::set_alpha>(this);
}

// ColorMaskNode
nitro::ColorMaskNode::ColorMaskNode() {

}
nitro::ColorMaskNode nitro::ColorMaskNode::create_from_file(const char* file_name, const Color& color) {
	int width, height;
	std::shared_ptr<Texture> mask = create_texture_from_file(file_name, width, height);
	ColorMaskNode node;
	node.set_size(width, height);
	node.set_mask(mask, Quad(0.f, 1.f, 1.f, 0.f));
	return node;
}
void nitro::ColorMaskNode::draw(const DrawContext& draw_context) {
	if (mask == nullptr) return;

	Program* program = get_color_mask_program();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		TextureState(mask.get(), GL_TEXTURE0, program->get_uniform_location("mask")),
		UniformMat4(program->get_uniform_location("projection"), draw_context.projection),
		AttributeVec4(program->get_attribute_location("color"), _color.unpremultiply()),
		AttributeArray(program->get_attribute_location("vertex"), 2, GL_FLOAT, vertices.get_data()),
		AttributeArray(program->get_attribute_location("mask_texcoord"), 2, GL_FLOAT, mask_texcoord.get_data())
	);
}
const nitro::Color& nitro::ColorMaskNode::get_color() const {
	return _color;
}
void nitro::ColorMaskNode::set_color(const Color& color) {
	_color = color;
}
nitro::Property<nitro::Color> nitro::ColorMaskNode::color() {
	return Property<Color> {this, [](ColorMaskNode* mask) {
		return mask->get_color();
	}, [](ColorMaskNode* mask, Color color) {
		mask->set_color(color);
	}};
}
void nitro::ColorMaskNode::set_mask(const std::shared_ptr<Texture>& mask, const Quad& mask_texcoord) {
	this->mask = mask;
	this->mask_texcoord = mask_texcoord;
}

// TextureMaskNode
nitro::TextureMaskNode::TextureMaskNode(): _alpha(1.f) {

}
void nitro::TextureMaskNode::draw(const DrawContext& draw_context) {
	if (texture == nullptr || mask == nullptr) return;

	Program* program = get_texture_mask_program();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		TextureState(texture.get(), GL_TEXTURE0, program->get_uniform_location("texture")),
		TextureState(mask.get(), GL_TEXTURE1, program->get_uniform_location("mask")),
		UniformMat4(program->get_uniform_location("projection"), draw_context.projection),
		UniformFloat(program->get_uniform_location("alpha"), _alpha),
		AttributeArray(program->get_attribute_location("vertex"), 2, GL_FLOAT, vertices.get_data()),
		AttributeArray(program->get_attribute_location("texcoord"), 2, GL_FLOAT, texcoord.get_data()),
		AttributeArray(program->get_attribute_location("mask_texcoord"), 2, GL_FLOAT, mask_texcoord.get_data())
	);
}
void nitro::TextureMaskNode::set_texture(const std::shared_ptr<Texture>& texture, const Quad& texcoord) {
	this->texture = texture;
	this->texcoord = texcoord;
}
void nitro::TextureMaskNode::set_mask(const std::shared_ptr<Texture>& mask, const Quad& mask_texcoord) {
	this->mask = mask;
	this->mask_texcoord = mask_texcoord;
}
float nitro::TextureMaskNode::get_alpha() const {
	return _alpha;
}
void nitro::TextureMaskNode::set_alpha(float alpha) {
	_alpha = alpha;
}
nitro::Property<float> nitro::TextureMaskNode::alpha() {
	return create_property<float, TextureMaskNode, &TextureMaskNode::get_alpha, &TextureMaskNode::set_alpha>(this);
}

// Rectangle
nitro::Rectangle::Rectangle(const Color& color) {
	set_color(color);
}
nitro::Node* nitro::Rectangle::get_child(size_t index) {
	return index == 0 ? &node : Bin::get_child(index-1);
}
void nitro::Rectangle::layout() {
	node.set_size(get_width(), get_height());
	Bin::layout();
}
const nitro::Color& nitro::Rectangle::get_color() const {
	return node.get_color();
}
void nitro::Rectangle::set_color(const Color& color) {
	node.set_color(color);
}
nitro::Property<nitro::Color> nitro::Rectangle::color() {
	return Property<Color> {this, [](Rectangle* rectangle) {
		return rectangle->get_color();
	}, [](Rectangle* rectangle, Color color) {
		rectangle->set_color(color);
	}};
}

// Clip
nitro::Clip::Clip() {

}
void nitro::Clip::prepare_draw() {
	if (Node* child = get_child(0)) {
		child->prepare_draw();
		fbo->use();
		DrawContext draw_context;
		draw_context.projection = gles2::project(get_width(), get_height(), get_width() * 2.f);
		child->draw(draw_context);
	}
}
void nitro::Clip::draw(const DrawContext& draw_context) {
	image.draw(draw_context);
}
void nitro::Clip::layout() {
	Bin::layout();
	fbo = std::make_shared<FramebufferObject>(get_width(), get_height());
	image.set_texture(fbo->get_texture(), Quad(0.f, 0.f, 1.f, 1.f));
	image.set_width(get_width());
	image.set_height(get_height());
}
float nitro::Clip::get_alpha() const {
	return image.get_alpha();
}
void nitro::Clip::set_alpha(float alpha) {
	image.set_alpha(alpha);
}
nitro::Property<float> nitro::Clip::alpha() {
	return image.alpha();
}

// RoundedRectangle
namespace {
	float circle(float x) {
		return sqrtf(1.f - x * x);
	}
	float int_circle(float x) {
		return 0.5f * (sqrtf(1.f-x*x) * x + asinf(x));
	}
	// computes (the area of) the intersection of the unit circle with the given rectangle (x, y, w, h)
	// x, y, w and h are all assumed to be non-negative
	float rounded_corner_area(float x, float y, const float w, const float h) {
		if (x*x+y*y >= 1.f) return 0.f;
		// assert(x < 1.f && y < 1.f);

		float x1 = std::min(x + w, 1.f);
		float y1 = std::min(y + h, 1.f);
		float result = 0.f;

		const float cy1 = circle(y1);
		if (cy1 >= x1) return w * h; // completely inside
		if (cy1 > x) {
			result += (cy1 - x) * h;
			x = cy1;
		}

		const float cy = circle(y);
		if (cy < x1) x1 = cy;

		result += int_circle(x1) - int_circle(x) - (x1 - x) * y;
		return result;
	}
	float rounded_corner(float radius, float x, float y, float w = 1.f, float h = 1.f) {
		w = w / radius;
		h = h / radius;
		return rounded_corner_area(x/radius, y/radius, w, h) / (w * h);
	}
	std::shared_ptr<Texture> create_rounded_corner_texture(int radius) {
		std::vector<unsigned char> data (radius * radius);
		for (int y = 0; y < radius; ++y) {
			for (int x = 0; x < radius; ++x) {
				data[y*radius+x] = rounded_corner((float)radius, (float)x, (float)y) * 255.f + 0.5f;
			}
		}
		return std::make_shared<Texture>(radius, radius, 1, data.data());
	}
}
nitro::RoundedRectangle::RoundedRectangle(const Color& color, float radius): radius(radius) {
	std::shared_ptr<Texture> mask = create_rounded_corner_texture(radius);
	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask, texcoord);

	set_color(color);
}
nitro::Node* nitro::RoundedRectangle::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &center;
		case 6: return &top;
		default: return Bin::get_child(index-7);
	}
}
void nitro::RoundedRectangle::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);

	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, radius);
	center.set_location(0.f, radius);
	center.set_size(get_width(), get_height() - 2.f * radius);
	top.set_location(radius, get_height() - radius);
	top.set_size(get_width() - 2.f * radius, radius);

	Bin::layout();
}
const nitro::Color& nitro::RoundedRectangle::get_color() const {
	return center.get_color();
}
void nitro::RoundedRectangle::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	center.set_color(color);
	top.set_color(color);
}
nitro::Property<nitro::Color> nitro::RoundedRectangle::color() {
	return Property<Color> {this, [](RoundedRectangle* rectangle) {
		return rectangle->get_color();
	}, [](RoundedRectangle* rectangle, Color color) {
		rectangle->set_color(color);
	}};
}

// RoundedImage
nitro::RoundedImage::RoundedImage(const std::shared_ptr<Texture>& texture, const Quad& texcoord, float radius): texcoord(texcoord), radius(radius) {
	std::shared_ptr<Texture> mask = create_rounded_corner_texture(radius);
	Quad mask_texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask, mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	top_left.set_mask(mask, mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	bottom_left.set_mask(mask, mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	bottom_right.set_mask(mask, mask_texcoord);

	set_texture(texture, texcoord);
}
nitro::RoundedImage nitro::RoundedImage::create_from_file(const char* file_name, float radius) {
	int width, height;
	std::shared_ptr<Texture> texture = create_texture_from_file(file_name, width, height);
	RoundedImage rounded_image (texture, Quad(0.f, 1.f, 1.f, 0.f), radius);
	rounded_image.set_size(width, height);
	return rounded_image;
}
nitro::Node* nitro::RoundedImage::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &center;
		case 6: return &top;
		default: return Bin::get_child(index-7);
	}
}
void nitro::RoundedImage::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);

	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, radius);
	center.set_location(0.f, radius);
	center.set_size(get_width(), get_height() - 2.f * radius);
	top.set_location(radius, get_height() - radius);
	top.set_size(get_width() - 2.f * radius, radius);

	set_texture(center.get_texture(), texcoord);

	Bin::layout();
}
void nitro::RoundedImage::set_texture(const std::shared_ptr<Texture>& texture, const Quad& texcoord) {
	this->texcoord = texcoord;
	const float width = get_width();
	const float height = get_height();
	bottom_left.set_texture(texture, texcoord*Quad(0, 0, radius/width, radius/height));
	bottom_right.set_texture(texture, texcoord*Quad(1-radius/width, 0, 1, radius/height));
	top_left.set_texture(texture, texcoord*Quad(0, 1-radius/height, radius/width, 1));
	top_right.set_texture(texture, texcoord*Quad(1-radius/width, 1-radius/height, 1, 1));
	bottom.set_texture(texture, texcoord*Quad(radius/width, 0, 1-radius/width, radius/height));
	center.set_texture(texture, texcoord*Quad(0, radius/height, 1, 1-radius/height));
	top.set_texture(texture, texcoord*Quad(radius/width, 1-radius/height, 1-radius/width, 1));
}
float nitro::RoundedImage::get_alpha() const {
	return center.get_alpha();
}
void nitro::RoundedImage::set_alpha(float alpha) {
	bottom_left.set_alpha(alpha);
	bottom_right.set_alpha(alpha);
	top_left.set_alpha(alpha);
	top_right.set_alpha(alpha);
	bottom.set_alpha(alpha);
	center.set_alpha(alpha);
	top.set_alpha(alpha);
}
nitro::Property<float> nitro::RoundedImage::alpha() {
	return create_property<float, RoundedImage, &RoundedImage::get_alpha, &RoundedImage::set_alpha>(this);
}

// RoundedBorder
namespace {
	std::shared_ptr<Texture> create_rounded_border_texture(int radius, int width) {
		std::vector<unsigned char> data (radius * radius);
		for (int y = 0; y < radius; ++y) {
			for (int x = 0; x < radius; ++x) {
				const float value = rounded_corner((float)radius, (float)x, (float)y) - rounded_corner((float)(radius-width), (float)x, (float)y);
				data[y*radius+x] = value * 255.f + 0.5f;
			}
		}
		return std::make_shared<Texture>(radius, radius, 1, data.data());
	}
}
nitro::RoundedBorder::RoundedBorder(float border_width, const Color& color, float radius): border_width(border_width), radius(radius) {
	std::shared_ptr<Texture> mask = create_rounded_border_texture(radius, border_width);
	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask, texcoord);

	set_color(color);
}
nitro::Node* nitro::RoundedBorder::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &left;
		case 6: return &right;
		case 7: return &top;
		default: return Bin::get_child(index-8);
	}
}
void nitro::RoundedBorder::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);
	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, border_width);
	left.set_location(0.f, radius);
	left.set_size(border_width, get_height() - 2.f * radius);
	right.set_location(get_width() - border_width, radius);
	right.set_size(border_width, get_height() - 2.f * radius);
	top.set_location(radius, get_height() - border_width);
	top.set_size(get_width() - 2.f * radius, border_width);
	Bin::layout();
}
const nitro::Color& nitro::RoundedBorder::get_color() const {
	return bottom.get_color();
}
void nitro::RoundedBorder::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	left.set_color(color);
	right.set_color(color);
	top.set_color(color);
}
nitro::Property<nitro::Color> nitro::RoundedBorder::color() {
	return Property<Color> {this, [](RoundedBorder* border) {
		return border->get_color();
	}, [](RoundedBorder* border, Color color) {
		border->set_color(color);
	}};
}

// BlurredRectangle
namespace {
	std::vector<float> create_gaussian_kernel(int radius) {
		std::vector<float> kernel (radius * 2 + 1);
		const float sigma = radius / 3.f;
		const float factor = 1.f / sqrtf(2.f * M_PI * sigma*sigma);
		for (unsigned int i = 0; i < kernel.size(); ++i) {
			const float x = (int)i - radius;
			kernel[i] = factor * expf(-x*x/(2.f*sigma*sigma));
		}
		return kernel;
	}
	void blur(const std::vector<float>& kernel, std::vector<unsigned char>& buffer, float w, float h) {
		const int center = kernel.size() / 2;
		std::vector<unsigned char> tmp (buffer.size());
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				float sum = 0.f;
				for (unsigned int k = 0; k < kernel.size(); ++k) {
					int kx = x + k - center;
					if (kx < 0) kx = 0;
					else if (kx >= w) kx = w - 1;
					sum += buffer[y * w + kx] * kernel[k];
				}
				tmp[y * w + x] = sum + .5f;
			}
		}
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				float sum = 0.f;
				for (unsigned int k = 0; k < kernel.size(); ++k) {
					int ky = y + k - center;
					if (ky < 0) ky = 0;
					else if (ky >= h) ky = h - 1;
					sum += tmp[ky * w + x] * kernel[k];
				}
				buffer[y * w + x] = sum + .5f;
			}
		}
	}
	std::shared_ptr<Texture> create_blurred_corner_texture(int radius, int blur_radius) {
		int size = radius + blur_radius * 2;
		std::vector<unsigned char> buffer (size * size);
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				const int i = y * size + x;
				const bool inside = x - blur_radius < radius && y - blur_radius < radius;
				buffer[i] = inside ? 255 : 0;
			}
		}
		for (int y = 0; y < radius; ++y) {
			for (int x = 0; x < radius; ++x) {
				const int i = (y + blur_radius) * size + (x + blur_radius);
				buffer[i] = rounded_corner(radius, x, y) * 255.f + .5f;
			}
		}
		blur(create_gaussian_kernel(blur_radius), buffer, size, size);
		return std::make_shared<Texture>(size, size, 1, buffer.data());
	}
}
nitro::BlurredRectangle::BlurredRectangle(const Color& color, float radius, float blur_radius): radius(radius), blur_radius(blur_radius) {
	std::shared_ptr<Texture> mask = create_blurred_corner_texture(radius, blur_radius);

	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask, texcoord);

	texcoord = Quad(0.f, 0.f, 0.f, 1.f);
	top.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	left.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	bottom.set_mask(mask, texcoord);
	texcoord = texcoord.rotate();
	right.set_mask(mask, texcoord);

	set_color(color);
}
nitro::Node* nitro::BlurredRectangle::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &left;
		case 6: return &right;
		case 7: return &top;
		case 8: return &center;
		default: return Bin::get_child(index-9);
	}
}
void nitro::BlurredRectangle::layout() {
	const float size = radius + 2.f * blur_radius;
	bottom_left.set_location(-blur_radius, -blur_radius);
	bottom_left.set_size(size, size);
	bottom_right.set_location(get_width() - radius - blur_radius, -blur_radius);
	bottom_right.set_size(size, size);
	top_left.set_location(-blur_radius, get_height() - radius - blur_radius);
	top_left.set_size(size, size);
	top_right.set_location(get_width() - radius - blur_radius, get_height() - radius - blur_radius);
	top_right.set_size(size, size);
	bottom.set_location(radius + blur_radius, -blur_radius);
	bottom.set_size(get_width() - 2.f * (radius + blur_radius), size);
	left.set_location(-blur_radius, radius + blur_radius);
	left.set_size(size, get_height() - 2.f * (radius + blur_radius));
	right.set_location(get_width() - radius - blur_radius, radius + blur_radius);
	right.set_size(size, get_height() - 2.f * (radius + blur_radius));
	top.set_location(radius + blur_radius, get_height() - radius - blur_radius);
	top.set_size(get_width() - 2.f * (radius + blur_radius), size);
	center.set_location(radius + blur_radius, radius + blur_radius);
	center.set_size(get_width() - 2.f * (radius + blur_radius),  get_height() - 2.f * (radius + blur_radius));
	Bin::layout();
}
const nitro::Color& nitro::BlurredRectangle::get_color() const {
	return center.get_color();
}
void nitro::BlurredRectangle::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	left.set_color(color);
	right.set_color(color);
	top.set_color(color);
	center.set_color(color);
}
