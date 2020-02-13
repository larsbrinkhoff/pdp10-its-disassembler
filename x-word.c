/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>

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

#include "dis.h"

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_x_word (FILE *f)
{
  word_t word;

  if (feof (f))
    return -1;

  word =  (word_t)get_byte (f) << 32;
  if (feof (f))
    return -1;
  word |= (word_t)get_byte (f) << 24;
  word |= (word_t)get_byte (f) << 16;
  word |= (word_t)get_byte (f) <<  8;
  word |= (word_t)get_byte (f) <<  0;

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36/8 format]\n");
      exit (1);
    }

  return word;
}

static void
write_x_word (FILE *f, word_t word)
{
  fputc ((word >> 32) & 0x0f, f);
  fputc ((word >> 24) & 0xff, f);
  fputc ((word >> 16) & 0xff, f);
  fputc ((word >>  8) & 0xff, f);
  fputc ((word >>  0) & 0xff, f);
}

struct word_format x_word_format = {
  "x",
  get_x_word,
  NULL,
  write_x_word,
  NULL
};
