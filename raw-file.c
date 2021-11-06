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

#include <stdio.h>
#include <stdlib.h>

#include "dis.h"
#include "memory.h"

void
read_raw_at (FILE *f, struct pdp10_memory *memory, int address)
{
  word_t word;

  while ((word = get_word (f)) != -1)
    {
      word_t *data = malloc (sizeof *data);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      data[0] = word;

      add_memory (memory, address++, 1, data);
    }
}

static void
read_raw (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  fprintf (output_file, "Raw format\n");

  read_raw_at (f, memory, 0);
}

void
write_raw_at (FILE *f, struct pdp10_memory *memory, int address)
{
  int i, end = memory->area[memory->areas-1].end;
  word_t word;

  for (i = address; i < end; i++)
    {
      word = get_word_at (memory, i);
      if (word == -1)
	word = 0;
      write_word (f, word);
    }

  flush_word (f);
}

static void
write_raw (FILE *f, struct pdp10_memory *memory)
{
  write_raw_at (f, memory, 0);
}

struct file_format raw_file_format = {
  "raw",
  read_raw,
  write_raw
};
