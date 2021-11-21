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
#include <string.h>
#include <getopt.h>
#include "dis.h"

/* The binary output from CROSS is formatted into blocks, with a
header of three words describing its type, length, and address.  After
the data bytes comes a filler byte (or checksum, but it's always
zero).

An Atari DOS binary file is also formatted into blocks.  The file
begins with two FF bytes, and then each block header has start and end
address (inclusive). */

#define MEMORY_SIZE   65536

static unsigned char buf[4];
static int n = 0;
static int checksum;
static unsigned char memory[MEMORY_SIZE];
static void (*output) (int address, int length);
static int start = MEMORY_SIZE, end = 0;
static int image_start = -1, image_end = -1;

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
  int data;
  if (n == 0)
    refill (f);
  data = buf[--n];
  checksum += data;
  return data;
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

static void binary_block (FILE *f)
{
  int i, type, length, address;

  checksum = 0;

  do
    type = get_8 (f);
  while (type == 0);
  type |= get_8 (f) << 8;

  length = get_16 (f) - 6; /* Subtract header length. */
  address = get_16 (f);

  if (length == 0)
    exit (0);

  fprintf (stderr, "Type %d, length %d, address %04x\n",
	   type, length, address);

  for (i = 0; i < length; i++)
    memory[address + i] = get_8 (f);

  type = -checksum & 0xFF;
  if (type != get_8 (f))
    fprintf (stderr, "Bad checksum: %04X.\n", type);

  output (address, length);
}

static int get_hex (FILE *f)
{
  int c;

  c = fgetc (f);
  if (c == EOF)
    {
      fprintf (stderr, "Unexpected end of input file.\n");
      exit (1);
    }

  switch (c)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return c - '0';
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      return c - 'A' + 10;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      return c - 'a' + 10;
    default:
      fprintf (stderr, "Bad hex digit: %c\n", c);
      exit (1);
    }
}

static int get_a8 (FILE *f)
{
  int data = (get_hex (f) << 4) | get_hex (f);
  checksum += data;
  return data;
}

static int get_a16 (FILE *f)
{
  return (get_a8 (f) << 8) | get_a8 (f);
}

static void ascii_block (FILE *f)
{
  int length, address, data;
  int c, i;

  do
    {
      c = fgetc (f);
      if (c == EOF)
	exit (0);
    }
  while (c != ';');

  checksum = 0;
  length = get_a8 (f);
  address = get_a16 (f);

  if (length == 0)
    exit (0);

  for (i = 0; i < length; i++)
    memory[address + i] = get_a8 (f);

  checksum &= 0xFFFF;
  data = checksum;
  if (data != get_a16 (f))
    fprintf (stderr, "Bad checksum: %04X.\n", data);

  output (address, length);
}

static void atari_output (int address, int length)
{
  int i;
  out_16 (address);
  out_16 (address + length - 1);
  for (i = 0; i < length; i++)
    out_8 (memory[address + i]);
}

static void image_output (int address, int length)
{
  if (address < start)
    start = address;
  if (address + length > end)
    end = address + length;
}

static void image_write (void)
{
  int i;
  if (image_start != -1)
    start = image_start;
  if (image_end != -1)
    end = image_end;
  for (i = start; i < end; i++)
    out_8 (memory[i]);
  fprintf (stderr, "Memory image from %04X to %04X (%d bytes)\n",
	   start, end, end - start);
}

static void usage (const char *argv0)
{
  fprintf (stderr, "Usage: %s [-abAEIS -W<word format>]\n", argv0);
  usage_word_format ();
  exit (1);
}

int main (int argc, char **argv)
{
  input_word_format = &its_word_format;
  void (*block) (FILE *);
  int opt;

  memset (memory, 0, sizeof memory);
  block = binary_block;
  output = atari_output;

  while ((opt = getopt (argc, argv, "abAEISW:")) != -1)
    {
      switch (opt)
	{
	case 'a':
	  block = ascii_block;
	  break;
	case 'b':
	  block = binary_block;
	  break;
	case 'A':
	  out_16 (0xFFFF);
	  output = atari_output;
	  break;
	case 'I':
	  output = image_output;
	  atexit (image_write);
	  break;
	case 'S':
	  image_start = atoi (optarg);
	  break;
	case 'E':
	  image_end = atoi (optarg);
	  break;
	case 'W':
	  if (parse_input_word_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	}
    }

  for (;;)
    block (stdin);

  return 0;
}
