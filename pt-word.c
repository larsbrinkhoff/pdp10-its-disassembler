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

/* This implements reading from paper tape images.

   36-bit words are stored six bits per paper tape frame, with the
   most significant bit set.  When reading, ignore frames without the
   most significant bit set. */

#include <stdio.h>

#include "dis.h"

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_pt_word (FILE *f)
{
  int i;
  word_t byte, word = 0;

  for (i = 0; i < 6; )
    {
      if (feof (f))
        return -1;

      byte = get_byte (f);
      if (byte & 0200)
        {
          word <<= 6;
          word |= byte & 077;
          i++;
        }
    }

  return word;
}

static void
write_pt_word (FILE *f, word_t word)
{
  fputc (((word >> 30) & 0x3F) | 0x80, f);
  fputc (((word >> 24) & 0x3F) | 0x80, f);
  fputc (((word >> 18) & 0x3F) | 0x80, f);
  fputc (((word >> 12) & 0x3F) | 0x80, f);
  fputc (((word >>  6) & 0x3F) | 0x80, f);
  fputc (( word        & 0x3F) | 0x80, f);
}

struct word_format pt_word_format = {
  "pt",
  get_pt_word,
  NULL,
  write_pt_word,
  NULL
};
