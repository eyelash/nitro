/*

Copyright (c) 2016, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "gles2.hpp"
#include "animation.hpp"
#include <vector>

namespace atmosphere {

class Color {
	float r, g, b, a;
	constexpr Color(float r, float g, float b, float a): r{r}, g{g}, b{b}, a{a} {

	}
public:
	static constexpr Color create(float r, float g, float b, float a = 1.f) {
		return Color{r*a, g*a, b*a, a};
	}
	constexpr GLES2::vec4 unpremultiply() const {
		return a == 0.f ? GLES2::vec4{0.f, 0.f, 0.f, 0.f} : GLES2::vec4{r/a, g/a, b/a, a};
	}
	constexpr Color operator+(const Color& c) const {
		return Color{r+c.r, g+c.g, b+c.b, a+c.a};
	}
	constexpr Color operator*(float x) const {
		return Color{r*x, g*x, b*x, a*x};
	}
};

class Transformation {
public:
	float x, y;
	float scale_x, scale_y;
	float rotation_x, rotation_y, rotation_z;
	Transformation(float x, float y);
	GLES2::mat4 get_matrix(float width, float height) const;
	GLES2::mat4 get_inverse_matrix(float width, float height) const;
};

struct DrawContext {
	GLES2::mat4 projection;
};

class Node {
	Transformation transformation;
	float _width, _height;
	bool mouse_inside;
public:
	Node(float x, float y, float width, float height);
	virtual Node* get_child(int index);
	virtual void prepare_draw();
	virtual void draw(const DrawContext& draw_context);
	virtual void layout();
	void handle_mouse_motion_event(const GLES2::vec4& parent_position);
	virtual void mouse_enter();
	virtual void mouse_leave();
	Property<float> position_x();
	Property<float> position_y();
	Property<float> width();
	Property<float> height();
	Property<float> rotation_z();
};

class Bin: public Node {
	Node* child;
	float padding;
public:
	Bin(float x, float y, float width, float height, float padding = 0.f);
	Node* get_child(int index) override;
	void layout() override;
	void set_child(Node* node);
	void set_padding(float padding);
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

class Rectangle: public Bin {
	Color _color;
public:
	Rectangle(float x, float y, float width, float height, const Color& color);
	void draw(const DrawContext& draw_context) override;
	Property<Color> color();
};

struct Texcoord {
	GLfloat t[8];
	constexpr Texcoord rotate() const {
		return Texcoord{
			t[4], t[5],
			t[0], t[1],
			t[6], t[7],
			t[2], t[3]
		};
	}
	static constexpr Texcoord create(float x0, float y0, float x1, float y1) {
		return Texcoord{
			x0, y0,
			x1, y0,
			x0, y1,
			x1, y1
		};
	}
};

class Image: public Node {
	GLES2::Texture* texture;
	Texcoord texcoord;
	float _alpha;
public:
	Image(float x, float y, float width, float height, GLES2::Texture* texture, const Texcoord& texcoord);
	static Image create_from_file(const char* file_name, float x = 0.f, float y = 0.f);
	void draw(const DrawContext& draw_context) override;
	void set_texture(GLES2::Texture* texture);
	Property<float> alpha();
};

class Mask: public Node {
	Color _color;
	GLES2::Texture* mask;
	Texcoord mask_texcoord;
public:
	Mask(float x, float y, float width, float height, const Color& color, GLES2::Texture* texture, const Texcoord& texcoord);
	static Mask create_from_file(const char* file_name, const Color& color, float x = 0.f, float y = 0.f);
	void draw(const DrawContext& draw_context) override;
	Property<Color> color();
};

class ImageMask: public Node {
	GLES2::Texture* texture;
	Texcoord texcoord;
	GLES2::Texture* mask;
	Texcoord mask_texcoord;
public:
	ImageMask(float x, float y, float width, float height, GLES2::Texture* texture, const Texcoord& texcoord, GLES2::Texture* mask, const Texcoord& mask_texcoord);
	void draw(const DrawContext& draw_context) override;
};

class Clip: public Bin {
	GLES2::FramebufferObject* fbo;
	Image image;
public:
	Clip(float x, float y, float width, float height);
	void prepare_draw() override;
	void draw(const DrawContext& draw_context) override;
	void layout() override;
};

class RoundedRectangle: public Bin {
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
	void layout() override;
	Property<Color> color();
};

class RoundedImage: public Bin {
	float radius;
	ImageMask* bottom_left;
	Image* bottom;
	ImageMask* bottom_right;
	Image* center;
	ImageMask* top_left;
	Image* top;
	ImageMask* top_right;
public:
	RoundedImage(float x, float y, float width, float height, GLES2::Texture* texture, float radius);
	static RoundedImage create_from_file(const char* file_name, float radius, float x = 0.f, float y = 0.f);
	Node* get_child(int index) override;
};

class Text: public Node {
	std::vector<Mask*> glyphs;
public:
	Text(const char* text, const Color& color);
	Node* get_child(int index) override;
	Property<Color> color();
};

enum class HorizontalAlignment {
	LEFT,
	CENTER,
	RIGHT
};
enum class VerticalAlignment {
	TOP,
	CENTER,
	BOTTOM
};
class TextContainer: public Node {
	Text text;
	HorizontalAlignment horizontal_alignment;
	VerticalAlignment vertical_alignment;
public:
	TextContainer(const char* text, const Color& color, float width, float height, HorizontalAlignment horizontal_alignment = HorizontalAlignment::CENTER, VerticalAlignment vertical_alignment = VerticalAlignment::CENTER);
	Node* get_child(int index) override;
	void layout() override;
	Property<Color> color();
};

class Window: public Bin {
	DrawContext draw_context;
	void dispatch_events();
public:
	Window(int width, int height, const char* title);
	void run();
};

}
