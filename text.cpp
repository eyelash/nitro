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
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/ubidi.h>
#include <unicode/uscript.h>
#include <unicode/uchriter.h>
#include <hb-icu.h>
#include <hb-ft.h>

// private ICU API
struct UScriptRun;
U_CAPI UScriptRun* U_EXPORT2 uscript_openRun(const UChar* src, int32_t length, UErrorCode* pErrorCode);
U_CAPI void U_EXPORT2 uscript_closeRun(UScriptRun* scriptRun);
U_CAPI UBool U_EXPORT2 uscript_nextRun(UScriptRun* scriptRun, int32_t* pRunStart, int32_t* pRunLimit, UScriptCode *pRunScript);

// Glyph
nitro::Glyph::Glyph(const Texture& texture, float x, float y, float width, float height): texture(texture), x(x), y(y), width(width), height(height) {

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
	return Glyph(texture, face->glyph->bitmap_left, face->glyph->bitmap_top - bitmap->rows, bitmap->width, bitmap->rows);
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
nitro::Text::Text(FontSet* font_set, const char* text_utf8, const Color& color) {
	UErrorCode status = U_ZERO_ERROR;
	// convert UTF-8 to UTF-16
	int32_t text_length;
	u_strFromUTF8(nullptr, 0, &text_length, text_utf8, -1, &status);
	status = U_ZERO_ERROR;
	std::vector<UChar> text(text_length);
	u_strFromUTF8(text.data(), text.size(), nullptr, text_utf8, -1, &status);
	if (U_FAILURE(status)) {
		fprintf(stderr, "error: %s\n", u_errorName(status));
	}
	// BiDi iterator
	UBiDi* bidi = ubidi_openSized(text.size(), 0, &status);
	ubidi_setPara(bidi, text.data(), text.size(), UBIDI_DEFAULT_LTR, nullptr, &status);
	int32_t bidi_limit;
	UBiDiLevel bidi_level;
	ubidi_getLogicalRun(bidi, 0, &bidi_limit, &bidi_level);
	// script iterator
	UScriptRun* script_iterator = uscript_openRun(text.data(), text.size(), &status);
	int32_t script_limit;
	UScriptCode script;
	uscript_nextRun(script_iterator, nullptr, &script_limit, &script);
	// iterate code points
	float x = 0.f;
	float y = font_set->get_descender();
	icu::UCharCharacterIterator iter(text.data(), text.size());
	int32_t previous_index = 0;
	Font* previous_font = font_set->get_font(iter.first32());
	while (iter.hasNext()) {
		const UChar32 c = iter.next32();
		const int32_t index = iter.getIndex();
		Font* font = font_set->get_font(c);
		if (index == bidi_limit || index == script_limit || font != previous_font) {
			hb_buffer_t* buffer = hb_buffer_create();
			hb_buffer_add_utf16(buffer, reinterpret_cast<uint16_t*>(text.data()), text.size(), previous_index, index-previous_index);
			hb_buffer_set_direction(buffer, (bidi_level & 1) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
			hb_buffer_set_script(buffer, hb_icu_script_to_script(script));
			hb_buffer_guess_segment_properties(buffer);
			hb_shape(previous_font->get_hb_font(), buffer, nullptr, 0);
			const unsigned int length = hb_buffer_get_length(buffer);
			hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, nullptr);
			hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, nullptr);
			for (unsigned int i = 0; i < length; ++i) {
				Glyph glyph = previous_font->render_glyph(infos[i].codepoint);
				ColorMaskNode* node = new ColorMaskNode();
				node->set_location(x + glyph.x, y + glyph.y);
				node->set_size(glyph.width, glyph.height);
				node->set_color(color);
				node->set_mask(glyph.texture);
				glyphs.push_back(node);
				x += positions[i].x_advance >> 6;
				y += positions[i].y_advance >> 6;
			}
			hb_buffer_destroy(buffer);
			previous_index = index;
			previous_font = font;
			if (index == bidi_limit) {
				ubidi_getLogicalRun(bidi, index, &bidi_limit, &bidi_level);
			}
			if (index == script_limit) {
				uscript_nextRun(script_iterator, nullptr, &script_limit, &script);
			}
		}
	}
	ubidi_close(bidi);
	uscript_closeRun(script_iterator);
	set_size(x, font_set->get_height());
}
nitro::Node* nitro::Text::get_child(size_t index) {
	return index < glyphs.size() ? glyphs[index] : nullptr;
}
const nitro::Color& nitro::Text::get_color() const {
	return glyphs[0]->get_color();
}
void nitro::Text::set_color(const Color& color) {
	for (ColorMaskNode* mask: glyphs) {
		mask->set_color(color);
	}
}
nitro::Property<nitro::Color> nitro::Text::color() {
	return Property<Color> {this, [](Text* text) {
		return text->get_color();
	}, [](Text* text, Color color) {
		text->set_color(color);
	}};
}

// TextContainer
nitro::TextContainer::TextContainer(FontSet* font, const char* text, const Color& color, HorizontalAlignment horizontal_alignment, VerticalAlignment vertical_alignment): text(font, text, color), horizontal_alignment(horizontal_alignment), vertical_alignment(vertical_alignment) {
	layout();
}
nitro::Node* nitro::TextContainer::get_child(size_t index) {
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
nitro::Property<nitro::Color> nitro::TextContainer::color() {
	return text.color();
}
