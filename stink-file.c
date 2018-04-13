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

static void
read_stink (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int address;
  word_t word;

  printf ("Stinking REL format\n");

  address = 0;
  while ((word = get_word (f)) != -1)
    {
      word_t *data = malloc (5);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      *data = word;
      add_memory (memory, address++, 1, data);
    }
}

struct file_format stink_file_format = {
  "stink",
  read_stink,
  NULL
};
