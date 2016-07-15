#include "gles2.hpp"
#include "animation.hpp"
#include <vector>

namespace atmosphere {

class Color {
	float r, g, b, a;
public:
	Color() {

	}
	constexpr Color(float r, float g, float b, float a = 1.f): r(r*a), g(g*a), b(b*a), a(a) {

	}
	Color with_alpha(float alpha) const {
		return {r, g, b, a*alpha};
	}
	GLES2::vec4 unpremultiply() const {
		return a == 0.f ? GLES2::vec4 {0.f, 0.f, 0.f, 0.f} : GLES2::vec4 {r/a, g/a, b/a, a};
	}
	Color operator+(const Color& c) const {
		return Color{r+c.r, g+c.g, b+c.b, a+c.a};
	}
	Color operator*(float x) const {
		return Color{r*x, g*x, b*x, a*x};
	}
};

class Transformation {
public:
	float x, y;
	float scale;
	float rotation_x, rotation_y, rotation_z;
	Transformation();
	Transformation(float x, float y);
	GLES2::mat4 get_matrix(float width, float height) const;
	GLES2::mat4 get_inverse_matrix(float width, float height) const;
};

struct DrawContext {
	GLES2::mat4 projection;
	GLES2::mat4 clipping;
	float alpha;
};

class Node {
	Transformation transformation;
	float _width, _height;
	float _alpha;
public:
	bool clipping;
	bool mouse_inside;
	Node();
	Node(float x, float y, float width, float height);
	virtual Node* get_child(int index);
	void draw(const DrawContext& parent_draw_context);
	virtual void draw_node(const DrawContext& draw_context);
	virtual void layout(float width, float height);
	void handle_mouse_motion_event(const GLES2::vec4& parent_position);
	virtual void mouse_enter();
	virtual void mouse_leave();
	Property<float> position_x();
	Property<float> position_y();
	Property<float> width();
	Property<float> height();
	Property<float> alpha();
	Property<float> rotation_z();
};

class SimpleContainer: public Node {
	std::vector<Node*> children;
public:
	SimpleContainer(float x, float y, float width, float height);
	Node* get_child(int index) override;
	void add_child(Node* node);
};

class TextureAtlas {
	//Texture texture;
	//bool used[1024];
public:
	//TextureAtlas(): Texture(512, 512, 4, nullptr) {}
};

class Rectangle: public SimpleContainer {
	Color _color;
public:
	Rectangle(float x, float y, float width, float height, const Color& color);
	void draw_node(const DrawContext& draw_context) override;
	Property<Color> color();
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
	Color _color;
	GLES2::Texture* mask;
	Texcoord mask_texcoord;
public:
	Mask(float x, float y, float width, float height, const Color& color, GLES2::Texture* texture, const Texcoord& texcoord);
	void draw_node(const DrawContext& draw_context) override;
	Property<Color> color();
};

class RoundedRectangle: public SimpleContainer {
	static GLES2::Texture* create_texture(int radius);
	float radius;
	Mask* bottom_left;
	Rectangle* bottom;
	Mask* bottom_right;
	Rectangle* center;
	Mask* top_left;
	Rectangle* top;
	Mask* top_right;
public:
	RoundedRectangle(float x, float y, float width, float height, const Color& color, float radius);
	Node* get_child(int index) override;
	void layout(float width, float height) override;
	Property<Color> color();
};

class Text: public Node {
	SimpleContainer glyphs;
public:
	Text(const char* text, const Color& color);
	Node* get_child(int index) override;
	void layout(float width, float height) override;
};

class Window {
	SimpleContainer root_node;
	DrawContext draw_context;
	void dispatch_events();
public:
	Window(int width, int height, const char* title);
	void add_child(Node* node);
	void run();
};

}
