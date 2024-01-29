#include <stdio.h>
#include "svg.h"

void svg_file_begin(FILE *f)
{
  fprintf(f, "<svg viewBox=\"%d %d %d %d\" ",
          -600, -600, 1200, 1200);
  fprintf(f, "xmlns=\"http://www.w3.org/2000/svg\">\n");
}

void svg_file_end(FILE *f)
{
  fprintf(f, "</svg>\n");
}

void svg_polyline_begin(FILE *f, int x, int y)
{
  fprintf(f, "  <polyline points=\"%d,%d", x, y);
}

void svg_polyline_point(FILE *f, int x, int y)
{
  fprintf(f, " %d,%d", x, y);
}

void svg_polyline_end(FILE *f)
{
  fprintf(f, "\" fill=\"none\" stroke=\"black\" />\n");
}

void svg_text_begin(FILE *f, int x, int y)
{
  fprintf(f, "  <text x=\"%d\" y=\"%d\">", x, y);
}

void svg_text_character(FILE *f, int c)
{
  switch(c) {
  case '<':
    fprintf(f, "&lt;");
    break;
  case '>':
    fprintf(f, "&gt;");
    break;
  case '&':
    fprintf(f, "&amp;");
    break;
  default:
    if (c <= 31)
      fprintf(f, "[^%c]", c + '@');
    else
      fprintf(f, "%c", c);
    break;
  }
}

void svg_text_end(FILE *f)
{
  fprintf(f, "</text>\n");
}
