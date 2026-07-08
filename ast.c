/* Copyright (C) 2026 Lars Brinkhoff <lars@nocrew.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include "ft.h"

/* XGP resolution in dots per inch. */
#define XGP_DPI 192

static const char *its_file;

static void octal(int x, const char *id)
{
  printf("%o %s %s\n", x, id, its_file);
}

static void decimal(int x, const char *id)
{
  printf("%d %s\n", x, id);
}

static void formfeed(void)
{
  printf("\014");
}

static void info(int height, int base)
{
  int kstid = 0;
  int adjustment = 0;

  printf("%d KSTID %s\n", kstid, its_file);
  decimal(height, "HEIGHT");
  decimal(base, "BASE LINE");
  decimal(adjustment, "COLUMN POSITION ADJUSTMENT");
  formfeed();
}

static void character(int code, int raster_width, int char_width,
                      int kern, int top)
{
  int i;
  octal(code, "CHARACTER CODE");
  decimal(raster_width, "RASTER WIDTH");
  decimal(char_width, "CHARACTER WIDTH");
  decimal(kern, "LEFT KERN");
  for (i = 0; i < top; i++)
    printf("\n");
  ft_draw();
  formfeed();
}

static void font(void)
{
  int max_height = 0, max_top = 0;
  int raster_width, character_width, height, kern, top;
  int i;

  for (i = 0; i < 128; i++) {
    if (!ft_char(i, &raster_width, &character_width, &height, &kern, &top))
      continue;
    if (height > max_height)
      max_height = height;
    if (top > max_top)
      max_top = top;
  }

  info(max_height, max_height - max_top);
  for (i = 0; i < 128; i++) {
    if (!ft_char(i, &raster_width, &character_width, &height, &kern, &top))
      continue;
    character(i, raster_width, character_width, kern, max_top - top);
  }
}

int main(int argc, char **argv)
{
  int size;

  if (argc != 4) {
    fprintf(stderr, "Usage %s <font size> <ITS target file name> <source font file>\n",
            argv[0]);
    return 1;
  }

  size = atoi(argv[1]);
  its_file = argv[2];
  ft_load(argv[3], size, XGP_DPI);
  font();

  return 0;
}
