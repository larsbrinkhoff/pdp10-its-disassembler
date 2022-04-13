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

/* Convert text files like Saildart.org.
   This is not a data preserving transformation. */

#include <stdio.h>
#include <stdlib.h>
#include "libword.h"

static int leftover, there_is_some_leftover = 0;
static int carriage_return = 0;

#define WORDMASK	(0777777777777LL)
#define SIGNBIT		(0400000000000LL)

static int sail (int c)
{
  switch (c)
    {
    case 020623: return 0001; //↓
    case 001661: return 0002; //α
    case 001662: return 0003; //β
    case 021047: return 0004; //∧
    case 000254: return 0005; //¬
    case 001665: return 0006; //ε
    case 001700: return 0007; //π
    case 001673: return 0010; //λ
    case 021036: return 0016; //∞
    case 021002: return 0017; //∂
    case 021202: return 0020; //⊂
    case 021203: return 0021; //⊃
    case 021051: return 0022; //∩
    case 021052: return 0023; //∪
    case 021000: return 0024; //∀
    case 021003: return 0025; //∃
    case 021227: return 0026; //⊗
    case 020624: return 0027; //↔
    case 020622: return 0031; //→
    case 021140: return 0033; //≠
    case 021144: return 0034; //≤
    case 021145: return 0035; //≥
    case 021141: return 0036; //≡
    case 021050: return 0037; //∨
    case 020621: return 0136; //↑
    case 020620: return 0137; //←
    default:
      fprintf (stderr, "[illegal character]\n");
      exit (1);
    }
}

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
get_sail_word (FILE *f)
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

      if (byte == 012)
	{
	  word = insert (word, 015, &bits);
	  word = insert (word, 012, &bits);
	}
      else if (byte == 0137)
	{
	  word = insert (word, 030, &bits);
	}
      else if (byte == 0175)
	{
	  word = insert (word, 0176, &bits);
	}
      else if (byte == 0176)
	{
	  word = insert (word, 032, &bits);
	}
      else if (byte <= 0177)
	{
	  word = insert (word, byte, &bits);
	}
      else if (byte >= 0300 && byte <= 0337)
	{
	  int c = (byte & 037) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else if (byte >= 0340 && byte <= 0357)
	{
	  int c = (byte & 017) << 12;
	  c |= (get_byte (f) & 077) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else if (byte >= 0360 && byte <= 0367)
	{
	  int c = (byte & 7) << 18;
	  c |= (get_byte (f) & 077) << 12;
	  c |= (get_byte (f) & 077) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else
	{
	  fprintf (stderr, "[illegal UTF-8]\n");
	  exit (1);
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
rewind_sail_word (FILE *f)
{
  there_is_some_leftover = 0;
  rewind (f);
}

static void
write_utf8 (FILE *f, unsigned c)
{
  if (c >= 0x10000)
    {
      fputc ((c >> 18) | 0xE0, f);
      fputc (((c >> 12) & 0x3F) | 0x80, f);
      fputc (((c >> 6) & 0x3F) | 0x80, f);
      fputc ((c & 0x03F) | 0x80, f);
    }
  else if (c >= 0x800)
    {
      fputc ((c >> 12) | 0xE0, f);
      fputc (((c >> 6) & 0x3F) | 0x80, f);
      fputc ((c & 0x03F) | 0x80, f);
    }
  else if (c >= 0x80)
    {
      fputc ((c >> 6) | 0xC0, f);
      fputc ((c & 0x3F) | 0x80, f);
    }
  else
    fputc (c, f);
}

static int unsail (int c)
{
  switch (c)
    {
    case 0001: return 020623; //↓
    case 0002: return 001661; //α
    case 0003: return 001662; //β
    case 0004: return 021047; //∧
    case 0005: return 000254; //¬
    case 0006: return 001665; //ε
    case 0007: return 001700; //π
    case 0010: return 001673; //λ
    case 0016: return 021036; //∞
    case 0017: return 021002; //∂
    case 0020: return 021202; //⊂
    case 0021: return 021203; //⊃
    case 0022: return 021051; //∩
    case 0023: return 021052; //∪
    case 0024: return 021000; //∀
    case 0025: return 021003; //∃
    case 0026: return 021227; //⊗
    case 0027: return 020624; //↔
    case 0030: return 000137; //_
    case 0031: return 020622; //→
    case 0032: return 000176; //~
    case 0033: return 021140; //≠
    case 0034: return 021144; //≤
    case 0035: return 021145; //≥
    case 0036: return 021141; //≡
    case 0037: return 021050; //∨
    case 0136: return 020621; //↑
    case 0137: return 020620; //←
    case 0176: return 000175; //}
    default:   return c;
    }
}

static void
write_char (FILE *f, char c)
{
  if (carriage_return)
    {
      if (c == 012)
	fputc ('\n', f);
      else
	fputc (015, f);
    }
  switch (c)
    {
    case 0000: break;
    case 0012: if (!carriage_return) fputc (012, f); break;
    case 0015: carriage_return = 1; break;
    default:   write_utf8 (f, unsail (c)); break;
    }
  carriage_return = (c == 015);
}

static void
write_sail_word (FILE *f, word_t word)
{
  int i;

  if (word & 1)
    {
      fprintf (stderr, "ERROR: Word has bit 0 set.  The sail format only works for text.\n");
      exit (1);
    }

  for (i = 0; i < 5; i++)
    {
      write_char (f, (word >> 29) & 0177);
      word <<= 7;
    }
}

static void
flush_sail_word (FILE *f)
{
  if (carriage_return)
    fputc (015, f);
  carriage_return = 0;
}

struct word_format sail_word_format = {
  "sail",
  get_sail_word,
  rewind_sail_word,
  NULL,
  write_sail_word,
  flush_sail_word
};
