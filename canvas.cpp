/*

Copyright (c) 2018, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <canvas.fs.glsl.h>
#include <canvas.vs.glsl.h>
#include <queue>
#include <set>
#include <algorithm>
#include <cassert>

class CanvasProgram: public gles2::Program {
public:
	GLint projection_location;
	GLint vertex_location;
	GLint use_texture_location;
	GLint use_mask_location;
	GLint use_inverted_mask_location;
	GLint color_location;
	GLint texture_location;
	GLint texture_texcoord_location;
	GLint mask_location;
	GLint mask_texcoord_location;
	GLint inverted_mask_location;
	GLint inverted_mask_texcoord_location;
	CanvasProgram(): gles2::Program(canvas_vs_glsl, canvas_fs_glsl) {
		projection_location = get_uniform_location("projection");
		vertex_location = get_attribute_location("vertex");
		use_texture_location = get_uniform_location("use_texture");
		use_mask_location = get_uniform_location("use_mask");
		use_inverted_mask_location = get_uniform_location("use_inverted_mask");
		color_location = get_attribute_location("color");
		texture_location = get_uniform_location("texture");
		texture_texcoord_location = get_attribute_location("texture_texcoord");
		mask_location = get_uniform_location("mask");
		mask_texcoord_location = get_attribute_location("mask_texcoord");
		inverted_mask_location = get_uniform_location("inverted_mask");
		inverted_mask_texcoord_location = get_attribute_location("inverted_mask_texcoord");
	}
};

nitro::CanvasElement::CanvasElement(float x0, float y0, float x1, float y1, const Color& color, const Texture& texture, const Texture& mask, const Texture& inverted_mask): Rectangle(x0, y0, x1, y1), color(color), texture(texture), mask(mask), inverted_mask(inverted_mask) {

}

void nitro::CanvasElement::draw(const gles2::mat4& projection) const {
	static CanvasProgram program;
	const Quad vertices(x0, y0, x1, y1);
	gles2::draw(
		&program,
		GL_TRIANGLE_STRIP,
		4,
		gles2::UniformMat4(program.projection_location, projection),
		gles2::AttributeArray(program.vertex_location, 2, GL_FLOAT, vertices.get_data()),
		gles2::UniformBool(program.use_texture_location, texture),
		gles2::UniformBool(program.use_mask_location, mask),
		gles2::UniformBool(program.use_inverted_mask_location, inverted_mask),
		gles2::AttributeVec4(program.color_location, color.unpremultiply()),
		gles2::TextureState(texture.texture.get(), GL_TEXTURE0, program.texture_location),
		gles2::AttributeArray(program.texture_texcoord_location, 2, GL_FLOAT, texture.texcoord.get_data()),
		gles2::TextureState(mask.texture.get(), GL_TEXTURE1, program.mask_location),
		gles2::AttributeArray(program.mask_texcoord_location, 2, GL_FLOAT, mask.texcoord.get_data()),
		gles2::TextureState(inverted_mask.texture.get(), GL_TEXTURE2, program.inverted_mask_location),
		gles2::AttributeArray(program.inverted_mask_texcoord_location, 2, GL_FLOAT, inverted_mask.texcoord.get_data())
	);
}

void nitro::Canvas::clear() {
	elements.clear();
}

void nitro::Canvas::set_color(float x, float y, float width, float height, const Color& color) {
	elements.emplace_back(x, y, x + width, y + height, color, Texture(), Texture(), Texture());
}

void nitro::Canvas::set_texture(float x, float y, float width, float height, const Texture& texture) {
	elements.emplace_back(x, y, x + width, y + height, Color(), texture, Texture(), Texture());
}

void nitro::Canvas::set_mask(float x, float y, float width, float height, const Texture& mask) {
	elements.emplace_back(x, y, x + width, y + height, Color(), Texture(), mask, Texture());
}

void nitro::Canvas::set_inverted_mask(float x, float y, float width, float height, const Texture& inverted_mask) {
	elements.emplace_back(x, y, x + width, y + height, Color(), Texture(), Texture(), inverted_mask);
}

void nitro::Canvas::prepare() {
	class Event {
	public:
		enum class Type {
			START,
			END
		};
		Type type;
		const CanvasElement* element;
		float y;
		constexpr Event(Type type, const CanvasElement* element): type(type), element(element), y(type == Type::START ? element->y0 : element->y1) {}
		constexpr bool operator >(const Event& event) const {
			return y != event.y ? y > event.y : element > event.element;
		}
	};

	class ElementStack {
		mutable float y0;
		mutable std::set<const CanvasElement*> elements;
	public:
		float x0, x1;
		ElementStack(const CanvasElement* element, float x0, float x1): y0(element->y0), x0(x0), x1(x1) {
			elements.insert(element);
		}
		void insert(const CanvasElement* element) const {
			elements.insert(element);
		}
		void remove(const CanvasElement* element) const {
			elements.erase(element);
		}
		bool empty() const {
			return elements.empty();
		}
		void get_element(float y1, std::vector<CanvasElement>& new_elements) const {
			if (y0 == y1) {
				return;
			}
			Color color;
			Texture texture;
			Texture mask;
			Texture inverted_mask;
			for (const CanvasElement* element: elements) {
				Quad quad(
					(x0 - element->x0) / (element->x1 - element->x0),
					(y0 - element->y0) / (element->y1 - element->y0),
					(x1 - element->x0) / (element->x1 - element->x0),
					(y1 - element->y0) / (element->y1 - element->y0)
				);
				if (element->texture) {
					texture = element->texture * quad;
				}
				else if (element->mask) {
					mask = element->mask * quad;
				}
				else if (element->inverted_mask) {
					inverted_mask = element->inverted_mask * quad;
				}
				else {
					color = element->color;
					texture = Texture();
				}
			}
			if (color || texture) {
				new_elements.push_back(CanvasElement(x0, y0, x1, y1, color, texture, mask, inverted_mask));
			}
			y0 = y1;
		}
		ElementStack split_horizontally(float x) {
			ElementStack left = *this;
			left.x1 = x;
			x0 = x;
			return left;
		}
		bool operator <(const ElementStack& element_stack) const {
			return x0 < element_stack.x0;
		}
	};

	// collect events
	std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;
	for (const CanvasElement& element: elements) {
		events.emplace(Event::Type::START, &element);
		events.emplace(Event::Type::END, &element);
	}

	// process events
	std::vector<CanvasElement> new_elements;
	std::set<ElementStack> stacks;
	while (!events.empty()) {
		Event event = events.top();
		events.pop();
		const CanvasElement* element = event.element;
		auto i0 = std::upper_bound(stacks.begin(), stacks.end(), element->x0, [](float x, const ElementStack& stack) {
			return x < stack.x1;
		});
		auto i1 = std::lower_bound(stacks.begin(), stacks.end(), element->x1, [](const ElementStack& stack, float x) {
			return stack.x0 < x;
		});
		if (event.type == Event::Type::START) {
			float x = element->x0;
			while (i0 != i1) {
				assert(i0->x1 > element->x0 && i0->x0 < element->x1);
				if (i0->x0 < element->x0) {
					assert(i0->x0 < element->x0 && element->x0 < i0->x1);
					ElementStack stack = *i0;
					stacks.erase(i0);
					ElementStack left = stack.split_horizontally(element->x0);
					stacks.insert(left);
					i0 = stacks.insert(stack).first;
				}
				if (element->x1 < i0->x1) {
					assert(i0->x0 < element->x1 && element->x1 < i0->x1);
					ElementStack stack = *i0;
					stacks.erase(i0);
					ElementStack left = stack.split_horizontally(element->x1);
					i0 = stacks.insert(left).first;
					i1 = stacks.insert(stack).first;
				}
				if (x < i0->x0) {
					// fill the gap
					stacks.insert(ElementStack(element, x, i0->x0));
				}
				i0->get_element(event.y, new_elements);
				i0->insert(element);
				x = i0->x1;
				++i0;
			}
			if (x < element->x1) {
				stacks.insert(ElementStack(element, x, element->x1));
			}
		}
		else {
			while (i0 != i1) {
				i0->get_element(event.y, new_elements);
				i0->remove(element);
				if (i0->empty()) {
					i0 = stacks.erase(i0);
				}
				else {
					++i0;
				}
			}
		}
	}
	assert(stacks.empty());
	elements = new_elements;
}

void nitro::Canvas::draw(const gles2::mat4& projection) const {
	for (const CanvasElement& element: elements) {
		element.draw(projection);
	}
}
