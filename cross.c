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

#include <stdio.h>
#include "dis.h"

/* The binary output from CROSS is formatted into blocks, with a
header of three words describing its type, length, and address.  After
the data bytes comes a filler byte (or checksum, but it's always
zero).

An Atari DOS binary file is also formatted into blocks.  The file
begins with two FF bytes, and then each block header has start and end
address (inclusive). */

static unsigned char buf[4];
static int n = 0;

static void refill (FILE *f)
{
  word_t word = get_word (f);
  if (word == -1)
    exit (0);

  buf[3] = (word >> 18) & 0377;
  buf[2] = (word >> 26) & 0377;
  buf[1] = (word >>  0) & 0377;
  buf[0] = (word >>  8) & 0377;
  n = 4;
}

static int get_8 (FILE *f)
{
  if (n == 0)
    refill (f);

  return buf[--n];
}

static int get_16 (FILE *f)
{
  int word;
  word = get_8 (f);
  word |= get_8 (f) << 8;
  return word;
}

static void out_8 (int word)
{
  putchar (word & 0xFF);
}

static void out_16 (int word)
{
  out_8 (word & 0xFF);
  out_8 (word >> 8);
}

static void block (FILE *f)
{
  int i, type, length, address;

  type = get_16 (f);
  length = get_16 (f);
  address = get_16 (f);

  fprintf (stderr, "Type %d, length %d, address %04x\n",
	   type, length, address);

  out_16 (address);
  out_16 (address + length - 1);

  for (i = 0; i < length; i++)
    out_8 (get_8 (f));

  get_8 (f);
}

int main (void)
{
  input_word_format = &its_word_format;
  
  out_16 (0xFFFF);

  for (;;)
    block (stdin);

  return 0;
}
