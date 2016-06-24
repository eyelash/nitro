#include "atmosphere.hpp"
#include <stb_image.h>

using namespace GLES2;

// Transformation
atmosphere::Transformation::Transformation(): x(0.f), y(0.f), scale(1.f), rotation_x(0.f), rotation_y(0.f), rotation_z(0.f) {

}
mat4 atmosphere::Transformation::get_matrix(float width, float height) const {
	return translate(width/2.f+x, height/2.f+y) * GLES2::scale(scale, scale) * rotateX(rotation_x) * rotateY(rotation_y) * rotateZ(rotation_z) * translate(-width/2.f, -height/2.f);
}
mat4 atmosphere::Transformation::get_inverse_matrix(float width, float height) const {
	return translate(width/2.f, height/2.f) * rotateZ(-rotation_z) * GLES2::scale(1.f/scale, 1.f/scale) * translate(-width/2.f-x, -height/2.f-y);
}

// Node
atmosphere::Node::Node(): clipping(false), mouse_inside(false) {

}
void atmosphere::Node::add_child(Node* node) {
	children.push_back(node);
}
void atmosphere::Node::draw(const DrawContext& parent_draw_context) {
	DrawContext draw_context;
	draw_context.projection = parent_draw_context.projection * transformation.get_matrix(width, height);
	if (clipping)
		draw_context.clipping = scale(1.f/width, 1.f/height);
	else
		draw_context.clipping = parent_draw_context.clipping * transformation.get_matrix(width, height);
	draw_node(draw_context);
	for (Node* node: children) {
		node->draw(draw_context);
	}
}
void atmosphere::Node::draw_node(const DrawContext& draw_context) {

}
void atmosphere::Node::handle_mouse_motion_event(const vec4& parent_position) {
	vec4 p = transformation.get_inverse_matrix(width, height) * parent_position;
	if (p.x >= 0.f && p.x < width && p.y >= 0.f && p.y < height) {
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
	for (Node* node: children) {
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
atmosphere::Property<float> atmosphere::Node::rotation_z() {
	return Property<float> {this, [](Node* node) {
		return node->transformation.rotation_z;
	}, [](Node* node, float value) {
		node->transformation.rotation_z = value;
	}};
}

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const vec4& color): color(color) {
	transformation.x = x;
	transformation.y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Rectangle::draw_node(const DrawContext& draw_context) {
	static Program program{"shaders/vertex.glsl", "shaders/fragment.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width, 0.f,
		width, height,
		0.f, height
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
}

// Image
atmosphere::Image::Image(const char* file_name, float x, float y): texcoord{0.f, 1.f, 1.f, 0.f} {
	int width, height, depth;
	unsigned char* data = stbi_load(file_name, &width, &height, &depth, 0);
	transformation.x = x;
	transformation.y = y;
	this->width = width;
	this->height = height;
	texture = new Texture(width, height, depth, data);
	stbi_image_free(data);
}
void atmosphere::Image::draw_node(const DrawContext& draw_context) {
	static Program texture_program{"shaders/vertex-texture.glsl", "shaders/fragment-texture.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width, 0.f,
		width, height,
		0.f, height
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
}

// Mask
atmosphere::Mask::Mask(float x, float y, float width, float height, const vec4& color, Texture* mask, const Texcoord& texcoord): color(color), mask(mask), mask_texcoord{texcoord} {
	transformation.x = x;
	transformation.y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Mask::draw_node(const DrawContext& draw_context) {
	static Program mask_program{"shaders/vertex-mask.glsl", "shaders/fragment-mask.glsl"};

	GLfloat vertices[] = {
		0.f, 0.f,
		width, 0.f,
		width, height,
		0.f, height
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
	transformation.x = x;
	transformation.y = y;
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
