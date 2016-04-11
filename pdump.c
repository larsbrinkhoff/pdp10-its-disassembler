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

void
dis_pdump (FILE *f, int cpu_model)
{
  word_t page_map[256];
  int page;
  word_t word;
  int pages;
  int i, j;

  printf ("PDUMP format\n\n");

  /* zero word */
  word = get_word (f);

  printf ("Page map:\n");
  pages = 0;
  for (i = 0; i < 256; i++)
    {
      word = get_word (f);
      page_map[i] = word;

      if (word != 0)
	{
	  pages++;

	  printf ("%06o  %012llo  ", i, word);

	  printf (word & PAGE_ABS ? "a" : "-");
	  printf (word & PAGE_CBCPY ? "c" : "-");
	  printf (word & PAGE_SHARE ? "s" : "-");
	  printf (word & PAGE_WRITE ? "w" : "-");
	  printf (word & PAGE_READ ? "r" : "-");
	  if (word & PAGE_NUM)
	    printf (" %03o", (int)(word & PAGE_NUM));

	  printf ("\n");
	}
    }

  /* the rest of the page is unused */
  for (i = 0; i < ITS_PAGESIZE - 257; i++)
    {
      get_word (f);
    }

  printf ("\n");

  page = 0;
  for (i = 0; i < pages; i++)
    {
      while (page_map[page] == 0 && page < 256)
	page++;

      printf ("Page %03o, #%d/%d:\n", page, i, pages);
      for (j = 0; j < ITS_PAGESIZE; j++)
	{
	  word = get_word (f);
	  disassemble_word (NULL, word, ITS_PAGESIZE * page + j, cpu_model);
	}

      page++;
    }

  printf ("\n");
  word = get_word (f);
  sblk_info (f, word, cpu_model);
}

void
read_pdump (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  word_t page_map[256];
  int page;
  word_t word;
  int pages;
  int i, j;

  printf ("PDUMP format\n\n");

  /* zero word */
  word = get_word (f);

  printf ("Page map:\n");
  printf ("Page  Address  Page description\n");
  pages = 0;
  for (i = 0; i < 256; i++)
    {
      word = get_word (f);
      page_map[i] = word;

      if (word != 0)
	{
	  pages++;

	  printf ("%03o   %06o   %06o,,%06o  ",
		  i, ITS_PAGESIZE * i, (int)(word >> 18), (int)word & 0777777);

	  printf (word & PAGE_ABS ? "a" : "-");
	  printf (word & PAGE_CBCPY ? "c" : "-");
	  printf (word & PAGE_SHARE ? "s" : "-");
	  printf (word & PAGE_WRITE ? "w" : "-");
	  printf (word & PAGE_READ ? "r" : "-");
	  if (word & PAGE_NUM)
	    printf (" %03o", (int)(word & PAGE_NUM));

	  printf ("\n");
	}
    }

  /* the rest of the page is unused */
  for (i = 0; i < ITS_PAGESIZE - 257; i++)
    {
      get_word (f);
    }

  page = 0;
  for (i = 0; i < pages; i++)
    {
      char *data, *ptr;

      data = malloc (5 * ITS_PAGESIZE);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      while (page_map[page] == 0 && page < 256)
	page++;

      ptr = data;
      for (j = 0; j < ITS_PAGESIZE; j++)
	{
	  word = get_word (f);
	  *ptr++ = (word >> 32) & 0x0f;
	  *ptr++ = (word >> 24) & 0xff;
	  *ptr++ = (word >> 16) & 0xff;
	  *ptr++ = (word >>  8) & 0xff;
	  *ptr++ = (word >>  0) & 0xff;
	}

      add_memory (memory, ITS_PAGESIZE * page, ITS_PAGESIZE, data);
      page++;
    }

  printf ("\n");
  word = get_word (f);
  sblk_info (f, word, cpu_model);
}
