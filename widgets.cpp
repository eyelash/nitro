/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"

// Padding
nitro::Padding::Padding(float padding): padding(padding) {

}
void nitro::Padding::layout() {
	if (Node* child = get_child()) {
		child->set_location(padding, padding);
		child->set_size(get_width() - 2.f * padding, get_height() - 2.f * padding);
	}
}
float nitro::Padding::get_padding() const {
	return padding;
}
void nitro::Padding::set_padding(float padding) {
	if (padding == this->padding) {
		return;
	}
	this->padding = padding;
	Padding::layout();
}

// Alignment
nitro::Alignment::Alignment(HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment): horizontal_alignment(horizontal_alignment), vertical_alignment(vertical_alignment) {

}
void nitro::Alignment::layout() {
	Node* child = get_child();
	if (child == nullptr) {
		return;
	}
	float x, y;
	switch (horizontal_alignment) {
	case HorizontalAlignment::LEFT:
		x = 0.f;
		break;
	case HorizontalAlignment::CENTER:
		x = roundf((get_width() - child->get_width()) / 2.f);
		break;
	case HorizontalAlignment::RIGHT:
		x = roundf(get_width() - child->get_width());
		break;
	}
	switch (vertical_alignment) {
	case VerticalAlignment::BOTTOM:
		y = 0.f;
		break;
	case VerticalAlignment::CENTER:
		y = roundf((get_height() - child->get_height()) / 2.f);
		break;
	case VerticalAlignment::TOP:
		y = roundf(get_height() - child->get_height());
		break;
	}
	child->set_location(x, y);
}
nitro::HorizontalAlignment nitro::Alignment::get_horizontal_alignment() const {
	return horizontal_alignment;
}
nitro::VerticalAlignment nitro::Alignment::get_vertical_alignment() const {
	return vertical_alignment;
}
void nitro::Alignment::set_alignment(HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment) {
	if (horizontal_alignment == this->horizontal_alignment && vertical_alignment == this->vertical_alignment) {
		return;
	}
	this->horizontal_alignment = horizontal_alignment;
	this->vertical_alignment = vertical_alignment;
	layout();
}

// Rectangle
nitro::Rectangle::Rectangle(const Color& color) {
	node.set_parent(this);
	set_color(color);
}
nitro::Node* nitro::Rectangle::get_child(size_t index) {
	return index == 0 ? &node : Bin::get_child(index-1);
}
void nitro::Rectangle::layout() {
	node.set_size(get_width(), get_height());
	Bin::layout();
}
const nitro::Color& nitro::Rectangle::get_color() const {
	return node.get_color();
}
void nitro::Rectangle::set_color(const Color& color) {
	node.set_color(color);
}
nitro::Property<nitro::Color> nitro::Rectangle::color() {
	return Property<Color> {this, [](Rectangle* rectangle) {
		return rectangle->get_color();
	}, [](Rectangle* rectangle, Color color) {
		rectangle->set_color(color);
	}};
}

// RoundedRectangle
static float circle(float x) {
	return sqrtf(1.f - x * x);
}
static float int_circle(float x) {
	return 0.5f * (sqrtf(1.f-x*x) * x + asinf(x));
}
// computes (the area of) the intersection of the unit circle with the given rectangle (x, y, w, h)
// x, y, w and h are all assumed to be non-negative
static float rounded_corner_area(float x, float y, const float w, const float h) {
	if (x*x+y*y >= 1.f) return 0.f;
	// assert(x < 1.f && y < 1.f);

	float x1 = std::min(x + w, 1.f);
	float y1 = std::min(y + h, 1.f);
	float result = 0.f;

	const float cy1 = circle(y1);
	if (cy1 >= x1) return w * h; // completely inside
	if (cy1 > x) {
		result += (cy1 - x) * h;
		x = cy1;
	}

	const float cy = circle(y);
	if (cy < x1) x1 = cy;

	result += int_circle(x1) - int_circle(x) - (x1 - x) * y;
	return result;
}
static float rounded_corner(float radius, float x, float y, float w = 1.f, float h = 1.f) {
	w = w / radius;
	h = h / radius;
	return rounded_corner_area(x/radius, y/radius, w, h) / (w * h);
}
static nitro::Texture create_rounded_corner_texture(int radius) {
	std::vector<unsigned char> data (radius * radius);
	for (int y = 0; y < radius; ++y) {
		for (int x = 0; x < radius; ++x) {
			data[y*radius+x] = rounded_corner((float)radius, (float)x, (float)y) * 255.f + 0.5f;
		}
	}
	return nitro::Texture::create_from_data(radius, radius, 1, data.data());
}
nitro::RoundedRectangle::RoundedRectangle(const Color& color, float radius): radius(radius) {
	bottom_left.set_parent(this);
	bottom_right.set_parent(this);
	top_left.set_parent(this);
	top_right.set_parent(this);
	bottom.set_parent(this);
	center.set_parent(this);
	top.set_parent(this);

	Texture mask = create_rounded_corner_texture(radius);
	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask * texcoord);

	set_color(color);
}
nitro::Node* nitro::RoundedRectangle::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &center;
		case 6: return &top;
		default: return Bin::get_child(index-7);
	}
}
void nitro::RoundedRectangle::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);

	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, radius);
	center.set_location(0.f, radius);
	center.set_size(get_width(), get_height() - 2.f * radius);
	top.set_location(radius, get_height() - radius);
	top.set_size(get_width() - 2.f * radius, radius);

	Bin::layout();
}
const nitro::Color& nitro::RoundedRectangle::get_color() const {
	return center.get_color();
}
void nitro::RoundedRectangle::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	center.set_color(color);
	top.set_color(color);
}
nitro::Property<nitro::Color> nitro::RoundedRectangle::color() {
	return Property<Color> {this, [](RoundedRectangle* rectangle) {
		return rectangle->get_color();
	}, [](RoundedRectangle* rectangle, Color color) {
		rectangle->set_color(color);
	}};
}

// RoundedImage
nitro::RoundedImage::RoundedImage(float radius): radius(radius) {
	bottom_left.set_parent(this);
	bottom_right.set_parent(this);
	top_left.set_parent(this);
	top_right.set_parent(this);
	bottom.set_parent(this);
	center.set_parent(this);
	top.set_parent(this);

	Texture mask = create_rounded_corner_texture(radius);
	Quad mask_texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask * mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	top_left.set_mask(mask * mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	bottom_left.set_mask(mask * mask_texcoord);
	mask_texcoord = mask_texcoord.rotate();
	bottom_right.set_mask(mask * mask_texcoord);
}
nitro::RoundedImage nitro::RoundedImage::create_from_file(const char* file_name, float radius) {
	int width, height;
	Texture texture = Texture::create_from_file(file_name, width, height);
	RoundedImage rounded_image(radius);
	rounded_image.set_texture(texture);
	rounded_image.set_size(width, height);
	return rounded_image;
}
nitro::Node* nitro::RoundedImage::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &center;
		case 6: return &top;
		default: return Bin::get_child(index-7);
	}
}
void nitro::RoundedImage::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);

	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, radius);
	center.set_location(0.f, radius);
	center.set_size(get_width(), get_height() - 2.f * radius);
	top.set_location(radius, get_height() - radius);
	top.set_size(get_width() - 2.f * radius, radius);

	set_texture(texture);

	Bin::layout();
}
void nitro::RoundedImage::set_texture(const Texture& texture) {
	this->texture = texture;
	const float width = get_width();
	const float height = get_height();
	bottom_left.set_texture(texture * Quad(0, 0, radius/width, radius/height));
	bottom_right.set_texture(texture * Quad(1-radius/width, 0, 1, radius/height));
	top_left.set_texture(texture * Quad(0, 1-radius/height, radius/width, 1));
	top_right.set_texture(texture * Quad(1-radius/width, 1-radius/height, 1, 1));
	bottom.set_texture(texture * Quad(radius/width, 0, 1-radius/width, radius/height));
	center.set_texture(texture * Quad(0, radius/height, 1, 1-radius/height));
	top.set_texture(texture * Quad(radius/width, 1-radius/height, 1-radius/width, 1));
}
float nitro::RoundedImage::get_alpha() const {
	return center.get_alpha();
}
void nitro::RoundedImage::set_alpha(float alpha) {
	bottom_left.set_alpha(alpha);
	bottom_right.set_alpha(alpha);
	top_left.set_alpha(alpha);
	top_right.set_alpha(alpha);
	bottom.set_alpha(alpha);
	center.set_alpha(alpha);
	top.set_alpha(alpha);
}
nitro::Property<float> nitro::RoundedImage::alpha() {
	return create_property<float, RoundedImage, &RoundedImage::get_alpha, &RoundedImage::set_alpha>(this);
}

// RoundedBorder
static nitro::Texture create_rounded_border_texture(int radius, int width) {
	std::vector<unsigned char> data (radius * radius);
	for (int y = 0; y < radius; ++y) {
		for (int x = 0; x < radius; ++x) {
			const float value = rounded_corner((float)radius, (float)x, (float)y) - rounded_corner((float)(radius-width), (float)x, (float)y);
			data[y*radius+x] = value * 255.f + 0.5f;
		}
	}
	return nitro::Texture::create_from_data(radius, radius, 1, data.data());
}
nitro::RoundedBorder::RoundedBorder(float border_width, const Color& color, float radius): border_width(border_width), radius(radius) {
	bottom_left.set_parent(this);
	bottom_right.set_parent(this);
	top_left.set_parent(this);
	top_right.set_parent(this);
	bottom.set_parent(this);
	left.set_parent(this);
	right.set_parent(this);
	top.set_parent(this);

	Texture mask = create_rounded_border_texture(radius, border_width);
	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask * texcoord);

	set_color(color);
}
nitro::Node* nitro::RoundedBorder::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &left;
		case 6: return &right;
		case 7: return &top;
		default: return Bin::get_child(index-8);
	}
}
void nitro::RoundedBorder::layout() {
	bottom_left.set_location(0.f, 0.f);
	bottom_left.set_size(radius, radius);
	bottom_right.set_location(get_width() - radius, 0.f);
	bottom_right.set_size(radius, radius);
	top_left.set_location(0.f, get_height() - radius);
	top_left.set_size(radius, radius);
	top_right.set_location(get_width() - radius, get_height() - radius);
	top_right.set_size(radius, radius);
	bottom.set_location(radius, 0.f);
	bottom.set_size(get_width() - 2.f * radius, border_width);
	left.set_location(0.f, radius);
	left.set_size(border_width, get_height() - 2.f * radius);
	right.set_location(get_width() - border_width, radius);
	right.set_size(border_width, get_height() - 2.f * radius);
	top.set_location(radius, get_height() - border_width);
	top.set_size(get_width() - 2.f * radius, border_width);
	Bin::layout();
}
const nitro::Color& nitro::RoundedBorder::get_color() const {
	return bottom.get_color();
}
void nitro::RoundedBorder::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	left.set_color(color);
	right.set_color(color);
	top.set_color(color);
}
nitro::Property<nitro::Color> nitro::RoundedBorder::color() {
	return Property<Color> {this, [](RoundedBorder* border) {
		return border->get_color();
	}, [](RoundedBorder* border, Color color) {
		border->set_color(color);
	}};
}

// BlurredRectangle
static std::vector<float> create_gaussian_kernel(int radius) {
	std::vector<float> kernel (radius * 2 + 1);
	const float sigma = radius / 3.f;
	const float factor = 1.f / sqrtf(2.f * M_PI * sigma*sigma);
	for (unsigned int i = 0; i < kernel.size(); ++i) {
		const float x = (int)i - radius;
		kernel[i] = factor * expf(-x*x/(2.f*sigma*sigma));
	}
	return kernel;
}
static void blur(const std::vector<float>& kernel, std::vector<unsigned char>& buffer, float w, float h) {
	const int center = kernel.size() / 2;
	std::vector<unsigned char> tmp (buffer.size());
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			float sum = 0.f;
			for (unsigned int k = 0; k < kernel.size(); ++k) {
				int kx = x + k - center;
				if (kx < 0) kx = 0;
				else if (kx >= w) kx = w - 1;
				sum += buffer[y * w + kx] * kernel[k];
			}
			tmp[y * w + x] = sum + .5f;
		}
	}
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			float sum = 0.f;
			for (unsigned int k = 0; k < kernel.size(); ++k) {
				int ky = y + k - center;
				if (ky < 0) ky = 0;
				else if (ky >= h) ky = h - 1;
				sum += tmp[ky * w + x] * kernel[k];
			}
			buffer[y * w + x] = sum + .5f;
		}
	}
}
static nitro::Texture create_blurred_corner_texture(int radius, int blur_radius) {
	int size = radius + blur_radius * 2;
	std::vector<unsigned char> buffer (size * size);
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			const int i = y * size + x;
			const bool inside = x - blur_radius < radius && y - blur_radius < radius;
			buffer[i] = inside ? 255 : 0;
		}
	}
	for (int y = 0; y < radius; ++y) {
		for (int x = 0; x < radius; ++x) {
			const int i = (y + blur_radius) * size + (x + blur_radius);
			buffer[i] = rounded_corner(radius, x, y) * 255.f + .5f;
		}
	}
	blur(create_gaussian_kernel(blur_radius), buffer, size, size);
	return nitro::Texture::create_from_data(size, size, 1, buffer.data());
}
nitro::BlurredRectangle::BlurredRectangle(const Color& color, float radius, float blur_radius): radius(radius), blur_radius(blur_radius) {
	bottom_left.set_parent(this);
	bottom_right.set_parent(this);
	top_left.set_parent(this);
	top_right.set_parent(this);
	bottom.set_parent(this);
	left.set_parent(this);
	right.set_parent(this);
	top.set_parent(this);
	center.set_parent(this);

	Texture mask = create_blurred_corner_texture(radius, blur_radius);

	Quad texcoord (0.f, 0.f, 1.f, 1.f);
	top_right.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	top_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom_right.set_mask(mask * texcoord);

	texcoord = Quad(0.f, 0.f, 0.f, 1.f);
	top.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	left.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	bottom.set_mask(mask * texcoord);
	texcoord = texcoord.rotate();
	right.set_mask(mask * texcoord);

	set_color(color);
}
nitro::Node* nitro::BlurredRectangle::get_child(size_t index) {
	switch (index) {
		case 0: return &bottom_left;
		case 1: return &bottom_right;
		case 2: return &top_left;
		case 3: return &top_right;
		case 4: return &bottom;
		case 5: return &left;
		case 6: return &right;
		case 7: return &top;
		case 8: return &center;
		default: return Bin::get_child(index-9);
	}
}
void nitro::BlurredRectangle::layout() {
	const float size = radius + 2.f * blur_radius;
	bottom_left.set_location(-blur_radius, -blur_radius);
	bottom_left.set_size(size, size);
	bottom_right.set_location(get_width() - radius - blur_radius, -blur_radius);
	bottom_right.set_size(size, size);
	top_left.set_location(-blur_radius, get_height() - radius - blur_radius);
	top_left.set_size(size, size);
	top_right.set_location(get_width() - radius - blur_radius, get_height() - radius - blur_radius);
	top_right.set_size(size, size);
	bottom.set_location(radius + blur_radius, -blur_radius);
	bottom.set_size(get_width() - 2.f * (radius + blur_radius), size);
	left.set_location(-blur_radius, radius + blur_radius);
	left.set_size(size, get_height() - 2.f * (radius + blur_radius));
	right.set_location(get_width() - radius - blur_radius, radius + blur_radius);
	right.set_size(size, get_height() - 2.f * (radius + blur_radius));
	top.set_location(radius + blur_radius, get_height() - radius - blur_radius);
	top.set_size(get_width() - 2.f * (radius + blur_radius), size);
	center.set_location(radius + blur_radius, radius + blur_radius);
	center.set_size(get_width() - 2.f * (radius + blur_radius),  get_height() - 2.f * (radius + blur_radius));
	Bin::layout();
}
const nitro::Color& nitro::BlurredRectangle::get_color() const {
	return center.get_color();
}
void nitro::BlurredRectangle::set_color(const Color& color) {
	bottom_left.set_color(color);
	bottom_right.set_color(color);
	top_left.set_color(color);
	top_right.set_color(color);
	bottom.set_color(color);
	left.set_color(color);
	right.set_color(color);
	top.set_color(color);
	center.set_color(color);
}
