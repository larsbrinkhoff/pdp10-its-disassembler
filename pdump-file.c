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

#define PAGE_ABS	(0400000000000LL)
#define PAGE_CBCPY	(0200000000000LL)
#define PAGE_SHARE	(0100000000000LL)
#define PAGE_WRITE	(0000000400000LL)
#define PAGE_READ	(0000000200000LL)
#define PAGE_NUM	(0000000000777LL)
#define PAGE_BITS	(PAGE_ABS & PAGE_CBCPY & PAGE_SHARE & \
			 PAGE_WRITE & PAGE_READ & PAGE_NUM)

static int read_page (word_t info)
{
  /* If the page map slot is zero, there is no page. */
  if (info == 0)
    return 0;

  /* If it's an absolute or shared page, it's not in the file. */
  if (info & (PAGE_ABS + PAGE_SHARE))
    return 0;

  /* If it's a readable or writable page, it's in the file. */
  return (info & (PAGE_READ + PAGE_WRITE)) != 0;
}

static void
read_pdump (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  word_t page_map[256];
  word_t word;
  int i, j;

  fprintf (output_file, "PDUMP format\n\n");

  /* zero word */
  word = get_word (f);

  fprintf (output_file, "Page map:\n");
  fprintf (output_file, "Page  Address  Page description\n");
  for (i = 0; i < 256; i++)
    {
      word = get_word (f);
      page_map[i] = word;

      if (word != 0)
	{
	  fprintf (output_file, "%03o   %06o   %06o,,%06o  ",
		  i, ITS_PAGESIZE * i, (int)(word >> 18), (int)word & 0777777);

	  fprintf (output_file, word & PAGE_ABS ? "a" : "-");
	  fprintf (output_file, word & PAGE_CBCPY ? "c" : "-");
	  fprintf (output_file, word & PAGE_SHARE ? "s" : "-");
	  fprintf (output_file, word & PAGE_WRITE ? "w" : "-");
	  fprintf (output_file, word & PAGE_READ ? "r" : "-");
	  if (word & PAGE_NUM)
	    fprintf (output_file, " %03o", (int)(word & PAGE_NUM));

	  fprintf (output_file, "\n");
	}
    }

  /* the rest of the page is unused */
  for (i = 0; i < ITS_PAGESIZE - 257; i++)
    {
      get_word (f);
    }

  for (i = 0; i < 256; i++)
    {
      word_t *data, *ptr;

      if (!read_page(page_map[i]))
	continue;

      data = malloc (ITS_PAGESIZE * sizeof *data);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      ptr = data;
      for (j = 0; j < ITS_PAGESIZE; j++)
	{
	  *ptr++ = get_word (f);
	}

      add_memory (memory, ITS_PAGESIZE * i, ITS_PAGESIZE, data);
      if ((page_map[i] & PAGE_WRITE) == 0)
	purify_memory (memory, ITS_PAGESIZE * i, ITS_PAGESIZE);
    }

  fprintf (output_file, "\n");
  word = get_word (f);
  start_instruction = word;
  sblk_info (f, word, cpu_model);
}

struct file_format pdump_file_format = {
  "pdump",
  read_pdump,
  NULL
};
