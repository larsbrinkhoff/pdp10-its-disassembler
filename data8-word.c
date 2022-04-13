/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

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

/* One 36-bit word stored right aligned per little endian 64-bit word. */

#include <stdio.h>
#include "dis.h"

static word_t
get_data8_word (FILE *f)
{
  word_t word = 0;
  int c, i;

  for (i = 0; i < 64; i += 8)
    {
      c = fgetc (f);
      if (c == EOF)
        return -1;
      word |= (word_t)(c & 0xff) << i;
    }

  if (word & 0xFFFFFFF000000000LL)
    fprintf (stderr, "WARNING: garbage in data8 word: %012llo.\n", word);

  return word;
}

static void
write_data8_word (FILE *f, word_t word)
{
  fputc ((word >>  0) & 0xff, f);
  fputc ((word >>  8) & 0xff, f);
  fputc ((word >> 16) & 0xff, f);
  fputc ((word >> 24) & 0xff, f);
  fputc ((word >> 32) & 0xff, f);
  fputc (0, f);
  fputc (0, f);
  fputc (0, f);
}

struct word_format data8_word_format = {
  "data8",
  get_data8_word,
  NULL,
  by_eight_octets,
  write_data8_word,
  NULL
};
