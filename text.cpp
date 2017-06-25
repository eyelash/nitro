/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "atmosphere.hpp"
#include <fontconfig/fontconfig.h>

static int utf8_get_next(const char*& c) {
	int result = 0;
	if ((c[0] & 0x80) == 0x00) {
		result = c[0];
		c += 1;
	}
	else if ((c[0] & 0xE0) == 0xC0) {
		result = (c[0] & 0x1F) << 6 | (c[1] & 0x3F);
		c += 2;
	}
	else if ((c[0] & 0xF0) == 0xE0) {
		result = (c[0] & 0x0F) << 12 | (c[1] & 0x3F) << 6 | (c[2] & 0x3F);
		c += 3;
	}
	else if ((c[0] & 0xF8) == 0xF0) {
		result = (c[0] & 0x07) << 18 | (c[1] & 0x3F) << 12 | (c[2] & 0x3F) << 6 | (c[3] & 0x3F);
		c += 4;
	}
	return result;
}

// Font
static FT_Library initialize_freetype() {
	FT_Library library;
	FT_Init_FreeType(&library);
	return library;
}
atmosphere::Font::Font(const char* family, float size) {
	static FT_Library library = initialize_freetype();
	FcPattern* pattern = FcPatternCreate();
	FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(family));
	FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	FcResult result;
	FcPattern* font = FcFontMatch(nullptr, pattern, &result);
	char* file_name;
	FcPatternGetString(font, FC_FILE, 0, reinterpret_cast<FcChar8**>(&file_name));
	FT_New_Face(library, file_name, 0, &face);
	FT_Set_Pixel_Sizes(face, 0, size);
	FcPatternDestroy(font);
	FcPatternDestroy(pattern);
	descender = -face->size->metrics.descender >> 6;
	font_height = descender + (face->size->metrics.ascender >> 6);
}
FT_GlyphSlot atmosphere::Font::load_char(int c) {
	FT_Load_Char(face, c, FT_LOAD_RENDER|FT_LOAD_TARGET_LIGHT);
	return face->glyph;
}

// Text
atmosphere::Text::Text(Font* font, const char* text, const Color& color) {
	float x = 0.f;
	float y = font->descender;
	while (int c = utf8_get_next(text)) {
		FT_GlyphSlot glyph = font->load_char(c);
		const int width = glyph->bitmap.width;
		const int height = glyph->bitmap.rows;
		auto texture = std::make_shared<gles2::Texture>(width, height, 1, glyph->bitmap.buffer);
		ColorMaskNode* node = new ColorMaskNode();
		node->set_location(x + glyph->bitmap_left, y + glyph->bitmap_top - height);
		node->set_size(width, height);
		node->set_color(color);
		node->set_mask(texture,  Quad(0.f, 1.f, 1.f, 0.f));
		glyphs.push_back(node);
		x += glyph->advance.x >> 6;
		y += glyph->advance.y >> 6;
	}
	set_size(x, font->font_height);
}
atmosphere::Node* atmosphere::Text::get_child(size_t index) {
	return index < glyphs.size() ? glyphs[index] : nullptr;
}
const atmosphere::Color& atmosphere::Text::get_color() const {
	return glyphs[0]->get_color();
}
void atmosphere::Text::set_color(const Color& color) {
	for (ColorMaskNode* mask: glyphs) {
		mask->set_color(color);
	}
}
atmosphere::Property<atmosphere::Color> atmosphere::Text::color() {
	return Property<Color> {this, [](Text* text) {
		return text->get_color();
	}, [](Text* text, Color color) {
		text->set_color(color);
	}};
}

// TextContainer
atmosphere::TextContainer::TextContainer(Font* font, const char* text, const Color& color, HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment): text(font, text, color), horizontal_alignment(horizontal_alignment), vertical_alignment(vertical_alignment) {
	layout();
}
atmosphere::Node* atmosphere::TextContainer::get_child(size_t index) {
	return index == 0 ? &text : nullptr;
}
void atmosphere::TextContainer::layout() {
	if (horizontal_alignment == HorizontalAlignment::CENTER)
		text.set_location_x(roundf((get_width() - text.get_width()) / 2.f));
	else if (horizontal_alignment == HorizontalAlignment::RIGHT)
		text.set_location_x(roundf(get_width() - text.get_width()));
	if (vertical_alignment == VerticalAlignment::TOP)
		text.set_location_y(roundf(get_height() - text.get_height()));
	else if (vertical_alignment == VerticalAlignment::CENTER)
		text.set_location_y(roundf((get_height() - text.get_height()) / 2.f));
}
const atmosphere::Color& atmosphere::TextContainer::get_color() const {
	return text.get_color();
}
void atmosphere::TextContainer::set_color(const Color& color) {
	text.set_color(color);
}
atmosphere::Property<atmosphere::Color> atmosphere::TextContainer::color() {
	return text.color();
}
