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

static int leftover, there_is_some_leftover = 0;

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

word_t
get_bin_word (FILE *f)
{
  unsigned char byte;
  word_t word;

  if (feof (f))
    return -1;

  if (there_is_some_leftover)
    {
      word = (word_t)leftover << 32 |
	     (word_t)get_byte (f) << 24 |
	     (word_t)get_byte (f) << 16 |
             (word_t)get_byte (f) <<  8 |
             (word_t)get_byte (f) <<  0;
      there_is_some_leftover = 0;
    }
  else
    {
      word =  ((word_t)get_byte (f) << 28);
      if (feof (f))
	return -1;
      word |= ((word_t)get_byte (f) << 20) |
	      ((word_t)get_byte (f) << 12) |
              ((word_t)get_byte (f) <<  4);
      byte = get_byte (f);
      word |=  (word_t)byte >> 4;
      there_is_some_leftover = 1;
      leftover = byte & 0x0f;
    }

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36/8 format]\n");
      exit (1);
    }

  return word;
}

void
rewind_bin_word (FILE *f)
{
  there_is_some_leftover = 0;
  rewind (f);
}
