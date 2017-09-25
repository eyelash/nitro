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
#include <map>
#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

namespace nitro {

class Point {
public:
	float x, y;
	Point() {}
	constexpr Point(float x, float y): x(x), y(y) {}
	constexpr Point operator +(const Point& p) const {
		return Point(x + p.x, y + p.y);
	}
	constexpr Point operator -(const Point& p) const {
		return Point(x - p.x, y - p.y);
	}
	constexpr Point operator *(float f) const {
		return Point(x * f, y * f);
	}
};

class Color {
	float r, g, b, a;
	constexpr Color(float r, float g, float b, float a): r(r), g(g), b(b), a(a) {}
public:
	constexpr Color(): Color(0.f, 0.f, 0.f, 0.f) {}
	static constexpr Color create(float r, float g, float b, float a = 1.f) {
		return Color(r*a, g*a, b*a, a);
	}
	constexpr gles2::vec4 unpremultiply() const {
		return a == 0.f ? gles2::vec4 {0.f, 0.f, 0.f, 0.f} : gles2::vec4 {r/a, g/a, b/a, a};
	}
	constexpr Color operator +(const Color& c) const {
		return Color(r+c.r, g+c.g, b+c.b, a+c.a);
	}
	constexpr Color operator *(float x) const {
		return Color(r*x, g*x, b*x, a*x);
	}
	constexpr bool operator ==(const Color& c) const {
		return r == c.r && g == c.g && b == c.b && a == c.a;
	}
};

class BoundingBox {
	Point bottom_left;
	Point top_right;
	static constexpr float min(float x, float y) {
		return x < y ? x : y;
	}
	static constexpr float max(float x, float y) {
		return x < y ? y : x;
	}
	static constexpr Point min(const Point& p0, const Point& p1) {
		return Point(min(p0.x, p1.x), min(p0.y, p1.y));
	}
	static constexpr Point max(const Point& p0, const Point& p1) {
		return Point(max(p0.x, p1.x), max(p0.y, p1.y));
	}
public:
	constexpr BoundingBox(const Point& bottom_left, const Point& top_right): bottom_left(bottom_left), top_right(top_right) {}
	friend constexpr BoundingBox intersect(const BoundingBox& b0, const BoundingBox& b1) {
		return BoundingBox(max(b0.bottom_left, b1.bottom_left), min(b0.top_right, b1.top_right));
	}
	friend constexpr BoundingBox _union(const BoundingBox& b0, const BoundingBox& b1) {
		return BoundingBox(min(b0.bottom_left, b1.bottom_left), max(b0.top_right, b1.top_right));
	}
};

class Transformation {
	float tx, ty;
	float sx, sy;
public:
	constexpr Transformation(float tx, float ty, float sx = 1.f, float sy = 1.f): tx(tx), ty(ty), sx(sx), sy(sy) {}
	constexpr Transformation get_inverse() const {
		return Transformation(-tx/sx, -ty/sy, 1.f/sx, 1.f/sy);
	}
	constexpr gles2::mat4 get_matrix() const {
		return gles2::mat4(
			gles2::vec4(sx, 0.f, 0.f, 0.f),
			gles2::vec4(0.f, sy, 0.f, 0.f),
			gles2::vec4(0.f, 0.f, 1.f, 0.f),
			gles2::vec4(tx, ty, 0.f, 1.f)
		);
	}
	constexpr Point operator *(const Point& p) const {
		return Point(sx * p.x + tx, sy * p.y + ty);
	}
	constexpr Transformation operator *(const Transformation& t) const {
		return Transformation(sx * t.tx + tx, sy * t.ty + ty, sx * t.sx, sy * t.sy);
	}
};

class Quad {
	Point origin;
	Point x;
	Point y;
public:
	constexpr Quad(const Point& origin, const Point& x, const Point& y): origin(origin), x(x), y(y) {}
	constexpr Quad(): origin(0.f, 0.f), x(1.f, 0.f), y(0.f, 1.f) {}
	constexpr Quad(float x0, float y0, float x1, float y1): origin(x0, y0), x(x1, y0), y(x0, y1) {}
	constexpr Quad rotate() const {
		return Quad(y, origin, x + y - origin);
	}
	constexpr Point operator *(const Point& p) const {
		return origin + (x - origin) * p.x + (y - origin) * p.y;
	}
	constexpr Quad operator *(const Quad& t) const {
		return Quad(operator *(t.origin), operator *(t.x), operator *(t.y));
	}
	struct Data {
		GLfloat data[8];
		constexpr operator const GLfloat*() const {
			return data;
		}
	};
	constexpr Data get_data() const {
		return Data {
			origin.x, origin.y,
			x.x, x.y,
			y.x, y.y,
			x.x+y.x-origin.x, x.y+y.y-origin.y
		};
	}
};

struct Texture {
	std::shared_ptr<gles2::Texture> texture;
	Quad texcoord;
	Texture(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord);
	static Texture create_from_data(int width, int height, int depth, const unsigned char* data);
	static Texture create_from_file(const char* file_name, int& width, int& height);
};

struct DrawContext {
	gles2::mat4 projection;
	constexpr DrawContext(const gles2::mat4& projection): projection(projection) {}
};

class Node {
	Node* parent;
	float x, y;
	float width, height;
	float scale_x, scale_y;
	bool mouse_inside;
public:
	Node();
	virtual ~Node();
	virtual Node* get_child(size_t index);
	virtual void prepare_draw();
	virtual void draw(const DrawContext& draw_context);
	virtual void layout();
	virtual void mouse_enter();
	virtual void mouse_leave();
	virtual void mouse_motion(const Point& point);
	virtual void mouse_button_press(const Point& point, int button);
	virtual void mouse_button_release(const Point& point, int button);
	virtual void request_redraw();
	void set_parent(Node* parent);
	Transformation get_transformation() const;
	float get_location_x() const;
	void set_location_x(float x);
	float get_location_y() const;
	void set_location_y(float y);
	float get_width() const;
	void set_width(float width);
	float get_height() const;
	void set_height(float height);
	float get_scale_x() const;
	void set_scale_x(float scale_x);
	float get_scale_y() const;
	void set_scale_y(float scale_x);
	void set_location(float x, float y);
	void set_size(float width, float height);
	void set_scale(float scale_x, float scale_y);
	bool is_mouse_inside() const;
	Property<float> position_x();
	Property<float> position_y();
};

class Bin: public Node {
	Node* child;
	float padding;
public:
	Bin();
	Node* get_child(size_t index) override;
	void layout() override;
	void set_child(Node* node);
	void set_padding(float padding);
};

class SimpleContainer: public Node {
	std::vector<Node*> children;
public:
	SimpleContainer();
	Node* get_child(size_t index) override;
	void add_child(Node* node);
};

class ColorNode: public Node {
	Color _color;
public:
	ColorNode();
	void draw(const DrawContext& draw_context) override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class TextureNode: public Node {
	std::shared_ptr<gles2::Texture> texture;
	Quad texcoord;
	float _alpha;
public:
	TextureNode();
	static TextureNode create_from_file(const char* file_name, float x = 0.f, float y = 0.f);
	void draw(const DrawContext& draw_context) override;
	std::shared_ptr<gles2::Texture> get_texture() const;
	void set_texture(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord);
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

class ColorMaskNode: public Node {
	Color _color;
	std::shared_ptr<gles2::Texture> mask;
	Quad mask_texcoord;
public:
	ColorMaskNode();
	static ColorMaskNode create_from_file(const char* file_name, const Color& color);
	void draw(const DrawContext& draw_context) override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
	void set_mask(const std::shared_ptr<gles2::Texture>& mask, const Quad& mask_texcoord);
};

class TextureMaskNode: public Node {
	std::shared_ptr<gles2::Texture> texture;
	Quad texcoord;
	std::shared_ptr<gles2::Texture> mask;
	Quad mask_texcoord;
	float _alpha;
public:
	TextureMaskNode();
	void draw(const DrawContext& draw_context) override;
	void set_texture(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord);
	void set_mask(const std::shared_ptr<gles2::Texture>& mask, const Quad& mask_texcoord);
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

class Clip: public Bin {
	std::shared_ptr<gles2::FramebufferObject> fbo;
	TextureNode image;
public:
	Clip();
	void prepare_draw() override;
	void draw(const DrawContext& draw_context) override;
	void layout() override;
	float get_alpha() const;
	void set_alpha(float alpha);
	Property<float> alpha();
};

struct Glyph {
	std::shared_ptr<gles2::Texture> texture;
	Quad texcoord;
	float x, y;
	Glyph(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord, float x, float y);
};

class Font {
	FT_Face face;
	hb_font_t* hb_font;
public:
	Font(const char* file_name, float size);
	Font(const Font&) = delete;
	~Font();
	Font& operator =(const Font&) = delete;
	float get_descender() const;
	float get_height() const;
	Glyph render_glyph(unsigned int glyph);
	hb_font_t* get_hb_font();
};

class FontSet {
	FcPattern* pattern;
	FcFontSet* font_set;
	FcCharSet* char_set;
	std::map<int, std::unique_ptr<Font>> fonts;
	Font* load_font(int index);
public:
	FontSet(const char* family, float size);
	FontSet(const FontSet&) = delete;
	~FontSet();
	FontSet& operator =(const FontSet&) = delete;
	float get_descender();
	float get_height();
	Font* get_font(uint32_t character);
};

class Text: public Node {
	std::vector<ColorMaskNode*> glyphs;
public:
	Text(FontSet* font, const char* text, const Color& color);
	Node* get_child(size_t index) override;
	const Color& get_color() const;
	void set_color(const Color& color);
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
	TextContainer(FontSet* font, const char* text, const Color& color, HorizontalAlignment horizontal_alignment = HorizontalAlignment::CENTER, VerticalAlignment vertical_alignment = VerticalAlignment::CENTER);
	Node* get_child(size_t index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class Window: public Bin {
	DrawContext draw_context;
	bool needs_redraw;
	void dispatch_events();
public:
	Window(int width, int height, const char* title);
	void layout() override;
	void request_redraw() override;
	void run();
};

class Rectangle: public Bin {
	ColorNode node;
public:
	Rectangle(const Color& color);
	Node* get_child(size_t index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
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
	RoundedRectangle(const Color& color, float radius);
	Node* get_child(size_t index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
	Property<Color> color();
};

class RoundedImage: public Bin {
	Quad texcoord;
	float radius;
	TextureMaskNode bottom_left;
	TextureMaskNode bottom_right;
	TextureMaskNode top_left;
	TextureMaskNode top_right;
	TextureNode bottom;
	TextureNode center;
	TextureNode top;
public:
	RoundedImage(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord, float radius);
	static RoundedImage create_from_file(const char* file_name, float radius);
	Node* get_child(size_t index) override;
	void layout() override;
	void set_texture(const std::shared_ptr<gles2::Texture>& texture, const Quad& texcoord);
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
	RoundedBorder(float border_width, const Color& color, float radius);
	Node* get_child(size_t index) override;
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
	Node* get_child(size_t index) override;
	void layout() override;
	const Color& get_color() const;
	void set_color(const Color& color);
};

}
