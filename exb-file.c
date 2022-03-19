/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

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

/* This file format will read in a paper tape with a hardware read-in
   loader.  To do this, it will read in the loader and run it by
   emulating a tiny subset of PDP-10 instructions.  The subset is enough to
   support the ITS RIM10 and DEC RIM10B loaders. */

#include <stdio.h>
#include <stdlib.h>

#include "dis.h"
#include "memory.h"

static int
read_8 (FILE *f)
{
  static int i = 0;
  static word_t word;

  if (i == 0)
    {
      word = get_word (f);
      if (word == -1)
	return -1;
      i = 4;
    }

  i--;
  switch (i)
    {
    case 0: return (word >>  8) & 0377;
    case 1: return (word >>  0) & 0377;
    case 2: return (word >> 26) & 0377;
    case 3: return (word >> 18) & 0377;
    }

  return -1;
}

static int
read_16 (FILE *f)
{
  int x1, x2;
  x1 = read_8 (f);
  if (x1 == -1)
    return 0;
  x2 = read_8 (f);
  if (x2 == -1)
    x2 = 0;
  return x1 | (x2 << 8);
}

static word_t
read_32 (FILE *f)
{
  word_t x1, x2;
  x1 = read_16 (f);
  if (feof (f))
    return 0;
  x2 = read_16 (f);
  if (feof (f))
    x2 = 0;
  return x1 | (x2 << 16);
}

static word_t
read_36 (FILE *f)
{
  word_t x1, x2;
  x1 = read_32 (f);
  if (feof (f))
    return 0;
  x2 = read_8 (f);
  if (feof (f))
    x2 = 0;
  return x1 | (x2 << 32);
}

static void
read_exb (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int i, address, length;
  word_t *core;
  word_t word;

  (void)cpu_model;

  fprintf (output_file, "EXB format\n");

  for (;;)
    {
      length = read_16 (f);
      address = read_32 (f);

      if (feof (f) || length < 4)
	{
	  fprintf (stderr, "Unexpected end of file.\n");
	  exit (1);
	}

      if (length == 4)
	{
	  fprintf (output_file, "Start address: %06o\n", address);
	  start_instruction = JRST + address;
	  return;
	}

      length -= 4;
      length /= 5;

      core = malloc (length * sizeof (word_t));
      if (core == NULL)
	{
	  fprintf (stderr, "Out of memory.\n");
	  exit (1);
	}

      for (i = 0; i < length; i++)
	{
	  word = read_36 (f);
	  if (feof (f))
	    {
	      fprintf (stderr, "Unexpected end of file.\n");
	      exit (1);
	    }
	  core[i] = word;
	}

      if (length & 1)
	read_8 (f);

      add_memory (memory, address, length, core);
    }
}

static void
write_8 (FILE *f, int data)
{
  static word_t word = 0;
  static int i = 0;

  data &= 0377;
  switch (i)
    {
    case 0: word |= data << 18; break;
    case 1: word |= data << 26; break;
    case 2: word |= data <<  0; break;
    case 3: word |= data <<  8; break;
    }

  i++;
  if (i == 4)
    {
      write_word (f, word);
      word = 0;
      i = 0;
    }
}

static void
write_16 (FILE *f, int data)
{
  write_8 (f, data);
  write_8 (f, data >> 8);
}

static void
write_32 (FILE *f, int data)
{
  write_16 (f, data);
  write_16 (f, data >> 16);
}

static void
write_36 (FILE *f, word_t data)
{
  write_32 (f, data);
  write_8 (f, data >> 32);
}

static void
write_block (FILE *f, struct pdp10_memory *memory, int address, int length)
{
  int i;

  write_16 (f, 5*length + 4);
  write_32 (f, address);

  for (i = address; i < address + length; i++)
    write_36 (f, get_word_at (memory, i));

  if (length & 1)
    write_8 (f, 0);
}

static void
write_exb (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i;

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      write_block (f, memory, start, length);
    }

  start = start_instruction & 0777777;
  if (start)
    write_block (f, memory, start, 0);
}

struct file_format exb_file_format = {
  "exb",
  read_exb,
  write_exb
};
