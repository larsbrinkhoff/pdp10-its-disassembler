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

static int leftover_input, have_leftover_input = 0;
static int leftover_output, have_leftover_output = 0;

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_bin_word (FILE *f)
{
  unsigned char byte;
  word_t word;

  if (feof (f))
    return -1;

  if (have_leftover_input)
    {
      word = (word_t)leftover_input << 32 |
	     (word_t)get_byte (f) << 24 |
	     (word_t)get_byte (f) << 16 |
             (word_t)get_byte (f) <<  8 |
             (word_t)get_byte (f) <<  0;
      have_leftover_input = 0;
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
      have_leftover_input = 1;
      leftover_input = byte & 0x0f;
    }

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36/8 format]\n");
      exit (1);
    }

  return word;
}

static void
rewind_bin_word (FILE *f)
{
  have_leftover_input = 0;
  rewind (f);
}

static void
seek_bin_word (FILE *f, int position)
{
  rewind_bin_word (f);
  fseek (f, 9 * position / 2, SEEK_SET);
  if (position & 1)
    {
      leftover_input = get_byte (f) & 0x0f;
      have_leftover_input = 1;
    }
}

static void
write_bin_word (FILE *f, word_t word)
{
  if (have_leftover_output)
    {
      fputc (leftover_output | ((word >> 32) & 0x0f), f);
      fputc ((word >> 24) & 0xff, f);
      fputc ((word >> 16) & 0xff, f);
      fputc ((word >>  8) & 0xff, f);
      fputc ((word >>  0) & 0xff, f);
      have_leftover_output = 0;
    }
  else
    {
      fputc ((word >> 28) & 0xff, f);
      fputc ((word >> 20) & 0xff, f);
      fputc ((word >> 12) & 0xff, f);
      fputc ((word >>  4) & 0xff, f);
      have_leftover_output = 1;
      leftover_output = (word << 4) & 0xf0;
    }
}

static void
flush_bin_word (FILE *f)
{
  if (have_leftover_output)
    {
      fputc (leftover_output, f);
      have_leftover_output = 0;
    }
}

struct word_format bin_word_format = {
  "bin",
  get_bin_word,
  rewind_bin_word,
  seek_bin_word,
  write_bin_word,
  flush_bin_word
};
