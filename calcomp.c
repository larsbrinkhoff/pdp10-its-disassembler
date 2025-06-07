/* Copyright (C) 2025 Lars Brinkhoff <lars@nocrew.org>

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
//#include <unistd.h>
#include "dis.h"
#include "svg.h"

static int x, y;
static void (*plot)(FILE *f, int x, int y);

static void fatal(const char *message)
{
  fputs(message, stderr);
  exit(1);
}

static void just_move(FILE *f, int x, int y)
{
  (void)f;
  (void)x;
  (void)y;
}

static void process(int data)
{
  if ((data & 060) == 060)
    fatal ("Pen both up and down.");
  if (data & 040) {
    if (plot != just_move)
      svg_polyline_end(stdout);
    plot = just_move;
  }
  if (data & 020) {
    if (plot == just_move)
      plot = svg_polyline_begin;
  }
  if (data & 010)
    y--;
  if (data & 004)
    y++;
  if (data & 002)
    x--;
  if (data & 001)
    x++;
  plot (stdout, x, y);
  if (plot == svg_polyline_begin)
    plot = svg_polyline_point;
}

static void calcomp(FILE *f)
{
  word_t data;
  int i;

  for (;;) {
    data = get_word(f);
    if (data == -1)
      return;
    for (i = 0; i < 6; i++, data <<= 6)
      process((data >> 30) & 077);
  }
}

int main(void)
{
  input_word_format = &aa_word_format;
  x = y = 0;
  plot = just_move;
  svg_file_begin(stdout);
  calcomp(stdin);
  svg_file_end(stdout);
  return 0;
}
