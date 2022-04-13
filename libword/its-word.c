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
#include <stdlib.h>
#include "libword.h"

static int leftover, there_is_some_leftover = 0;

#define WORDMASK	(0777777777777LL)
#define SIGNBIT		(0400000000000LL)

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static inline word_t
insert (word_t word, unsigned char byte, int *bits)
{
  *bits += 7;
  return (word << 7) | byte;
}

static word_t
get_its_word (FILE *f)
{
  unsigned char byte;
  word_t word;
  int bits;

  if (feof (f))
    return -1;

  word = 0;
  bits = 0;

  if (there_is_some_leftover)
    {
      word = leftover;
      bits = 7;
      there_is_some_leftover = 0;
    }

  while (bits < 36)
    {
      byte = get_byte (f);
      if (feof (f))
	{
	  if (bits == 0)
	    return -1;
	}

      if (byte <= 011 ||
	  (byte >= 013 && byte <= 014) ||
	  (byte >= 016 && byte <= 0176))
	{
	  word = insert (word, byte, &bits);
	}
      else if (byte == 012)
	{
	  word = insert (word, 015, &bits);
	  word = insert (word, 012, &bits);
	}
      else if (byte == 015)
	{
	  word = insert (word, 012, &bits);
	}
      else if (byte == 0177)
	{
	  word = insert (word, 0177, &bits);
	  word = insert (word,    7, &bits);
	}
      else if ((byte >= 0200 && byte <= 0206) ||
	       (byte >= 0210 && byte <= 0211) ||
	       (byte >= 0213 && byte <= 0214) ||
	       (byte >= 0216 && byte <= 0355))
	{
	  word = insert (word, 0177,        &bits);
	  word = insert (word, byte - 0200, &bits);
	}
      else if (byte == 0207)
	{
	  word = insert (word, 0177, &bits);
	  word = insert (word, 0177, &bits);
	}
      else if (byte == 0212)
	{
	  word = insert (word, 0177, &bits);
	  word = insert (word,  015, &bits);
	}
      else if (byte == 0215)
	{
	  word = insert (word, 0177, &bits);
	  word = insert (word,  012, &bits);
	}
      else if (byte == 0356)
	{
	  word = insert (word, 015, &bits);
	}
      else if (byte == 0357)
	{
	  word = insert (word, 0177, &bits);
	}
      else if (byte >= 0360)
	{
	  if (bits != 0)
	    {
	      fprintf (stderr, "[error in 36-bit file format]\n");
	      exit (1);
	    }
	  word = byte & 017;
	  word = (word << 8) | get_byte (f);
	  word = (word << 8) | get_byte (f);
	  word = (word << 8) | get_byte (f);
	  word = (word << 8) | get_byte (f);
	  bits = 36;
	}

      if (bits == 35)
	{
	  word <<= 1;
	  bits++;
	}
      else if (bits == 42)
	{
	  leftover = word & 0177;
	  there_is_some_leftover = 1;
	  word >>= 7;
	  word <<= 1;
	}
    }

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36-bit file format (word too large)]\n");
      exit (1);
    }

  return word;
}

static void
rewind_its_word (FILE *f)
{
  there_is_some_leftover = 0;
  rewind (f);
}

static void
fputc2 (int c1, int c2, FILE *f)
{
  fputc (c1, f);
  fputc (c2, f);
}

static int previous_octet = -1;

static void
flush_its_word (FILE *f)
{
  if (previous_octet == 015)
    fputc (0356, f);
  else if (previous_octet == 0177)
    fputc (0357, f);
  previous_octet = -1;
}

static void
binary_word (FILE *f, word_t word)
{
  flush_its_word (f);

  fputc (((word >> 32) &  017) + 0360, f);
  fputc (((word >> 24) & 0377), f);
  fputc (((word >> 16) & 0377), f);
  fputc (((word >>  8) & 0377), f);
  fputc (( word        & 0377), f);
}

static void
ascii_word (FILE *f, word_t word)
{
  char c, octets[5];
  int i;

  octets[0] = (word >> 29) & 0177;
  octets[1] = (word >> 22) & 0177;
  octets[2] = (word >> 15) & 0177;
  octets[3] = (word >>  8) & 0177;
  octets[4] = (word >>  1) & 0177;

  for (i = 0; i < 5; i++)
    {
      c = octets[i];

      if (previous_octet == 015)
	{
	  if (c == 012)
	    fputc (012, f);
	  else if (c == 015)
	    fputc2 (0356, 0356, f);
	  else if (c == 0177)
	    fputc2 (0356, 0357, f);
	  else
	    fputc2 (0356, c, f);
	  previous_octet = -1;
	}
      else if (previous_octet == 0177)
	{
	  switch (c)
	    {
	    case 0007: fputc (0177, f); break;
	    case 0012: fputc (0215, f); break;
	    case 0015: fputc (0212, f); break;
	    case 0177: fputc (0207, f); break;
	    default:
	      if (c < 0156)
		fputc (c + 0200, f);
	      else if (c == 012)
		fputc2 (0357, 015, f);
	      else if (c == 015)
		fputc2 (0357, 0356, f);
	      else if (c == 0177)
		fputc2 (0357, 0357, f);
	      else
		fputc2 (0357, c, f);
	      break;
	    }
	  previous_octet = -1;
	}
      else if (c == 015 || c == 0177)
	previous_octet = c;
      else if (c == 012)
	fputc (015, f);
      else
	fputc (c, f);
    }
}

static void
write_its_word (FILE *f, word_t word)
{
  if (word & 1)
    binary_word (f, word);
  else
    ascii_word (f, word);
}

struct word_format its_word_format = {
  "its",
  get_its_word,
  rewind_its_word,
  NULL,
  write_its_word,
  flush_its_word
};
