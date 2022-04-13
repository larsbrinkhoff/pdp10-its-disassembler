/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>

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

#include "libword.h"

static int position = 0;

static void
rewind_dta_word (FILE *f)
{
  position = 0;
  rewind (f);
}

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static inline word_t
get_half (FILE *f)
{
  return (get_byte (f)
	  + (get_byte (f) << 8)
	  + (get_byte (f) << 16)
	  + (get_byte (f) << 24));
}

static word_t
get_dta_word (FILE *f)
{
  word_t word;

  if (feof (f))
    return -1;

  word = (get_half (f) << 18);
  word += get_half (f);

  if ((position % 128) == 0)
    word |= START_RECORD;

  position++;
  return word;
}

static void
write_half (FILE *f, int word)
{
  fputc (word & 0377, f);
  fputc ((word >> 8) & 0377, f);
  fputc ((word >> 16) & 0377, f);
  fputc ((word >> 24) & 0377, f);
}

void
write_dta_word (FILE *f, word_t word)
{
  write_half (f, (word >> 18) & 0777777);
  write_half (f, word & 0777777);
}

struct word_format dta_word_format = {
  "dta",
  get_dta_word,
  rewind_dta_word,
  by_eight_octets,
  write_dta_word,
  NULL
};
