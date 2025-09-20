#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>

// GL.hpp will include a non-namespace-polluting set of opengl prototypes:
#include "GL.hpp"

// Includes for libSDL:
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cmath>

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

// This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
// You probably shouldn't be looking here to learn to use either library.

int main(int argc, char **argv)
{
	const char *fontfile;
	const char *text;

	if (argc > 3)
	{
		throw "Crash";
	}

	fontfile = argv[1];
	text = argv[2];

	std::cout << fontfile << std::endl;

	FT_Library ft_library;
	FT_Face ft_face;
	FT_Error ft_error;

	if ((ft_error = FT_Init_FreeType(&ft_library)))
	{
		std::cout << "No library" << std::endl;
		abort();
	}
	if ((ft_error = FT_New_Face(ft_library, fontfile, 0, &ft_face)))
	{
		std::cout << "No Face " << ft_error << std::endl;
		abort();
	}
	if ((ft_error = FT_Set_Char_Size(ft_face, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0)))
	{
		std::cout << "No Char size" << std::endl;
		abort();
	}

	hb_font_t *hb_font;
	hb_font = hb_ft_font_create(ft_face, NULL);

	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8(hb_buffer, text, -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);

	hb_shape(hb_font, hb_buffer, NULL, 0);

	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

	/* Print them out as is. */
	printf("Raw buffer contents:\n");
	for (unsigned int i = 0; i < len; i++)
	{
		hb_codepoint_t gid = info[i].codepoint;
		unsigned int cluster = info[i].cluster;
		double x_advance = pos[i].x_advance / 64.;
		double y_advance = pos[i].y_advance / 64.;
		double x_offset = pos[i].x_offset / 64.;
		double y_offset = pos[i].y_offset / 64.;

		char glyphname[32];
		hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

		printf("glyph='%s'	cluster=%d	advance=(%g,%g)	offset=(%g,%g)\n",
			   glyphname, cluster, x_advance, y_advance, x_offset, y_offset);
	}

	printf("Converted to absolute positions:\n");
	/* And converted to absolute positions. */
	{
		double current_x = 0;
		double current_y = 0;
		for (unsigned int i = 0; i < len; i++)
		{
			hb_codepoint_t gid = info[i].codepoint;
			unsigned int cluster = info[i].cluster;
			double x_position = current_x + pos[i].x_offset / 64.;
			double y_position = current_y + pos[i].y_offset / 64.;

			char glyphname[32];
			hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

			printf("glyph='%s'	cluster=%d	position=(%g,%g)\n",
				   glyphname, cluster, x_position, y_position);

			current_x += pos[i].x_advance / 64.;
			current_y += pos[i].y_advance / 64.;
		}
	}

	// Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	// Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_Window *window = SDL_CreateWindow(
		"gp25 game4: choice-based game",		 // TODO: remember to set a title for your game!
		1280, 720,								 // TODO: modify window size if you'd like
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE // uncomment to allow resizing
			| SDL_WINDOW_HIGH_PIXEL_DENSITY		 // uncomment for full resolution on high-DPI screens
	);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	init_GL();

	glEnable(GL_FRAMEBUFFER_SRGB);

	GLuint textureId;

	for (unsigned int i = 0; i < len; i++)
	{
		FT_Load_Glyph(ft_face, info[0].codepoint, FT_LOAD_DEFAULT);

		FT_GlyphSlot slot = ft_face->glyph;
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

		FT_Bitmap bitmap = slot->bitmap;

		int twidth = pow(2, ceil(log(bitmap.width)/log(2)));
        int theight = pow(2, ceil(log(bitmap.rows)/log(2)));

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width, bitmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, twidth, theight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.buffer);

	}

	std::cout << "It worked?" << std::endl;

	hb_buffer_destroy(hb_buffer);
	hb_font_destroy(hb_font);

	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft_library);
}
