#include "gles2.hpp"
#include <set>

namespace atmosphere {

class Node {
public:
	virtual void draw() = 0;
};

class TextureAtlas {
	//Texture texture;
	//bool used[1024];
public:
	//TextureAtlas(): Texture(512, 512, 4, nullptr) {}
};

class Image: public Node {

};

class Rectangle: public Node {
	float x, y, width, height;
public:
	Rectangle(float x, float y, float width, float height);
	void draw();
};

class SceneGraph {
	std::set<Node*> nodes;
public:
	void add_node(Node* node);
	void draw();
};

class Window {
	SceneGraph scene_graph;
public:
	Window(int width, int height, const char* title);
	void add(Node* node);
	void run();
};

}
