#include "gles2.hpp"
#include <set>

namespace atmosphere {

class Node {
protected:
	float x, y, width, height;
public:
	virtual void draw(const GLES2::mat4& projection) = 0;
};

class TextureAtlas {
	//Texture texture;
	//bool used[1024];
public:
	//TextureAtlas(): Texture(512, 512, 4, nullptr) {}
};

class Image: public Node {
	GLES2::Texture* texture;
public:
	Image(const char* file_name, float x = 0.f, float y = 0.f);
	void draw(const GLES2::mat4& projection);
};

class Rectangle: public Node {
	GLES2::vec4 color;
public:
	Rectangle(float x, float y, float width, float height, const GLES2::vec4& color);
	void draw(const GLES2::mat4& projection);
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
	SceneGraph scene_graph;
	void dispatch_events();
public:
	Window(int width, int height, const char* title);
	void add(Node* node);
	void run();
};

}
