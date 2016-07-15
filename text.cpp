#include "atmosphere.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

atmosphere::Text::Text(const char* text, const Color& color): Node(0,0,0,0), glyphs(0.f, 0.f, 0.f, 0.f) {
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
		glyphs.add_child(new Mask{x+face->glyph->bitmap_left, y+face->glyph->bitmap_top-height, (float)width, (float)height, color, texture, {0.f, 1.f, 1.f, 0.f}});
		x += face->glyph->advance.x >> 6;
		y += face->glyph->advance.y >> 6;
		++text;
	}
	glyphs.width().set(x);
	glyphs.height().set(font_height);
}
atmosphere::Node* atmosphere::Text::get_child(int index) {
	return index == 0 ? &glyphs : nullptr;
}
void atmosphere::Text::layout(float width, float height) {
	glyphs.position_x().set(roundf((width - glyphs.width().get()) / 2.f));
	glyphs.position_y().set(roundf((height - glyphs.height().get()) / 2.f));
}
