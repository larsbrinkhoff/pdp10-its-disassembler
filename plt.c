/* Copyright (C) 2024 Lars Brinkhoff <lars@nocrew.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <unistd.h>
#include "dis.h"
#include "svg.h"

static void finish(word_t data, void (*fn)(FILE *))
{
  if (data != -1)
    return;
  if (fn != NULL)
    fn(stdout);
  exit(0);
}

static int sext(int data)
{
  data &= 0377777;
  data ^= 0200000;
  return data - 0200000;
}

static int posx(word_t data)
{
  return sext(data >> 19);
}

static int posy(word_t data)
{
  return sext(data >> 1);
}

static void (*plot)(FILE *f, int x, int y);

static void vector(word_t data)
{
  int x = posx(data);
  int y = posy(data);
  plot(stdout, x, y);
}

static word_t vectors(FILE *f, word_t data)
{
  plot = svg_polyline_begin;
  do {
    vector(data);
    plot = svg_polyline_point;
    data = get_word(f);
    finish(data, svg_polyline_end);
  } while (data & 1);
  svg_polyline_end(stdout);
  return data;
}

static void ascii(word_t data)
{
  int c1, c2, c3, c4, c5;
  c1 = (data >> 29) & 0177;
  c2 = (data >> 22) & 0177;
  c3 = (data >> 15) & 0177;
  c4 = (data >>  8) & 0177;
  c5 = (data >>  1) & 0177;
  if (c1)
    svg_text_character(stdout, c1);
  if (c2)
    svg_text_character(stdout, c2);
  if (c3)
    svg_text_character(stdout, c3);
  if (c4)
    svg_text_character(stdout, c4);
  if (c5)
    svg_text_character(stdout, c5);
}

static word_t text(FILE *f, word_t data)
{
  int x = posx(data);
  int y = posy(data);

  if ((data & 0777777000000LL) == 0400001000000LL)
    finish(-1, NULL);

  svg_text_begin(stdout, x, y);

  data = get_word(f);
  finish(data, svg_text_end);
  if ((data & 1) == 0) {
    svg_text_end(stdout);
    return data;
  }

  if (data & 0777777000000LL) {
    printf("Not text\n");
    exit(1);
  }

  for(;;) {
    data = get_word(f);
    finish(data, svg_text_end);
    if (data & 1)
      ascii(data);
    else {
      svg_text_end(stdout);
      return data;
    }
  }
}

static void plt(FILE *f)
{
  word_t data;

  data = get_word(f);
  finish(data, NULL);

  for (;;) {
      switch (data & 01000001) {
      case 00000000:
        data = vectors(f, data);
        break;
      case 01000000:
        data = text(f, data);
        break;
      default:
        fprintf(stderr, "Error in input: %012llo\n", data);
        exit(1);
    }
  }
}

int main(void)
{
  svg_file_begin(stdout);
  plt(stdin);
  return 0;
}
