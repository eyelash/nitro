#include "atmosphere.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

atmosphere::Text::Text(const char* text, const GLES2::vec4& color) {
	static FT_Library library = nullptr;
	static FT_Face face = nullptr;
	if (!face) {
		FT_Init_FreeType(&library);
		FT_New_Face(library, "/usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf", 0, &face);
		FT_Set_Pixel_Sizes(face, 0, 16);
	}

	float x = 0.f;
	float y = 0.f;
	while (*text != '\0') {
		FT_Load_Char(face, *text, FT_LOAD_RENDER|FT_LOAD_TARGET_LIGHT);
		const int width = face->glyph->bitmap.width;
		const int height = face->glyph->bitmap.rows;
		GLES2::Texture* texture = new GLES2::Texture{width, height, 1, face->glyph->bitmap.buffer};
		add_child(new Mask{x+face->glyph->bitmap_left, y+face->glyph->bitmap_top-height, width, height, color, texture, {0.f, 1.f, 1.f, 0.f}});
		x += face->glyph->advance.x >> 6;
		y += face->glyph->advance.y >> 6;
		++text;
	}
}
