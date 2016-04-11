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

void
dis_sblk (FILE *f, int cpu_model)
{
  int address;
  word_t word;
  int i;

  int block_length, block_address;

  printf ("SLBK format\n\n");

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
      block_length = -((word >> 18) | ((-1) & ~0777777));
      block_address = word & 0777777;

      for (i = 0; i < block_length; i++)
	{
	  disassemble_word (NULL, get_word (f),
			    block_address + i, cpu_model);
	}

      word = get_word (f);
      printf ("checksum: %012llo\n", word);
    }

  printf ("\n");
  sblk_info (f, word, cpu_model);
}

void
read_sblk (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int address;
  word_t word;
  int i;

  int block_length, block_address;

  printf ("SLBK format\n");

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
      char *data, *ptr;

      block_length = -((word >> 18) | ((-1) & ~0777777));
      block_address = word & 0777777;

      data = malloc (5 * block_length);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      ptr = data;
      for (i = 0; i < block_length; i++)
	{
	  word = get_word (f);
	  *ptr++ = (word >> 32) & 0x0f;
	  *ptr++ = (word >> 24) & 0xff;
	  *ptr++ = (word >> 16) & 0xff;
	  *ptr++ = (word >>  8) & 0xff;
	  *ptr++ = (word >>  0) & 0xff;
	}

      add_memory (memory, block_address, block_length, data);

      word = get_word (f);
      /*printf ("checksum: %012llo\n", word);*/
    }

  printf ("\n");
  sblk_info (f, word, cpu_model);
}
