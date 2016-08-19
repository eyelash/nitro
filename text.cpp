/*

Copyright (c) 2016, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "atmosphere.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

atmosphere::Text::Text(const char* text, const Color& color): Node{0, 0, 0, 0} {
	static FT_Library library = nullptr;
	static FT_Face face = nullptr;
	static float descender;
	static float font_height;
	if (!face) {
		FT_Init_FreeType(&library);
		FT_New_Face(library, "/usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf", 0, &face);
		FT_Set_Pixel_Sizes(face, 0, 16);
		descender = -face->size->metrics.descender >> 6;
		font_height = descender + (face->size->metrics.ascender >> 6);
	}

	float x = 0.f;
	float y = descender;
	while (*text != '\0') {
		FT_Load_Char(face, *text, FT_LOAD_RENDER|FT_LOAD_TARGET_LIGHT);
		const int width = face->glyph->bitmap.width;
		const int height = face->glyph->bitmap.rows;
		GLES2::Texture* texture = new GLES2::Texture{width, height, 1, face->glyph->bitmap.buffer};
		glyphs.push_back(new Mask{x+face->glyph->bitmap_left, y+face->glyph->bitmap_top-height, (float)width, (float)height, color, texture, Texcoord::create(0.f, 1.f, 1.f, 0.f)});
		x += face->glyph->advance.x >> 6;
		y += face->glyph->advance.y >> 6;
		++text;
	}
	width().set(x);
	height().set(font_height);
}
atmosphere::Node* atmosphere::Text::get_child(int index) {
	return index < glyphs.size() ? glyphs[index] : nullptr;
}
atmosphere::Property<atmosphere::Color> atmosphere::Text::color() {
	return Property<Color> {this, [](Text* text) {
		return text->glyphs[0]->color().get();
	}, [](Text* text, Color color) {
		for (Mask* mask: text->glyphs) {
			mask->color().set(color);
		}
	}};
}

atmosphere::TextContainer::TextContainer(const char* text, const Color& color, float width, float height, HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment): Node{0, 0, width, height}, text{text, color}, horizontal_alignment{horizontal_alignment}, vertical_alignment{vertical_alignment} {
	layout();
}
atmosphere::Node* atmosphere::TextContainer::get_child(int index) {
	return index == 0 ? &text : nullptr;
}
void atmosphere::TextContainer::layout() {
	if (horizontal_alignment == HorizontalAlignment::CENTER)
		text.position_x().set(roundf((width().get() - text.width().get()) / 2.f));
	else if (horizontal_alignment == HorizontalAlignment::RIGHT)
		text.position_x().set(roundf(width().get() - text.width().get()));
	if (vertical_alignment == VerticalAlignment::TOP)
		text.position_y().set(roundf(height().get() - text.height().get()));
	else if (vertical_alignment == VerticalAlignment::CENTER)
		text.position_y().set(roundf((height().get() - text.height().get()) / 2.f));
}
atmosphere::Property<atmosphere::Color> atmosphere::TextContainer::color() {
	return text.color();
}
