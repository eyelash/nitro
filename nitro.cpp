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

#include <color.fs.glsl.h>
#include <color.vs.glsl.h>
#include <texture.fs.glsl.h>
#include <texture.vs.glsl.h>
#include <color_mask.fs.glsl.h>
#include <color_mask.vs.glsl.h>
#include <texture_mask.fs.glsl.h>
#include <texture_mask.vs.glsl.h>

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
nitro::Node* nitro::Bin::get_child(size_t index) {
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
nitro::Node* nitro::SimpleContainer::get_child(size_t index) {
	return index < children.size() ? children[index] : nullptr;
}
void nitro::SimpleContainer::add_child(Node* node) {
	children.push_back(node);
	node->set_parent(this);
}

class ColorProgram: public gles2::Program {
	GLint projection_location;
	GLint vertex_location;
	GLint color_location;
	ColorProgram(): gles2::Program(color_vs_glsl, color_fs_glsl) {
		projection_location = get_uniform_location("projection");
		vertex_location = get_attribute_location("vertex");
		color_location = get_attribute_location("color");
	}
public:
	static ColorProgram* get() {
		static ColorProgram program;
		return &program;
	}
	GLint get_projection_location() const {
		return projection_location;
	}
	GLint get_vertex_location() const {
		return vertex_location;
	}
	GLint get_color_location() const {
		return color_location;
	}
};
class TextureProgram: public gles2::Program {
	GLint projection_location;
	GLint vertex_location;
	GLint texcoord_location;
	GLint texture_location;
	GLint alpha_location;
	TextureProgram(): gles2::Program(texture_vs_glsl, texture_fs_glsl) {
		projection_location = get_uniform_location("projection");
		vertex_location = get_attribute_location("vertex");
		texcoord_location = get_attribute_location("texcoord");
		texture_location = get_uniform_location("texture");
		alpha_location = get_uniform_location("alpha");
	}
public:
	static TextureProgram* get() {
		static TextureProgram program;
		return &program;
	}
	GLint get_projection_location() const {
		return projection_location;
	}
	GLint get_vertex_location() const {
		return vertex_location;
	}
	GLint get_texcoord_location() const {
		return texcoord_location;
	}
	GLint get_texture_location() const {
		return texture_location;
	}
	GLint get_alpha_location() const {
		return alpha_location;
	}
};
class ColorMaskProgram: public gles2::Program {
	GLint projection_location;
	GLint vertex_location;
	GLint color_location;
	GLint mask_texcoord_location;
	GLint mask_location;
	ColorMaskProgram(): gles2::Program(color_mask_vs_glsl, color_mask_fs_glsl) {
		projection_location = get_uniform_location("projection");
		vertex_location = get_attribute_location("vertex");
		color_location = get_attribute_location("color");
		mask_texcoord_location = get_attribute_location("mask_texcoord");
		mask_location = get_uniform_location("mask");
	}
public:
	static ColorMaskProgram* get() {
		static ColorMaskProgram program;
		return &program;
	}
	GLint get_projection_location() const {
		return projection_location;
	}
	GLint get_vertex_location() const {
		return vertex_location;
	}
	GLint get_color_location() const {
		return color_location;
	}
	GLint get_mask_texcoord_location() const {
		return mask_texcoord_location;
	}
	GLint get_mask_location() const {
		return mask_location;
	}
};
class TextureMaskProgram: public gles2::Program {
	GLint projection_location;
	GLint vertex_location;
	GLint texcoord_location;
	GLint mask_texcoord_location;
	GLint texture_location;
	GLint alpha_location;
	GLint mask_location;
	TextureMaskProgram(): gles2::Program(texture_mask_vs_glsl, texture_mask_fs_glsl) {
		projection_location = get_uniform_location("projection");
		vertex_location = get_attribute_location("vertex");
		texcoord_location = get_attribute_location("texcoord");
		mask_texcoord_location = get_attribute_location("mask_texcoord");
		texture_location = get_uniform_location("texture");
		alpha_location = get_uniform_location("alpha");
		mask_location = get_uniform_location("mask");
	}
public:
	static TextureMaskProgram* get() {
		static TextureMaskProgram program;
		return &program;
	}
	GLint get_projection_location() const {
		return projection_location;
	}
	GLint get_vertex_location() const {
		return vertex_location;
	}
	GLint get_texcoord_location() const {
		return texcoord_location;
	}
	GLint get_mask_texcoord_location() const {
		return mask_texcoord_location;
	}
	GLint get_texture_location() const {
		return texture_location;
	}
	GLint get_alpha_location() const {
		return alpha_location;
	}
	GLint get_mask_location() const {
		return mask_location;
	}
};

// ColorNode
nitro::ColorNode::ColorNode() {

}
void nitro::ColorNode::draw(const DrawContext& draw_context) {
	if (get_width() <= 0.f || get_height() <= 0.f) {
		return;
	}

	ColorProgram* program = ColorProgram::get();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		gles2::UniformMat4(program->get_projection_location(), draw_context.projection),
		gles2::AttributeVec4(program->get_color_location(), _color.unpremultiply()),
		gles2::AttributeArray(program->get_vertex_location(), 2, GL_FLOAT, vertices.get_data())
	);
}
const nitro::Color& nitro::ColorNode::get_color() const {
	return _color;
}
void nitro::ColorNode::set_color(const Color& color) {
	if (color == _color) {
		return;
	}
	_color = color;
	request_redraw();
}
nitro::Property<nitro::Color> nitro::ColorNode::color() {
	return Property<Color> {this, [](ColorNode* rectangle) {
		return rectangle->_color;
	}, [](ColorNode* rectangle, Color color) {
		rectangle->_color = color;
	}};
}

// TextureNode
nitro::TextureNode::TextureNode(): _alpha(1.f) {

}
nitro::TextureNode nitro::TextureNode::create_from_file(const char* file_name, float x, float y) {
	int width, height;
	Texture texture = Texture::create_from_file(file_name, width, height);
	TextureNode node;
	node.set_size(width, height);
	node.set_texture(texture);
	node.set_location(x, y);
	return node;
}
void nitro::TextureNode::draw(const DrawContext& draw_context) {
	if (get_width() <= 0.f || get_height() <= 0.f || texture.texture == nullptr) {
		return;
	}

	TextureProgram* program = TextureProgram::get();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		gles2::TextureState(texture.texture.get(), GL_TEXTURE0, program->get_texture_location()),
		gles2::UniformMat4(program->get_projection_location(), draw_context.projection),
		gles2::UniformFloat(program->get_alpha_location(), _alpha),
		gles2::AttributeArray(program->get_vertex_location(), 2, GL_FLOAT, vertices.get_data()),
		gles2::AttributeArray(program->get_texcoord_location(), 2, GL_FLOAT, texture.texcoord.get_data())
	);
}
void nitro::TextureNode::set_texture(const Texture& texture) {
	this->texture = texture;
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
	Texture mask = Texture::create_from_file(file_name, width, height);
	ColorMaskNode node;
	node.set_size(width, height);
	node.set_mask(mask);
	return node;
}
void nitro::ColorMaskNode::draw(const DrawContext& draw_context) {
	if (get_width() <= 0.f || get_height() <= 0.f || mask.texture == nullptr) {
		return;
	}

	ColorMaskProgram* program = ColorMaskProgram::get();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		gles2::TextureState(mask.texture.get(), GL_TEXTURE0, program->get_mask_location()),
		gles2::UniformMat4(program->get_projection_location(), draw_context.projection),
		gles2::AttributeVec4(program->get_color_location(), _color.unpremultiply()),
		gles2::AttributeArray(program->get_vertex_location(), 2, GL_FLOAT, vertices.get_data()),
		gles2::AttributeArray(program->get_mask_texcoord_location(), 2, GL_FLOAT, mask.texcoord.get_data())
	);
}
const nitro::Color& nitro::ColorMaskNode::get_color() const {
	return _color;
}
void nitro::ColorMaskNode::set_color(const Color& color) {
	if (color == _color) {
		return;
	}
	_color = color;
	request_redraw();
}
nitro::Property<nitro::Color> nitro::ColorMaskNode::color() {
	return Property<Color> {this, [](ColorMaskNode* mask) {
		return mask->get_color();
	}, [](ColorMaskNode* mask, Color color) {
		mask->set_color(color);
	}};
}
void nitro::ColorMaskNode::set_mask(const Texture& mask) {
	this->mask = mask;
}

// TextureMaskNode
nitro::TextureMaskNode::TextureMaskNode(): _alpha(1.f) {

}
void nitro::TextureMaskNode::draw(const DrawContext& draw_context) {
	if (get_width() <= 0.f || get_height() <= 0.f || texture.texture == nullptr || mask.texture == nullptr) {
		return;
	}

	TextureMaskProgram* program = TextureMaskProgram::get();

	Quad vertices (0.f, 0.f, get_width(), get_height());

	gles2::draw(
		program,
		GL_TRIANGLE_STRIP,
		4,
		gles2::TextureState(texture.texture.get(), GL_TEXTURE0, program->get_texture_location()),
		gles2::TextureState(mask.texture.get(), GL_TEXTURE1, program->get_mask_location()),
		gles2::UniformMat4(program->get_projection_location(), draw_context.projection),
		gles2::UniformFloat(program->get_alpha_location(), _alpha),
		gles2::AttributeArray(program->get_vertex_location(), 2, GL_FLOAT, vertices.get_data()),
		gles2::AttributeArray(program->get_texcoord_location(), 2, GL_FLOAT, texture.texcoord.get_data()),
		gles2::AttributeArray(program->get_mask_texcoord_location(), 2, GL_FLOAT, mask.texcoord.get_data())
	);
}
void nitro::TextureMaskNode::set_texture(const Texture& texture) {
	this->texture = texture;
}
void nitro::TextureMaskNode::set_mask(const Texture& mask) {
	this->mask = mask;
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

// Clip
nitro::Clip::Clip() {

}
void nitro::Clip::prepare_draw() {
	if (Node* child = get_child(0)) {
		child->prepare_draw();
		fbo->use();
		child->draw(DrawContext(gles2::project(get_width(), get_height())));
	}
}
void nitro::Clip::draw(const DrawContext& draw_context) {
	image.draw(draw_context);
}
void nitro::Clip::layout() {
	Bin::layout();
	fbo = std::make_shared<gles2::FramebufferObject>(get_width(), get_height());
	image.set_texture(Texture(fbo->get_texture(), Quad(0.f, 0.f, 1.f, 1.f)));
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
