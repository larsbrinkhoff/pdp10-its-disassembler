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
#include "memory.h"

/* The binary output from CROSS is formatted into blocks, with a
header of three words describing its type, length, and address.  After
the data bytes comes a checksum byte. */

static unsigned char buf[4];
static int n = 0;
static int checksum;

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

static void
read_cross (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int i, data, length, address;
  word_t *core;

  (void)cpu_model;

  for (;;)
    {
      checksum = 0;

      do
        data = get_8 (f);
      while (data == 0);
      data |= get_8 (f) << 8;

      length = get_16 (f) - 6; /* Subtract header length. */
      address = get_16 (f);

      if (length == 0)
        return;

      core = malloc (length * sizeof (word_t));
      if (core == NULL)
        {
          fprintf (stderr, "Out of memory.\n");
          exit (1);
        }

      fprintf (stderr, "Type %d, length %d, address %04x\n",
               data, length, address);

      for (i = 0; i < length; i++)
        core[i] = get_8 (f);
      add_memory (memory, address, length, core);

      data = get_8 (f);
      if ((checksum & 0xFF) != 0)
        fprintf (stderr, "Bad checksum: %04X.\n", data);
    }
}

struct file_format cross_file_format = {
  "cross",
  read_cross,
  NULL
};
