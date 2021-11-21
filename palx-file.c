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

static int checksum;

static int
get_8 (FILE *f)
{
  word_t word = get_word (f);
  if (word == -1)
    {
      fprintf (stderr, "Unexpected end of PALX file.\n");
      exit (1);
    }
  word &= 0377;
  checksum += word;
  return word;
}

static int
get_16 (FILE *f)
{
  return get_8 (f) | (get_8 (f) << 8);
}

static void
read_palx (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int i, data, length, address;
  word_t *core;

  (void)cpu_model;
  start_instruction = 1;

  for (;;)
    {
      checksum = 0;
      do
        data = get_8 (f);
      while (data == 0);
      data |= get_8 (f) << 8;

      if (data != 1)
        {
          fprintf (stderr, "Error looking for start of PALX block.\n");
          exit (1);
        }

      length = get_16 (f);
      address = get_16 (f);

      length -= 6;
      if (length == 0)
        {
          if ((address & 1) == 0)
            start_instruction = address;
          return;
        }

      core = malloc (length * sizeof (word_t));
      if (core == NULL)
        {
          fprintf (stderr, "Out of memory.\n");
          exit (1);
        }

      for (i = 0; i < length; i++)
        core[i] = get_8 (f);
      add_memory (memory, address, length, core);

      data = get_8 (f);
      if ((checksum & 0xFF) != 0)
        fprintf (stderr, "Bad checksum %02X\n", data);

    }
}

struct file_format palx_file_format = {
  "palx",
  read_palx,
  NULL
};
