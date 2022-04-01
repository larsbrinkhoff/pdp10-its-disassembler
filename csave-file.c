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

#include <stdio.h>
#include <stdlib.h>

#include "dis.h"
#include "memory.h"
#include "symbols.h"

static void
read_csave (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int address, length;
  word_t word;
  int i;

  fprintf (output_file, "Nonsharable/compressed SAVE format\n");

  while ((word = get_word (f)) & SIGNBIT)
    {
      word_t *data, *ptr;

      length = 01000000 - ((word >> 18) & 0777777);
      address = (word & 0777777) + 1;

      data = malloc (length * sizeof (word_t));
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      ptr = data;
      for (i = 0; i < length; i++)
	*ptr++ = get_word (f);
      add_memory (memory, address, length, data);
    }

  length = (word >> 18) & 0777777;
  address = word & 0777777;

  fprintf (output_file, "\n");
  dec_info (memory, length, address, cpu_model);
}

void
write_dec_symbols (struct pdp10_memory *memory)
{
}

static void
write_block (FILE *f, struct pdp10_memory *memory, int address, int length)
{
  word_t word;
  word = -length << 18;
  word |= address - 1;
  word &= WORDMASK;
  write_word (f, word);

  while (length-- > 0)
    write_word (f, get_word_at (memory, address++));
}

static void
write_core (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, n;

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      while (length > 0)
	{
	  n = length > 512 ? 512 : length;
	  write_block (f, memory, start, n);
	  start += n;
	  length -= n;
	}
    }
}

static void
write_csave (FILE *f, struct pdp10_memory *memory)
{
  write_dec_symbols (memory);
  write_core (f, memory);
  write_word (f, JRST + (start_instruction & 0777777));
}

struct file_format csave_file_format = {
  "csave",
  read_csave,
  write_csave
};

/* Alias for csave. */
struct file_format osave_file_format = {
  "osave",
  read_csave,
  write_csave
};
