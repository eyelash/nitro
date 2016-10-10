/*

Copyright (c) 2016, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "atmosphere.hpp"
#include <stb_image.h>
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <cstring>

using namespace GLES2;

// Transformation
atmosphere::Transformation::Transformation(float x, float y): x{x}, y{y}, scale_x{1.f}, scale_y{1.f}, rotation_x{0.f}, rotation_y{0.f}, rotation_z{0.f} {

}
mat4 atmosphere::Transformation::get_matrix(float width, float height) const {
	return translate(width/2.f+x, height/2.f+y) * GLES2::scale(scale_x, scale_y) * rotateX(rotation_x) * rotateY(rotation_y) * rotateZ(rotation_z) * translate(-width/2.f, -height/2.f);
}
mat4 atmosphere::Transformation::get_inverse_matrix(float width, float height) const {
	return translate(width/2.f, height/2.f) * rotateZ(-rotation_z) * GLES2::scale(1.f/scale_x, 1.f/scale_y) * translate(-width/2.f-x, -height/2.f-y);
}

// Node
atmosphere::Node::Node(float x, float y, float width, float height): transformation{x, y}, _width{width}, _height{height}, mouse_inside{false} {

}
atmosphere::Node* atmosphere::Node::get_child(int index) {
	return nullptr;
}
void atmosphere::Node::prepare_draw() {
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->prepare_draw();
	}
}
void atmosphere::Node::draw(const DrawContext& draw_context) {
	for (int i = 0; Node* node = get_child(i); ++i) {
		DrawContext child_draw_context;
		child_draw_context.projection = draw_context.projection * node->transformation.get_matrix(node->_width, node->_height);
		node->draw(child_draw_context);
	}
}
void atmosphere::Node::layout() {

}
void atmosphere::Node::handle_mouse_motion_event(const vec4& parent_position) {
	vec4 p = transformation.get_inverse_matrix(_width, _height) * parent_position;
	if (p.x >= 0.f && p.x < _width && p.y >= 0.f && p.y < _height) {
		if (!mouse_inside) {
			mouse_enter();
			mouse_inside = true;
		}
	}
	else {
		if (mouse_inside) {
			mouse_leave();
			mouse_inside = false;
		}
	}
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->handle_mouse_motion_event(p);
	}
}
void atmosphere::Node::mouse_enter() {

}
void atmosphere::Node::mouse_leave() {

}
atmosphere::Property<float> atmosphere::Node::position_x() {
	return Property<float> {this, [](Node* node) {
		return node->transformation.x;
	}, [](Node* node, float value) {
		node->transformation.x = value;
	}};
}
atmosphere::Property<float> atmosphere::Node::position_y() {
	return Property<float> {this, [](Node* node) {
		return node->transformation.y;
	}, [](Node* node, float value) {
		node->transformation.y = value;
	}};
}
atmosphere::Property<float> atmosphere::Node::width() {
	return Property<float> {this, [](Node* node) {
		return node->_width;
	}, [](Node* node, float value) {
		if (node->_width == value) return;
		node->_width = value;
		node->layout();
	}};
}
atmosphere::Property<float> atmosphere::Node::height() {
	return Property<float> {this, [](Node* node) {
		return node->_height;
	}, [](Node* node, float value) {
		if (node->_height == value) return;
		node->_height = value;
		node->layout();
	}};
}
atmosphere::Property<float> atmosphere::Node::rotation_z() {
	return Property<float> {this, [](Node* node) {
		return node->transformation.rotation_z;
	}, [](Node* node, float value) {
		node->transformation.rotation_z = value;
	}};
}

// Bin
atmosphere::Bin::Bin(float x, float y, float width, float height, float padding): Node{x, y, width, height}, child{nullptr}, padding{padding} {

}
atmosphere::Node* atmosphere::Bin::get_child(int index) {
	return index == 0 ? child : nullptr;
}
void atmosphere::Bin::layout() {
	if (child) {
		child->position_x().set(padding);
		child->position_y().set(padding);
		child->width().set(width().get() - 2.f * padding);
		child->height().set(height().get() - 2.f * padding);
	}
}
void atmosphere::Bin::set_child(Node* node) {
	child = node;
	layout();
}
void atmosphere::Bin::set_padding(float padding) {
	this->padding = padding;
	layout();
}

// SimpleContainer
atmosphere::SimpleContainer::SimpleContainer(float x, float y, float width, float height): Node{x, y, width, height} {

}
atmosphere::Node* atmosphere::SimpleContainer::get_child(int index) {
	return index < children.size() ? children[index] : nullptr;
}
void atmosphere::SimpleContainer::add_child(Node* node) {
	children.push_back(node);
}

static Program* get_color_program() {
	static Program program {"shaders/color.vs.glsl", "shaders/color.fs.glsl"};
	return &program;
}
static Program* get_texture_program() {
	static Program program {"shaders/texture.vs.glsl", "shaders/texture.fs.glsl"};
	return &program;
}

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const Color& color): Bin{x, y, width, height}, _color{color} {

}
void atmosphere::Rectangle::draw(const DrawContext& draw_context) {
	Program* program = get_color_program();

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		0.f, height().get(),
		width().get(), height().get()
	};

	program->use();
	{
		program->set_uniform("projection", draw_context.projection);
		program->set_attribute("color", _color.get());
		VertexAttributeArray attr_vertex = program->set_attribute_array("vertex", 2, vertices);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	Bin::draw(draw_context);
}
atmosphere::Property<atmosphere::Color> atmosphere::Rectangle::color() {
	return Property<Color> {this, [](Rectangle* rectangle) {
		return rectangle->_color;
	}, [](Rectangle* rectangle, Color color) {
		rectangle->_color = color;
	}};
}

// Gradient
atmosphere::Gradient::Gradient(float x, float y, float width, float height, const Color& top_color, const Color& bottom_color): Bin{x, y, width, height}, _top_color{top_color}, _bottom_color{bottom_color} {

}
void atmosphere::Gradient::draw(const DrawContext& draw_context) {
	Program* program = get_color_program();

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		0.f, height().get(),
		width().get(), height().get()
	};
	vec4 bottom = _bottom_color.get();
	vec4 top = _top_color.get();
	GLfloat vertex_color[] = {
		bottom.x, bottom.y, bottom.z, bottom.w,
		bottom.x, bottom.y, bottom.z, bottom.w,
		top.x, top.y, top.z, top.w,
		top.x, top.y, top.z, top.w
	};

	program->use();
	{
		program->set_uniform("projection", draw_context.projection);
		VertexAttributeArray vertex = program->set_attribute_array("vertex", 2, vertices);
		VertexAttributeArray color = program->set_attribute_array("color", 4, vertex_color);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	Bin::draw(draw_context);
}
atmosphere::Property<atmosphere::Color> atmosphere::Gradient::top_color() {
	return Property<Color> {this, [](Gradient* gradient) {
		return gradient->_top_color;
	}, [](Gradient* gradient, Color color) {
		gradient->_top_color = color;
	}};
}
atmosphere::Property<atmosphere::Color> atmosphere::Gradient::bottom_color() {
	return Property<Color> {this, [](Gradient* gradient) {
		return gradient->_bottom_color;
	}, [](Gradient* gradient, Color color) {
		gradient->_bottom_color = color;
	}};
}

// Image
static Texture* create_texture_from_file(const char* file_name, int& width, int& height) {
	Texture* texture;
	if (!strcmp(file_name+strlen(file_name)-4, ".svg")) {
		NSVGimage* svg_image = nsvgParseFromFile(file_name, "px", 96);
		width = svg_image->width;
		height = svg_image->height;
		unsigned char* data = (unsigned char*) malloc(width * height * 4);
		NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
		nsvgRasterize(rasterizer, svg_image, 0.f, 0.f, 1.f, data, width, height, width * 4);
		nsvgDeleteRasterizer(rasterizer);
		nsvgDelete(svg_image);
		texture = new Texture(width, height, 4, data);
		free(data);
	}
	else {
		int depth;
		unsigned char* data = stbi_load(file_name, &width, &height, &depth, 0);
		texture = new Texture(width, height, depth, data);
		stbi_image_free(data);
	}
	return texture;
}
atmosphere::Image::Image(float x, float y, float width, float height, Texture* texture, const Texcoord& texcoord): Node{x, y, width, height}, texture{texture}, texcoord{texcoord}, _alpha{1.f} {

}
atmosphere::Image atmosphere::Image::create_from_file(const char* file_name, float x, float y) {
	int width, height;
	Texture* texture = create_texture_from_file(file_name, width, height);
	return Image{x, y, (float)width, (float)height, texture, Texcoord::create(0.f, 1.f, 1.f, 0.f)};
}
void atmosphere::Image::draw(const DrawContext& draw_context) {
	static Program texture_program{"shaders/texture.vs.glsl", "shaders/texture.fs.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		0.f, height().get(),
		width().get(), height().get()
	};

	texture_program.use();
	texture_program.set_uniform("projection", draw_context.projection);
	texture_program.set_uniform("texture", 0);
	texture_program.set_uniform("alpha", _alpha);
	VertexAttributeArray attr_vertex = texture_program.set_attribute_array("vertex", 2, vertices);
	VertexAttributeArray attr_texcoord = texture_program.set_attribute_array("texcoord", 2, texcoord.t);

	texture->bind();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	texture->unbind();
}
void atmosphere::Image::set_texture(Texture* texture) {
	this->texture = texture;
}
atmosphere::Property<float> atmosphere::Image::alpha() {
	return Property<float> {this, [](Image* image) {
		return image->_alpha;
	}, [](Image* image, float value) {
		image->_alpha = value;
	}};
}

// Mask
atmosphere::Mask::Mask(float x, float y, float width, float height, const Color& color, Texture* mask, const Texcoord& texcoord): Node{x, y, width, height}, _color{color}, mask{mask}, mask_texcoord{texcoord} {

}
atmosphere::Mask atmosphere::Mask::create_from_file(const char* file_name, const Color& color, float x, float y) {
	int width, height;
	Texture* texture = create_texture_from_file(file_name, width, height);
	return Mask{x, y, (float)width, (float)height, color, texture, Texcoord::create(0.f, 1.f, 1.f, 0.f)};
}
void atmosphere::Mask::draw(const DrawContext& draw_context) {
	static Program mask_program{"shaders/mask.vs.glsl", "shaders/mask.fs.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		0.f, height().get(),
		width().get(), height().get()
	};

	mask_program.use();
	mask_program.set_uniform("projection", draw_context.projection);
	mask_program.set_uniform("mask", 0);
	mask_program.set_attribute("color", _color.unpremultiply());
	VertexAttributeArray attr_vertex = mask_program.set_attribute_array("vertex", 2, vertices);
	VertexAttributeArray attr_texcoord = mask_program.set_attribute_array("texcoord", 2, mask_texcoord.t);

	mask->bind();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	mask->unbind();
}
atmosphere::Property<atmosphere::Color> atmosphere::Mask::color() {
	return Property<Color> {this, [](Mask* mask) {
		return mask->_color;
	}, [](Mask* mask, Color color) {
		mask->_color = color;
	}};
}

// ImageMask
atmosphere::ImageMask::ImageMask(float x, float y, float width, float height, Texture* texture, const Texcoord& texcoord, Texture* mask, const Texcoord& mask_texcoord): Node{x, y, width, height}, texture{texture}, texcoord{texcoord}, mask{mask}, mask_texcoord{mask_texcoord}, _alpha{1.f} {

}
void atmosphere::ImageMask::draw(const DrawContext& draw_context) {
	static Program program{"shaders/texture_mask.vs.glsl", "shaders/texture_mask.fs.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		0.f, height().get(),
		width().get(), height().get()
	};

	program.use();
	program.set_uniform("projection", draw_context.projection);
	program.set_uniform("texture", 0);
	program.set_uniform("mask", 1);
	program.set_uniform("alpha", _alpha);
	VertexAttributeArray attr_vertex = program.set_attribute_array("vertex", 2, vertices);
	VertexAttributeArray attr_texcoord = program.set_attribute_array("texcoord", 2, texcoord.t);
	VertexAttributeArray attr_mask_texcoord = program.set_attribute_array("mask_texcoord", 2, mask_texcoord.t);

	texture->bind(GL_TEXTURE0);
	mask->bind(GL_TEXTURE1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	mask->unbind(GL_TEXTURE1);
	texture->unbind(GL_TEXTURE0);
}
void atmosphere::ImageMask::set_texture(Texture* texture) {
	this->texture = texture;
}
atmosphere::Property<float> atmosphere::ImageMask::alpha() {
	return Property<float> {this, [](ImageMask* image_mask) {
		return image_mask->_alpha;
	}, [](ImageMask* image_mask, float value) {
		image_mask->_alpha = value;
	}};
}

// Clip
atmosphere::Clip::Clip(float x, float y, float width, float height): Bin{x, y, width, height}, fbo{new FramebufferObject{(int)width, (int)height}}, image{0.f, 0.f, width, height, fbo->texture, Texcoord::create(0.f, 0.f, 1.f, 1.f)} {

}
void atmosphere::Clip::prepare_draw() {
	if (Node* child = get_child(0)) {
		child->prepare_draw();
		fbo->use();
		DrawContext draw_context;
		draw_context.projection = GLES2::project(width().get(), height().get(), width().get()*2);
		child->draw(draw_context);
	}
}
void atmosphere::Clip::draw(const DrawContext& draw_context) {
	image.draw(draw_context);
}
void atmosphere::Clip::layout() {
	Bin::layout();
	delete fbo;
	fbo = new FramebufferObject{(int)width().get(), (int)height().get()};
	image.set_texture(fbo->texture);
	image.width().set(width().get());
	image.height().set(height().get());
}
atmosphere::Property<float> atmosphere::Clip::alpha() {
	return image.alpha();
}

// RoundedRectangle
static constexpr float curve(float x) {
	return sqrtf(1.f - x * x);
}
static constexpr float int_curve(float x) {
	return 0.5f * (sqrtf(1.f-x*x) * x + asinf(x));
}
static float corner(float x, float y, float w, float h) {
	const float x1 = x + w;
	const float y1 = y + h;

	if (x1*x1+y1*y1 <= 1.f) return 1.f;
	if (x*x+y*y >= 1.f) return 0.f;

	float start = x;
	float end = x1;
	float result = 0.f;
	if (curve(y1) > start) {
		start = curve(y1);
		result += (start-x) / w;
	}
	if (curve(y) < end) {
		end = curve(y);
	}
	result += (int_curve(end)-int_curve(start) - (end-start)*y) / (w*h);
	return result;
}
static Texture* create_rounded_corner_texture(int radius) {
	unsigned char* data = (unsigned char*)malloc(radius*radius);
	for (int y = 0; y < radius; ++y) {
		for (int x = 0; x < radius; ++x) {
			data[y*radius+x] = corner((float)x/radius, (float)y/radius, 1.f/radius, 1.f/radius) * 255.f + 0.5f;
		}
	}
	Texture* result = new Texture(radius, radius, 1, data);
	free(data);
	return result;
}
atmosphere::RoundedRectangle::RoundedRectangle(float x, float y, float width, float height, const Color& color, float radius): Bin{x, y, width, height}, radius{radius} {
	Texture* texture = create_rounded_corner_texture(radius);
	Texcoord texcoord = Texcoord::create(0.f, 0.f, 1.f, 1.f);
	top_right = new Mask{width-radius, height-radius, radius, radius, color, texture, texcoord};
	texcoord = texcoord.rotate();
	top_left = new Mask{0.f, height-radius, radius, radius, color, texture, texcoord};
	texcoord = texcoord.rotate();
	bottom_left = new Mask{0.f, 0.f, radius, radius, color, texture, texcoord};
	texcoord = texcoord.rotate();
	bottom_right = new Mask{width-radius, 0.f, radius, radius, color, texture, texcoord};

	bottom = new Rectangle{radius, 0.f, width-2.f*radius, radius, color};
	center = new Rectangle{0.f, radius, width, height-2.f*radius, color};
	top = new Rectangle{radius, height-radius, width-2.f*radius, radius, color};
}
atmosphere::Node* atmosphere::RoundedRectangle::get_child(int index) {
	switch (index) {
		case 0: return bottom_left;
		case 1: return bottom;
		case 2: return bottom_right;
		case 3: return center;
		case 4: return top_left;
		case 5: return top;
		case 6: return top_right;
		default: return Bin::get_child(index-7);
	}
}
void atmosphere::RoundedRectangle::layout() {
	bottom->width().set(width().get() - 2.f * radius);
	bottom_right->position_x().set(width().get() - radius);
	center->width().set(width().get());
	center->height().set(height().get() - 2.f * radius);
	top_left->position_y().set(height().get() - radius);
	top->position_y().set(height().get() - radius);
	top->width().set(width().get()-2.f*radius);
	top_right->position_x().set(width().get() - radius);
	top_right->position_y().set(height().get() - radius);
}
atmosphere::Property<atmosphere::Color> atmosphere::RoundedRectangle::color() {
	return Property<Color> {this, [](RoundedRectangle* rectangle) {
		return rectangle->center->color().get();
	}, [](RoundedRectangle* rectangle, Color color) {
		rectangle->bottom_left->color().set(color);
		rectangle->bottom->color().set(color);
		rectangle->bottom_right->color().set(color);
		rectangle->center->color().set(color);
		rectangle->top_left->color().set(color);
		rectangle->top->color().set(color);
		rectangle->top_right->color().set(color);
	}};
}

// RoundedImage
atmosphere::RoundedImage::RoundedImage(float x, float y, float width, float height, Texture* texture, float radius): Bin{x, y, width, height}, radius{radius} {
	Texture* mask = create_rounded_corner_texture(radius);
	Texcoord texcoord = Texcoord::create(0.f, 0.f, 1.f, 1.f);
	top_right = new ImageMask{width-radius, height-radius, radius, radius, texture, Texcoord::create(1-radius/width, radius/height, 1, 0), mask, texcoord};
	texcoord = texcoord.rotate();
	top_left = new ImageMask{0.f, height-radius, radius, radius, texture, Texcoord::create(0, radius/height, radius/width, 0), mask, texcoord};
	texcoord = texcoord.rotate();
	bottom_left = new ImageMask{0.f, 0.f, radius, radius, texture, Texcoord::create(0, 1, radius/width, 1-radius/height), mask, texcoord};
	texcoord = texcoord.rotate();
	bottom_right = new ImageMask{width-radius, 0.f, radius, radius, texture, Texcoord::create(1-radius/width, 1, 1, 1-radius/height), mask, texcoord};

	bottom = new Image{radius, 0.f, width-2.f*radius, radius, texture, Texcoord::create(radius/width, 1, 1-radius/width, 1-radius/height)};
	center = new Image{0.f, radius, width, height-2.f*radius, texture, Texcoord::create(0, 1-radius/height, 1, radius/height)};
	top = new Image{radius, height-radius, width-2.f*radius, radius, texture, Texcoord::create(radius/width, radius/height, 1-radius/width, 0)};
}
atmosphere::RoundedImage atmosphere::RoundedImage::create_from_file(const char* file_name, float radius, float x, float y) {
	int width, height;
	Texture* texture = create_texture_from_file(file_name, width, height);
	return RoundedImage{x, y, (float)width, (float)height, texture, radius};
}
atmosphere::Node* atmosphere::RoundedImage::get_child(int index) {
	switch (index) {
		case 0: return bottom_left;
		case 1: return bottom;
		case 2: return bottom_right;
		case 3: return center;
		case 4: return top_left;
		case 5: return top;
		case 6: return top_right;
		default: return Bin::get_child(index-7);
	}
}
void atmosphere::RoundedImage::layout() {
	bottom->width().set(width().get() - 2.f * radius);
	bottom_right->position_x().set(width().get() - radius);
	center->width().set(width().get());
	center->height().set(height().get() - 2.f * radius);
	top_left->position_y().set(height().get() - radius);
	top->position_y().set(height().get() - radius);
	top->width().set(width().get()-2.f*radius);
	top_right->position_x().set(width().get() - radius);
	top_right->position_y().set(height().get() - radius);
}
atmosphere::Property<float> atmosphere::RoundedImage::alpha() {
	return Property<float> {this, [](RoundedImage* image) {
		return image->center->alpha().get();
	}, [](RoundedImage* image, float alpha) {
		image->bottom_left->alpha().set(alpha);
		image->bottom->alpha().set(alpha);
		image->bottom_right->alpha().set(alpha);
		image->center->alpha().set(alpha);
		image->top_left->alpha().set(alpha);
		image->top->alpha().set(alpha);
		image->top_right->alpha().set(alpha);
	}};
}
