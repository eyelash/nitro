/*

Copyright (c) 2016-2018, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "nitro.hpp"
#include <hb-ft.h>

static hb_codepoint_t utf8_get_next(const char*& c) {
	hb_codepoint_t result = 0;
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

static bool script_is_real(hb_script_t script) {
	return script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED && script != HB_SCRIPT_UNKNOWN;
}
static bool compare_scripts(hb_script_t script1, hb_script_t script2) {
	return script_is_real(script1) && script_is_real(script2) && script1 != script2;
}

// Glyph
nitro::Glyph::Glyph(const Texture& texture, float x, float y, float width, float height): texture(texture), x(x), y(y), width(width), height(height) {

}
void nitro::Glyph::draw(const Color& color, const gles2::mat4& projection) const {
	CanvasElement(x, y, x + width, y + height, color, Texture(), 0.f, texture, Texture()).draw(projection);
}

// Font
static FT_Library initialize_freetype() {
	FT_Library library;
	FT_Init_FreeType(&library);
	return library;
}
nitro::Font::Font(const char* file_name, float size) {
	static FT_Library library = initialize_freetype();
	FT_New_Face(library, file_name, 0, &face);
	FT_Set_Pixel_Sizes(face, 0, size);
	hb_font = hb_ft_font_create(face, nullptr);
}
nitro::Font::~Font() {
	hb_font_destroy(hb_font);
	FT_Done_Face(face);
}
float nitro::Font::get_descender() const {
	return -face->size->metrics.descender >> 6;
}
float nitro::Font::get_height() const {
	return get_descender() + (face->size->metrics.ascender >> 6);
}
nitro::Glyph nitro::Font::render_glyph(unsigned int glyph) {
	FT_Load_Glyph(face, glyph, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);
	FT_Bitmap* bitmap = &face->glyph->bitmap;
	Texture texture = Texture::create_from_data(bitmap->width, bitmap->rows, 1, bitmap->buffer, true);
	return Glyph(texture, face->glyph->bitmap_left, face->glyph->bitmap_top - static_cast<int>(bitmap->rows), bitmap->width, bitmap->rows);
}
hb_font_t* nitro::Font::get_hb_font() {
	return hb_font;
}

// FontSet
nitro::Font* nitro::FontSet::load_font(int index) {
	auto iterator = fonts.find(index);
	if (iterator != fonts.end()) {
		return iterator->second.get();
	}
	FcPattern* font_pattern = FcFontRenderPrepare(nullptr, pattern, font_set->fonts[index]);
	char* file;
	double size;
	FcPatternGetString(font_pattern, FC_FILE, 0, reinterpret_cast<FcChar8**>(&file));
	FcPatternGetDouble(font_pattern, FC_PIXEL_SIZE, 0, &size);
	Font* font = new Font(file, size);
	FcPatternDestroy(font_pattern);
	fonts[index] = std::unique_ptr<Font>(font);
	return font;
}
nitro::FontSet::FontSet(const char* family, float size) {
	FcResult result;
	pattern = FcPatternCreate();
	FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(family));
	FcPatternAddDouble(pattern, FC_PIXEL_SIZE, size);
	FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	font_set = FcFontSort(nullptr, pattern, true, &char_set, &result);
}
nitro::FontSet::~FontSet() {
	FcCharSetDestroy(char_set);
	FcFontSetDestroy(font_set);
	FcPatternDestroy(pattern);
}
float nitro::FontSet::get_descender() {
	return load_font(0)->get_descender();
}
float nitro::FontSet::get_height() {
	return load_font(0)->get_height();
}
nitro::Font* nitro::FontSet::get_font(uint32_t character) {
	int i = 0;
	if (FcCharSetHasChar(char_set, character)) {
		for (i = 0; i < font_set->nfont; ++i) {
			FcCharSet* font_char_set;
			FcPatternGetCharSet(font_set->fonts[i], FC_CHARSET, 0, &font_char_set);
			if (FcCharSetHasChar(font_char_set, character)) {
				break;
			}
		}
	}
	return load_font(i);
}

// Text
nitro::Text::Text(FontSet* font_set, const char* text, const Color& color): color(color) {
	// TODO: handle bidirectional text
	hb_unicode_funcs_t* funcs = hb_unicode_funcs_get_default();
	float x = 0.f;
	float y = font_set->get_descender();
	const char* text_start = text;
	uint32_t codepoint = utf8_get_next(text);
	hb_script_t script = hb_unicode_script(funcs, codepoint);
	Font* font = font_set->get_font(codepoint);
	while (codepoint) {
		const char* text_end = text;
		uint32_t next_codepoint = utf8_get_next(text);
		hb_script_t next_script = hb_unicode_script(funcs, next_codepoint);
		Font* next_font = font_set->get_font(next_codepoint);
		if (next_codepoint == 0 || compare_scripts(next_script, script) || next_font != font) {
			hb_buffer_t* buffer = hb_buffer_create();
			hb_buffer_add_utf8(buffer, text_start, text_end - text_start, 0, -1);
			hb_buffer_guess_segment_properties(buffer);
			text_start = text_end;
			hb_shape(font->get_hb_font(), buffer, nullptr, 0);
			const unsigned int length = hb_buffer_get_length(buffer);
			hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, nullptr);
			hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, nullptr);
			for (unsigned int i = 0; i < length; ++i) {
				Glyph glyph = font->render_glyph(infos[i].codepoint);
				glyph.x = x + positions[i].x_offset / 64 + glyph.x;
				glyph.y = y + positions[i].y_offset / 64 + glyph.y;
				glyphs.push_back(glyph);
				x += positions[i].x_advance / 64;
				y += positions[i].y_advance / 64;
			}
			hb_buffer_destroy(buffer);
		}
		codepoint = next_codepoint;
		if (script_is_real(next_script)) {
			script = next_script;
		}
		font = next_font;
	}
	set_size(x, font_set->get_height());
}
void nitro::Text::draw(const DrawContext& draw_context) {
	for (const Glyph& glyph: glyphs) {
		glyph.draw(color, draw_context.projection);
	}
}
const nitro::Color& nitro::Text::get_color() const {
	return color;
}
void nitro::Text::set_color(const Color& color) {
	this->color = color;
}

// TextContainer
nitro::TextContainer::TextContainer(FontSet* font, const char* text, const Color& color, HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment): text(font, text, color), horizontal_alignment(horizontal_alignment), vertical_alignment(vertical_alignment) {
	layout();
}
nitro::Node* nitro::TextContainer::get_child(std::size_t index) {
	return index == 0 ? &text : nullptr;
}
void nitro::TextContainer::layout() {
	if (horizontal_alignment == HorizontalAlignment::CENTER)
		text.set_location_x(roundf((get_width() - text.get_width()) / 2.f));
	else if (horizontal_alignment == HorizontalAlignment::RIGHT)
		text.set_location_x(roundf(get_width() - text.get_width()));
	if (vertical_alignment == VerticalAlignment::TOP)
		text.set_location_y(roundf(get_height() - text.get_height()));
	else if (vertical_alignment == VerticalAlignment::CENTER)
		text.set_location_y(roundf((get_height() - text.get_height()) / 2.f));
}
const nitro::Color& nitro::TextContainer::get_color() const {
	return text.get_color();
}
void nitro::TextContainer::set_color(const Color& color) {
	text.set_color(color);
}
