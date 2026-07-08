#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ft.h"

static FT_Library ft;
static FT_Face face;

void ft_load(const char *font, int size, int dpi)
{
  FT_Error error;

  error = FT_Init_FreeType(&ft);
  if (error != FT_Err_Ok) {
    fprintf(stderr, "Error initializing font library: %s\n",
            FT_Error_String(error));
    exit(1);
  }

  error = FT_New_Face(ft, font, 0, &face);
  if (error != FT_Err_Ok) {
    fprintf(stderr, "Error loading font %s: %s\n",
            font, FT_Error_String(error));
    exit(1);
  }

  error = FT_Set_Char_Size(face, 0, size * 64, 0, dpi);
  if (error != FT_Err_Ok) {
    fprintf(stderr, "Error setting character size: %s\n",
            FT_Error_String(error));
    exit(1);
  }
}

int ft_char(int c, int *raster_width, int *character_width,
            int *height, int *kern, int *top)
{
  FT_UInt glyph = FT_Get_Char_Index(face, c);
  FT_Error error;

  if (glyph == 0)
    return 0;

  error = FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
  if (error != FT_Err_Ok) {
    fprintf(stderr, "Error loading glyph %d: %s\n",
            c, FT_Error_String(error));
    return 0;
  }
  if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP &&
      (error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO)) != FT_Err_Ok) {
    fprintf(stderr, "Error rendering glyph %d: %s\n",
            c, FT_Error_String(error));
    return 0;
  }

  *raster_width = face->glyph->bitmap.width;
  *character_width = face->glyph->advance.x / 64;
  *height = face->glyph->bitmap.rows;
  *kern = face->glyph->bitmap_left;
  *top = face->glyph->bitmap_top;
  return 1;
}

void ft_draw(void)
{
  FT_Bitmap *bitmap = &face->glyph->bitmap;
  unsigned i, j;

  for (i = 0; i < bitmap->rows; i++) {
    for (j = 0; j < bitmap->width; j++) {
      unsigned mask = 0x80 >> (j % 8);
      printf("%c", (bitmap->buffer[i * bitmap->pitch + j/8] & mask) != 0
             ? '*' : ' ');
    }
    printf("\n");
  }
}
