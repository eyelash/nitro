#include "gles2.hpp"
#include "animation.hpp"
#include <set>
#include <vector>

namespace atmosphere {

class Transformation {
public:
	float x, y;
	float scale;
	float rotation_x, rotation_y, rotation_z;
	Transformation();
	GLES2::mat4 get_matrix(float width, float height) const;
	GLES2::mat4 get_inverse_matrix(float width, float height) const;
};

struct DrawContext {
	GLES2::mat4 projection;
	GLES2::mat4 clipping;
};

class Node {
	std::vector<Node*> children;
public:
	Transformation transformation;
	float width, height;
	bool clipping;
	Node();
	void add_child(Node* node);
	void draw(const DrawContext& parent_draw_context);
	virtual void draw_node(const DrawContext& draw_context);
	Property<float> position_x();
	Property<float> position_y();
	Property<float> rotation_z();
};

class TextureAtlas {
	//Texture texture;
	//bool used[1024];
public:
	//TextureAtlas(): Texture(512, 512, 4, nullptr) {}
};

class Rectangle: public Node {
	GLES2::vec4 color;
public:
	Rectangle(float x, float y, float width, float height, const GLES2::vec4& color);
	void draw_node(const DrawContext& draw_context) override;
};

class Texcoord {
public:
	float x0, y0, x1, y1;
};

class Image: public Node {
	GLES2::Texture* texture;
	Texcoord texcoord;
public:
	Image(const char* file_name, float x = 0.f, float y = 0.f);
	void draw_node(const DrawContext& draw_context) override;
};

class Mask: public Node {
	GLES2::vec4 color;
	GLES2::Texture* mask;
	Texcoord mask_texcoord;
public:
	Mask(float x, float y, float width, float height, const GLES2::vec4& color, GLES2::Texture* texture, const Texcoord& texcoord);
	void draw_node(const DrawContext& draw_context) override;
};

class RoundedRectangle: public Node {
	static GLES2::Texture* create_texture(int radius);
public:
	RoundedRectangle(float x, float y, float width, float height, const GLES2::vec4& color, float radius);
};

class Text: public Node {
public:
	Text(const char* text, const GLES2::vec4& color);
};

class SceneGraph {
	std::set<Node*> nodes;
	GLES2::mat4 projection;
public:
	SceneGraph();
	void add_node(Node* node);
	void draw();
	void set_size(int width, int height);
};

class Window {
	Node root_node;
	DrawContext draw_context;
	void dispatch_events();
public:
	Window(int width, int height, const char* title);
	void add_child(Node* node);
	void run();
};

}
