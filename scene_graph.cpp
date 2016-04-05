#include "atmosphere.hpp"
#include <stb_image.h>

using namespace GLES2;

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height, const GLES2::vec4& color): color(color) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}
void atmosphere::Rectangle::draw(const GLES2::mat4& projection) {
	static Program* program = nullptr;
	if (!program) program = new Program ("shaders/vertex.glsl", "shaders/fragment.glsl");

	GLfloat vertices[] = {
		x, y,
		x+width, y,
		x+width, y+height,
		x, y+height
	};

	program->use ();
	program->set_uniform ("projection", projection);
	GLint vertex_location = program->get_attribute_location ("vertex");
	glVertexAttribPointer (vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray (vertex_location);
	program->set_attribute ("color", color);

	glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray (vertex_location);
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
void atmosphere::Image::draw(const GLES2::mat4& projection) {
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
	texture_program->set_uniform ("projection", projection);
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
}

// SceneGraph
atmosphere::SceneGraph::SceneGraph(): projection(glOrtho (0, 800, 0, 600, -1, 1)) {

}
void atmosphere::SceneGraph::add_node(Node* node) {
	nodes.insert(node);
}
void atmosphere::SceneGraph::draw() {
	for (Node* node: nodes)
		node->draw(projection);
}
void atmosphere::SceneGraph::set_size(int width, int height) {
	projection = glOrtho (0, width, 0, height, -1, 1);
}
