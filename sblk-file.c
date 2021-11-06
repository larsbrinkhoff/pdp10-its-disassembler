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

#include "dis.h"
#include "memory.h"
#include "symbols.h"

#define SYHKL       0400000000000
#define SYKIL       0200000000000
#define SYLCL       0100000000000
#define SYGBL       0040000000000

static void
read_sblk (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int address;
  word_t word;
  int i;

  int block_length, block_address;

  fprintf (output_file, "SBLK format\n");

  address = 0;
  while ((word = get_word (f)) != JRST_1)
    {
      address++;
      if (address > 100)
	{
	  fprintf (stderr, "JRST 1 instruction not found in the first "
		   "100 words\n");
	  exit (1);
	}
    }
      
  while ((word = get_word (f)) & SIGNBIT)
    {
      word_t *data, *ptr;

      reset_checksum (word);
      block_length = -((word >> 18) | ((-1) & ~0777777));
      block_address = word & 0777777;

      data = malloc (block_length * sizeof *data);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      ptr = data;
      for (i = 0; i < block_length; i++)
	{
	  *ptr++ = get_checksummed_word (f);
	}

      add_memory (memory, block_address, block_length, data);

      word = get_word (f);
      check_checksum (word);
    }

  fprintf (output_file, "\n");
  start_instruction = word;
  sblk_info (f, word, cpu_model);
}

static void
write_block (FILE *f, struct pdp10_memory *memory, int start, int end)
{
  word_t word, cksum;
  int i, length;

  length = end - start;
  word = -length << 18;
  word |= start;
  word &= WORDMASK;
  write_word (f, word);

  cksum = word;
  for (i = start; i < end; i++)
    {
      cksum = (cksum << 1) | (cksum >> 35);
      word = get_word_at (memory, i);
      cksum += word;
      cksum &= WORDMASK;
      write_word (f, word);
    }

  write_word (f, cksum);
}

void
write_sblk_core (FILE *f, struct pdp10_memory *memory)
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
	  write_block (f, memory, start, start + n);
	  start += n;
	  length -= n;
	}
    }
}

void
write_sblk_symbols (FILE *f)
{
  word_t word, cksum;
  int i, length;

  length = 2 * num_symbols;
  word = length;
  word = (-word) << 18;
  word &= WORDMASK;
  write_word (f, word);

  cksum = word;
  for (i = 0; i < num_symbols; i++)
    {
      cksum = (cksum << 1) | (cksum >> 35);
      word = ascii_to_sixbit (symbols[i].name);
      if (symbols[i].flags & SYMBOL_KILLED)
	word |= SYKIL;
      if (symbols[i].flags & SYMBOL_HALFKILLED)
	word |= SYHKL;
      if (symbols[i].flags & SYMBOL_GLOBAL)
	word |= SYGBL;
      else
	word |= SYLCL;
      cksum += word;
      cksum &= WORDMASK;
      write_word (f, word);

      cksum = (cksum << 1) | (cksum >> 35);
      word = symbols[i].value;
      cksum += word;
      cksum &= WORDMASK;
      write_word (f, word);
    }

  write_word (f, cksum);
}

static void
write_sblk (FILE *f, struct pdp10_memory *memory)
{
  write_word (f, JRST_1);
  write_sblk_core (f, memory);
  write_word (f, start_instruction);
  write_sblk_symbols (f);
  write_word (f, start_instruction);
}

struct file_format sblk_file_format = {
  "sblk",
  read_sblk,
  write_sblk
};
