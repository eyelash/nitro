#include "atmosphere.hpp"
using namespace GLES2;

// Rectangle
atmosphere::Rectangle::Rectangle(float x, float y, float width, float height): x(x), y(y), width(width), height(height) {

}
void atmosphere::Rectangle::draw() {
	static Program* program = nullptr;
	if (!program) program = new Program ("shaders/vertex.glsl", "shaders/fragment.glsl");

	GLfloat vertices[] = {
		x, y,
		x+width, y,
		x+width, y+height,
		x, y+height
	};

	program->use ();
	program->set_uniform ("projection", glOrtho (0, 800, 0, 600, -1, 1));
	GLint vertex_location = program->get_attribute_location ("vertex");
	glVertexAttribPointer (vertex_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray (vertex_location);
	program->set_attribute ("color", vec4 {0, 1, 0, 1});

	glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray (vertex_location);
}

// SceneGraph
void atmosphere::SceneGraph::add_node(Node* node) {
	nodes.insert(node);
}
void atmosphere::SceneGraph::draw() {
	for (Node* node: nodes)
		node->draw();
}
