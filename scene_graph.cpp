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
atmosphere::Transformation::Transformation(): x(0.f), y(0.f), scale(1.f), rotation_x(0.f), rotation_y(0.f), rotation_z(0.f) {

}
atmosphere::Transformation::Transformation(float x, float y): x(x), y(y), scale(1.f), rotation_x(0.f), rotation_y(0.f), rotation_z(0.f) {

}
mat4 atmosphere::Transformation::get_matrix(float width, float height) const {
	return translate(width/2.f+x, height/2.f+y) * GLES2::scale(scale, scale) * rotateX(rotation_x) * rotateY(rotation_y) * rotateZ(rotation_z) * translate(-width/2.f, -height/2.f);
}
mat4 atmosphere::Transformation::get_inverse_matrix(float width, float height) const {
	return translate(width/2.f, height/2.f) * rotateZ(-rotation_z) * GLES2::scale(1.f/scale, 1.f/scale) * translate(-width/2.f-x, -height/2.f-y);
}

// Node
atmosphere::Node::Node(): _alpha(1.f), clipping(false), mouse_inside(false) {

}
atmosphere::Node::Node(float x, float y, float width, float height): transformation(x, y), _width(width), _height(height), _alpha(1.f), clipping(false), mouse_inside(false) {

}
atmosphere::Node* atmosphere::Node::get_child(int index) {
	return nullptr;
}
void atmosphere::Node::draw(const DrawContext& parent_draw_context) {
	DrawContext draw_context;
	draw_context.projection = parent_draw_context.projection * transformation.get_matrix(_width, _height);
	if (clipping)
		draw_context.clipping = scale(1.f/_width, 1.f/_height);
	else
		draw_context.clipping = parent_draw_context.clipping * transformation.get_matrix(_width, _height);
	draw_context.alpha = parent_draw_context.alpha * _alpha;
	draw_node(draw_context);
	for (int i = 0; Node* node = get_child(i); ++i) {
		node->draw(draw_context);
	}
}
void atmosphere::Node::draw_node(const DrawContext& draw_context) {

}
void atmosphere::Node::layout(float width, float height) {

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
		node->_width = value;
		node->layout(node->_width, node->_height);
	}};
}
atmosphere::Property<float> atmosphere::Node::height() {
	return Property<float> {this, [](Node* node) {
		return node->_height;
	}, [](Node* node, float value) {
		node->_height = value;
		node->layout(node->_width, node->_height);
	}};
}
atmosphere::Property<float> atmosphere::Node::alpha() {
	return Property<float> {this, [](Node* node) {
		return node->_alpha;
	}, [](Node* node, float value) {
		node->_alpha = value;
	}};
}
atmosphere::Property<float> atmosphere::Node::rotation_z() {
	return Property<float> {this, [](Node* node) {
		return node->transformation.rotation_z;
	}, [](Node* node, float value) {
		node->transformation.rotation_z = value;
	}};
}

// SimpleContainer
atmosphere::SimpleContainer::SimpleContainer(float x, float y, float width, float height): Node(x, y, width, height) {

}
atmosphere::Node* atmosphere::SimpleContainer::get_child(int index) {
	return index < children.size() ? children[index] : nullptr;
}
void atmosphere::SimpleContainer::add_child(Node* node) {
	children.push_back(node);
}

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const Color& color): SimpleContainer(x, y, width, height), _color(color) {

}
void atmosphere::Rectangle::draw_node(const DrawContext& draw_context) {
	static Program program{"shaders/vertex.glsl", "shaders/fragment.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		width().get(), height().get(),
		0.f, height().get()
	};

	program.use();
	program.set_uniform("projection", draw_context.projection);
	program.set_uniform("clipping", draw_context.clipping);
	program.set_attribute("color", _color.with_alpha(draw_context.alpha).unpremultiply());
	VertexAttributeArray vertex = program.set_attribute_array("vertex", 2, vertices);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
atmosphere::Property<atmosphere::Color> atmosphere::Rectangle::color() {
	return Property<Color> {this, [](Rectangle* rectangle) {
		return rectangle->_color;
	}, [](Rectangle* rectangle, Color color) {
		rectangle->_color = color;
	}};
}

// Image
atmosphere::Image::Image(const char* file_name, float x, float y): Node(x, y, 0.f, 0.f), texcoord{0.f, 1.f, 1.f, 0.f} {
	int width, height, depth;
	if (!strcmp(file_name+strlen(file_name)-4, ".svg")) {
		NSVGimage* svg_image = nsvgParseFromFile(file_name, "px", 96);
		width = svg_image->width;
		height = svg_image->height;
		printf("svg dimensions: %i x %i\n", width, height);
		unsigned char* data = (unsigned char*) malloc(width*height*4);
		NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
		nsvgRasterize(rasterizer, svg_image, 0.f, 0.f, 1.f, data, width, height, width*4);
		nsvgDeleteRasterizer(rasterizer);
		nsvgDelete(svg_image);
		texture = new Texture(width, height, 4, data);
		free(data);
	}
	else {
		unsigned char* data = stbi_load(file_name, &width, &height, &depth, 0);
		texture = new Texture(width, height, depth, data);
		stbi_image_free(data);
	}
	this->width().set(width);
	this->height().set(height);
}
void atmosphere::Image::draw_node(const DrawContext& draw_context) {
	static Program texture_program{"shaders/vertex-texture.glsl", "shaders/fragment-texture.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		width().get(), height().get(),
		0.f, height().get()
	};
	GLfloat texcoords[] = {
		texcoord.x0, texcoord.y0,
		texcoord.x1, texcoord.y0,
		texcoord.x1, texcoord.y1,
		texcoord.x0, texcoord.y1
	};

	texture_program.use();
	texture_program.set_uniform("projection", draw_context.projection);
	texture_program.set_uniform("clipping", draw_context.clipping);
	texture_program.set_uniform("texture", 0);
	VertexAttributeArray vertex = texture_program.set_attribute_array("vertex", 2, vertices);
	VertexAttributeArray texcoord = texture_program.set_attribute_array("texcoord", 2, texcoords);

	texture->bind();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	texture->unbind();
}

// Mask
atmosphere::Mask::Mask(float x, float y, float width, float height, const Color& color, Texture* mask, const Texcoord& texcoord): Node(x, y, width, height), _color(color), mask(mask), mask_texcoord{texcoord} {

}
void atmosphere::Mask::draw_node(const DrawContext& draw_context) {
	static Program mask_program{"shaders/vertex-mask.glsl", "shaders/fragment-mask.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width().get(), 0.f,
		width().get(), height().get(),
		0.f, height().get()
	};
	GLfloat texcoords[] = {
		mask_texcoord.x0, mask_texcoord.y0,
		mask_texcoord.x1, mask_texcoord.y0,
		mask_texcoord.x1, mask_texcoord.y1,
		mask_texcoord.x0, mask_texcoord.y1
	};

	mask_program.use();
	mask_program.set_uniform("projection", draw_context.projection);
	mask_program.set_uniform("clipping", draw_context.clipping);
	mask_program.set_uniform("texture", 0);
	mask_program.set_attribute("color", _color.with_alpha(draw_context.alpha).unpremultiply());
	VertexAttributeArray vertex = mask_program.set_attribute_array("vertex", 2, vertices);
	VertexAttributeArray texcoord = mask_program.set_attribute_array("texcoord", 2, texcoords);

	mask->bind();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	mask->unbind();
}
atmosphere::Property<atmosphere::Color> atmosphere::Mask::color() {
	return Property<Color> {this, [](Mask* mask) {
		return mask->_color;
	}, [](Mask* mask, Color color) {
		mask->_color = color;
	}};
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
Texture* atmosphere::RoundedRectangle::create_texture(int radius) {
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
atmosphere::RoundedRectangle::RoundedRectangle(float x, float y, float width, float height, const Color& color, float radius): SimpleContainer(x, y, width, height), radius(radius) {
	Texture* texture = create_texture(radius);

	bottom_left = new Mask{0.f, 0.f, radius, radius, color, texture, {1.f, 1.f, 0.f, 0.f}};
	bottom_right = new Mask{width-radius, 0.f, radius, radius, color, texture, {0.f, 1.f, 1.f, 0.f}};
	top_left = new Mask{0.f, height-radius, radius, radius, color, texture, {1.f, 0.f, 0.f, 1.f}};
	top_right = new Mask{width-radius, height-radius, radius, radius, color, texture, {0.f, 0.f, 1.f, 1.f}};

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
		default: return SimpleContainer::get_child(index-7);
	}
}
void atmosphere::RoundedRectangle::layout(float width, float height) {
	bottom->width().set(width - 2.f * radius);
	bottom_right->position_x().set(width - radius);
	center->width().set(width);
	center->height().set(height - 2.f * radius);
	top_left->position_y().set(height - radius);
	top->position_y().set(height - radius);
	top->width().set(width-2.f*radius);
	top_right->position_x().set(width - radius);
	top_right->position_y().set(height - radius);
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
