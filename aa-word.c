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

/* This implements the ANSI/ASCII format.

   A 36-bit word AAAAAAABBBBBBBCCCCCCCDDDDDDDEEEEEEEF is stored as five
   octets.  X means written as zero, ignored when read.

   XAAAAAAA
   XBBBBBBB
   XCCCCCCC
   XDDDDDDD
   FEEEEEEE */

#include <stdio.h>

#include "dis.h"

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

word_t
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

void
rewind_aa_word (FILE *f)
{
  rewind (f);
}

void
write_aa_word (FILE *f, word_t word)
{
  fputc ((word >> 29) & 0177, f);
  fputc ((word >> 22) & 0177, f);
  fputc ((word >> 15) & 0177, f);
  fputc ((word >>  8) & 0177, f);
  fputc (((word >> 1) & 0177) +
	 ((word << 7) & 0200), f);
}

static void
flush_aa_word (FILE *f)
{
  (void)f;
}
