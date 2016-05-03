#include "gles2.hpp"
#include <set>
#include <vector>
#include <functional>

namespace atmosphere {

class Node {
protected:
	float x, y, width, height;
public:
	virtual void draw(const GLES2::mat4& projection) = 0;
	void set_position(float x, float y);
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

class AnimationType {
public:
	virtual float get_y (float x) const = 0;
	static const AnimationType* LINEAR;
	static const AnimationType* ACCELERATING;
	static const AnimationType* DECELERATING;
	static const AnimationType* OSCILLATING;
	static const AnimationType* SWAY;
};
class LinearAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class AcceleratingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class DeceleratingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class OscillatingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class SwayAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};

class Animation {
	using ApplyFunction = std::function<void(float)>;
	float start_value, end_value;
	long start_time, duration;
	ApplyFunction apply_function;
	const AnimationType* type;
	static std::vector<Animation> animations;
	static long time;
public:
	Animation(float from, float to, long duration, const ApplyFunction& apply_function, const AnimationType* type);
	bool apply();
	static void animate(float from, float to, long duration, const ApplyFunction& apply_function, const AnimationType* type = AnimationType::SWAY);
	static void set_time(long time);
	static void apply_all();
};

}
