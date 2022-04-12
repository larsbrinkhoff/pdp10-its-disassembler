/* Copyright (C) 2018, 2022 Lars Brinkhoff <lars@nocrew.org>

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

/* This implements the ANSI/ASCII format.

   A 36-bit word AAAAAAABBBBBBBCCCCCCCDDDDDDDEEEEEEEF is stored as five
   octets.  X means written as zero, ignored when read.

   XAAAAAAA
   XBBBBBBB
   XCCCCCCC
   XDDDDDDD
   FEEEEEEE */

#include <stdio.h>

#include "libword.h"

static word_t output = -1;

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_aa_word (FILE *f)
{
  word_t word = 0;
  word_t x;

  if (feof (f))
    return -1;

  x = get_byte (f); word += (x & 0177) << 29;
  if (feof (f))
    return -1;
  x = get_byte (f); word += (x & 0177) << 22;
  x = get_byte (f); word += (x & 0177) << 15;
  x = get_byte (f); word += (x & 0177) <<  8;
  x = get_byte (f); word += (x & 0177) <<  1;
                    word += (x & 0200) >>  7;

  return word;
}

static void
write_aa_word (FILE *f, word_t word)
{
  if (output != -1)
    {
      fputc ((output >> 29) & 0177, f);
      fputc ((output >> 22) & 0177, f);
      fputc ((output >> 15) & 0177, f);
      fputc ((output >>  8) & 0177, f);
      fputc (((output >> 1) & 0177) +
	     ((output << 7) & 0200), f);
    }

  output = word;
}

static void
flush_aa_word (FILE *f)
{
  int i, c;
  if (output == -1)
    return;
  fputc ((output >> 29) & 0177, f);
  for (i = 0; i < 4; i++)
    {
      output &= 03777777777LL;
      if (output == 0)
	break;
      c = (output >> 22) & 0177;
      if (i == 3 && (output & 010000000LL) != 0)
	c |= 0200;
      fputc (c, f);
      output <<= 7;
    }
  output = -1;
}

struct word_format aa_word_format = {
  "ascii",
  get_aa_word,
  NULL,
  by_five_octets,
  write_aa_word,
  flush_aa_word
};
