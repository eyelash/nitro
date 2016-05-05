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
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const GLES2::vec4& color): color(color) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Rectangle::draw(const DrawContext& draw_context) {
	static Program* program = nullptr;
	if (!program) program = new Program ("shaders/vertex.glsl", "shaders/fragment.glsl");

	GLfloat vertices[] = {
		x, y,
		x+width, y,
		x+width, y+height,
		x, y+height
	};

	program->use ();
	program->set_uniform ("projection", draw_context.projection);
	program->set_uniform ("clipping", draw_context.clipping);
	GLint vertex_location = program->get_attribute_location ("vertex");
	glVertexAttribPointer (vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray (vertex_location);
	program->set_attribute ("color", color);

	glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray (vertex_location);

	Node::draw(draw_context);
}

// Image
atmosphere::Image::Image(const char* file_name, float x, float y) {
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
	static Program* texture_program = nullptr;
	if (!texture_program) texture_program = new Program ("shaders/vertex-texture.glsl", "shaders/fragment-texture.glsl");

	GLfloat vertices[] = {
		x, y,
		x + width, y,
		x + width, y + height,
		x, y + height
	};
	GLfloat texcoords[] = {
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	texture_program->use ();
	texture_program->set_uniform ("projection", draw_context.projection);
	texture_program->set_uniform ("clipping", draw_context.clipping);
	//texture_program->set_uniform ("texture", image->identifier);
	GLint vertex_location = texture_program->get_attribute_location ("vertex");
	glVertexAttribPointer (vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	GLint texcoord_location = texture_program->get_attribute_location ("texcoord");
	glVertexAttribPointer (texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

	glEnableVertexAttribArray (vertex_location);
	glEnableVertexAttribArray (texcoord_location);
	glEnable (GL_TEXTURE_2D);
	texture->bind ();

	glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

	texture->unbind ();
	glDisable (GL_TEXTURE_2D);
	glDisableVertexAttribArray (texcoord_location);
	glDisableVertexAttribArray (vertex_location);

	Node::draw(draw_context);
}
