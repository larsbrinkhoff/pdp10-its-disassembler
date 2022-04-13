/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

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

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

word_t
get_alto_word (FILE *f)
{
  word_t x1, x2, x3, x4, x5;
  word_t word;

  if (feof (f))
    return -1;

  x1 = get_byte (f);
  if (feof (f))
    return -1;
  x2 = get_byte (f);
  x3 = get_byte (f);
  x4 = get_byte (f);
  x5 = get_byte (f);
  word = (x1 << 32) | (x2 << 24) | (x3 << 16) | (x4 << 8) | x5;
  word &= 0777777777777LL;

  return word;
}

void
write_alto_word (FILE *f, word_t word)
{
  fputc ((word >> 32) & 0x0F, f);
  fputc ((word >> 24) & 0xFF, f);
  fputc ((word >> 16) & 0xFF, f);
  fputc ((word >>  8) & 0xFF, f);
  fputc ( word        & 0xFF, f);
}

struct word_format alto_word_format = {
  "alto",
  get_alto_word,
  NULL,
  by_five_octets,
  write_alto_word,
  NULL
};
