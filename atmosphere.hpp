/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "gles2.hpp"
#include "animation.hpp"
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace atmosphere {

class Point {
public:
	float x, y;
	Point() {

	}
	constexpr Point(float x, float y): x{x}, y{y} {

	}
	constexpr Point operator+(const Point& p) const {
		return Point{x + p.x, y + p.y};
	}
	constexpr Point operator*(float f) const {
		return Point{x * f, y * f};
	}
};

class Color {
	float r, g, b, a;
	constexpr Color(float r, float g, float b, float a): r{r}, g{g}, b{b}, a{a} {

	}
public:
	Color() {

	}
	static constexpr Color create(float r, float g, float b, float a = 1.f) {
		return Color{r*a, g*a, b*a, a};
	}
	constexpr GLES2::vec4 get() const {
		return GLES2::vec4{r, g, b, a};
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
	virtual ~Node();
	virtual Node* get_child(int index);
	virtual void prepare_draw();
	virtual void draw(const DrawContext& draw_context);
	virtual void layout();
	virtual void mouse_enter();
	virtual void mouse_leave();
	virtual void mouse_motion(const GLES2::vec4& position);
	virtual void mouse_button_press(const GLES2::vec4& position, int button);
	virtual void mouse_button_release(const GLES2::vec4& position, int button);
	float get_location_x() const;
	void set_location_x(float x);
	float get_location_y() const;
	void set_location_y(float y);
	float get_width() const;
	void set_width(float width);
	float get_height() const;
	void set_height(float height);
	float get_rotation_z() const;
	void set_rotation_z(float rotation_z);
	void set_location(float x, float y);
	void set_size(float width, float height);
	bool is_mouse_inside() const;
	Property<Point> position();
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

class ColorNode: public Node {
	Color _color;
public:
	ColorNode();
	ColorNode(float x, float y, float width, float height, const Color& color);
	void draw(const DrawContext& draw_context) override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

struct Quad {
	GLfloat data[8];
	constexpr Quad rotate() const {
		return Quad{
			data[4], data[5],
			data[0], data[1],
			data[6], data[7],
			data[2], data[3]
		};
	}
	static constexpr Quad create(float x0, float y0, float x1, float y1) {
		return Quad{
			x0, y0,
			x1, y0,
			x0, y1,
			x1, y1
		};
	}
};

class TextureNode: public Node {
	GLES2::Texture* texture;
	Quad texcoord;
	float _alpha;
public:
	TextureNode();
	TextureNode(float x, float y, float width, float height, GLES2::Texture* texture, const Quad& texcoord);
	static TextureNode create_from_file(const char* file_name, float x = 0.f, float y = 0.f);
	void draw(const DrawContext& draw_context) override;
	void set_texture(GLES2::Texture* texture, const Quad& texcoord);
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

class ColorMaskNode: public Node {
	Color _color;
	GLES2::Texture* mask;
	Quad mask_texcoord;
public:
	ColorMaskNode();
	ColorMaskNode(float x, float y, float width, float height, const Color& color, GLES2::Texture* texture, const Quad& texcoord);
	static ColorMaskNode create_from_file(const char* file_name, const Color& color, float x = 0.f, float y = 0.f);
	void draw(const DrawContext& draw_context) override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
	void set_mask(GLES2::Texture* mask, const Quad& mask_texcoord);
};

class TextureMaskNode: public Node {
	GLES2::Texture* texture;
	Quad texcoord;
	GLES2::Texture* mask;
	Quad mask_texcoord;
	float _alpha;
public:
	TextureMaskNode();
	TextureMaskNode(float x, float y, float width, float height, GLES2::Texture* texture, const Quad& texcoord, GLES2::Texture* mask, const Quad& mask_texcoord);
	void draw(const DrawContext& draw_context) override;
	void set_texture(GLES2::Texture* texture, const Quad& texcoord);
	void set_mask(GLES2::Texture* mask, const Quad& mask_texcoord);
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

class Rectangle: public Bin {
	ColorNode node;
public:
	Rectangle(float x, float y, float width, float height, const Color& color);
	Node* get_child(int index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class Clip: public Bin {
	GLES2::FramebufferObject* fbo;
	TextureNode image;
public:
	Clip(float x, float y, float width, float height);
	void prepare_draw() override;
	void draw(const DrawContext& draw_context) override;
	void layout() override;
	Property<float> alpha();
};

class RoundedRectangle: public Bin {
	float radius;
	ColorMaskNode bottom_left;
	ColorMaskNode bottom_right;
	ColorMaskNode top_left;
	ColorMaskNode top_right;
	ColorNode bottom;
	ColorNode center;
	ColorNode top;
public:
	RoundedRectangle(float x, float y, float width, float height, const Color& color, float radius);
	Node* get_child(int index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class RoundedImage: public Bin {
	float radius;
	TextureMaskNode bottom_left;
	TextureMaskNode bottom_right;
	TextureMaskNode top_left;
	TextureMaskNode top_right;
	TextureNode bottom;
	TextureNode center;
	TextureNode top;
public:
	RoundedImage(float x, float y, float width, float height, GLES2::Texture* texture, float radius);
	static RoundedImage create_from_file(const char* file_name, float radius, float x = 0.f, float y = 0.f);
	Node* get_child(int index) override;
	void layout() override;
	void set_texture(GLES2::Texture* texture, const Quad& texcoord);
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

class RoundedBorder: public Bin {
	float border_width;
	float radius;
	ColorMaskNode bottom_left;
	ColorMaskNode bottom_right;
	ColorMaskNode top_left;
	ColorMaskNode top_right;
	ColorNode bottom;
	ColorNode left;
	ColorNode right;
	ColorNode top;
public:
	RoundedBorder(float x, float y, float width, float height, float border_width, const Color& color, float radius);
	Node* get_child(int index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class BlurredRectangle: public Bin {
	float radius;
	float blur_radius;
	ColorMaskNode bottom_left;
	ColorMaskNode bottom_right;
	ColorMaskNode top_left;
	ColorMaskNode top_right;
	ColorMaskNode bottom;
	ColorMaskNode left;
	ColorMaskNode right;
	ColorMaskNode top;
	ColorNode center;
public:
	BlurredRectangle(const Color& color, float radius, float blur_radius);
	Node* get_child(int index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
};

class Font {
	FT_Face face;
public:
	float descender;
	float font_height;
	Font(const char* file_name, float size);
	FT_GlyphSlot load_char(int c);
};

class Text: public Node {
	std::vector<ColorMaskNode*> glyphs;
public:
	Text(Font* font, const char* text, const Color& color);
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
	TextContainer(Font* font, const char* text, const Color& color, float width, float height, HorizontalAlignment horizontal_alignment = HorizontalAlignment::CENTER, VerticalAlignment vertical_alignment = VerticalAlignment::CENTER);
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
