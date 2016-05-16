#include "atmosphere.hpp"
#include <stb_image.h>

using namespace GLES2;

// Node
atmosphere::Node::Node(): x(0), y(0), clipping(false) {

}
void atmosphere::Node::add_child(Node* node) {
	children.push_back(node);
}
void atmosphere::Node::draw(const DrawContext& parent_draw_context) {
	DrawContext draw_context;
	draw_context.projection = parent_draw_context.projection * translate(x, y);
	if (clipping)
		draw_context.clipping = scale(1.f/width, 1.f/height);
	else
		draw_context.clipping = parent_draw_context.clipping * translate(x, y);
	for (Node* node: children) {
		node->draw(draw_context);
	}
}
void atmosphere::Node::set_position(float x, float y) {
	this->x = x;
	this->y = y;
}

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const vec4& color): color(color) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Rectangle::draw(const DrawContext& draw_context) {
	static Program program{"shaders/vertex.glsl", "shaders/fragment.glsl"};

	GLfloat vertices[] = {
		x, y,
		x+width, y,
		x+width, y+height,
		x, y+height
	};

	program.use();
	program.set_uniform("projection", draw_context.projection);
	program.set_uniform("clipping", draw_context.clipping);
	GLint vertex_location = program.get_attribute_location("vertex");
	glVertexAttribPointer(vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	program.set_attribute("color", color);

	glEnableVertexAttribArray(vertex_location);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(vertex_location);

	Node::draw(draw_context);
}

// Image
atmosphere::Image::Image(const char* file_name, float x, float y): texcoord{0.f, 1.f, 1.f, 0.f} {
	int width, height, depth;
	unsigned char* data = stbi_load(file_name, &width, &height, &depth, 0);
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	texture = new Texture(width, height, depth, data);
	stbi_image_free(data);
}
void atmosphere::Image::draw(const DrawContext& draw_context) {
	static Program texture_program{"shaders/vertex-texture.glsl", "shaders/fragment-texture.glsl"};

	GLfloat vertices[] = {
		x, y,
		x + width, y,
		x + width, y + height,
		x, y + height
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
	GLint vertex_location = texture_program.get_attribute_location("vertex");
	glVertexAttribPointer(vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	GLint texcoord_location = texture_program.get_attribute_location("texcoord");
	glVertexAttribPointer(texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

	glEnableVertexAttribArray(vertex_location);
	glEnableVertexAttribArray(texcoord_location);
	texture->bind();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	texture->unbind();
	glDisableVertexAttribArray(texcoord_location);
	glDisableVertexAttribArray(vertex_location);

	Node::draw(draw_context);
}

// Mask
atmosphere::Mask::Mask(float x, float y, float width, float height, const vec4& color, Texture* mask, const Texcoord& texcoord): color(color), mask(mask), mask_texcoord{texcoord} {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Mask::draw(const DrawContext& draw_context) {
	static Program mask_program{"shaders/vertex-mask.glsl", "shaders/fragment-mask.glsl"};

	GLfloat vertices[] = {
		x, y,
		x + width, y,
		x + width, y + height,
		x, y + height
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
	GLint vertex_location = mask_program.get_attribute_location("vertex");
	glVertexAttribPointer(vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	mask_program.set_attribute("color", color);
	GLint texcoord_location = mask_program.get_attribute_location("texcoord");
	glVertexAttribPointer(texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

	glEnableVertexAttribArray(vertex_location);
	glEnableVertexAttribArray(texcoord_location);
	mask->bind();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	mask->unbind();
	glDisableVertexAttribArray(texcoord_location);
	glDisableVertexAttribArray(vertex_location);

	Node::draw(draw_context);
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
atmosphere::RoundedRectangle::RoundedRectangle(float x, float y, float width, float height, const vec4& color, float radius) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	Texture* texture = create_texture(radius);

	add_child(new Mask{0.f, 0.f, radius, radius, color, texture, {1.f, 1.f, 0.f, 0.f}});
	add_child(new Mask{width-radius, 0.f, radius, radius, color, texture, {0.f, 1.f, 1.f, 0.f}});
	add_child(new Mask{0.f, height-radius, radius, radius, color, texture, {1.f, 0.f, 0.f, 1.f}});
	add_child(new Mask{width-radius, height-radius, radius, radius, color, texture, {0.f, 0.f, 1.f, 1.f}});

	add_child(new Rectangle{radius, 0.f, width-2.f*radius, radius, color});
	add_child(new Rectangle{0.f, radius, width, height-2.f*radius, color});
	add_child(new Rectangle{radius, height-radius, width-2.f*radius, radius, color});
}
