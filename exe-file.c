/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2020 Adam Sampson <ats@offog.org>

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

/* This can be larger, but we only handle a single section at present */
#define FILE_MAX_PAGES ((1 << 18) / DEC_PAGESIZE)

static void
read_exe (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int position = 0;
  word_t word;
  word_t entry_vec_addr = -1, entry_vec_len = -1;
  word_t file_map[FILE_MAX_PAGES];
  int i, j;

  for (i = 0; i < FILE_MAX_PAGES; i++)
    file_map[i] = -1;

  fprintf (output_file, "DEC sharable format\n\n");

  /* Read the directory */
  for (;;)
    {
      word_t block_type, block_len;

      word = get_word (f);
      position++;

      block_type = word >> 18;
      block_len = word & 0777777;

      switch (block_type)
	{
	case 01776: /* directory block */
	  fprintf (output_file, "Directory:\n");
	  fprintf (output_file, "Prot  File page  Memory page  Count\n");
	  for (i = 1; i < block_len; i += 2)
	    {
	      word_t access_bits, file_page, mem_page, count;

	      word = get_word (f);
	      position++;

	      access_bits = (word >> 27);
	      file_page = word & ((1 << 27) - 1);

	      word = get_word (f);
	      position++;

	      mem_page = word & ((1 << 27) - 1);
	      count = (word >> 27) + 1;

	      fprintf (output_file, "%03llo   ", access_bits);
	      if (file_page == 0)
		fprintf (output_file, "none       ");
	      else
		fprintf (output_file, "%09llo  ", file_page);
	      fprintf (output_file, "%09llo    %llo\n", mem_page, count);

	      if (file_page != 0)
		{
		  for (j = 0; j < count; j++)
		    {
		      if (file_page + j < FILE_MAX_PAGES)
			file_map[file_page + j] = mem_page + j;
		      else
			fprintf (output_file, "  (too many pages; not loaded)\n");
		    }
		}
	    }
	  fprintf (output_file, "\n");
	  break;

	case 01775: /* entry vector block */
	  entry_vec_len = get_word (f);
	  position++;

	  entry_vec_addr = get_word (f);
	  position++;

	  break;

	case 01777: /* end block */
	  goto enddir;

	default:
	  fprintf (output_file, "Unknown block type %06llo\n\n", block_type);
	  /* fall through */

	case 01774: /* PDV block - ignore */
	  for (i = 1; i < block_len; i++)
	    {
	      get_word (f);
	      position++;
	    }
	  break;
	}
    }
 enddir:

  /* Skip to the end of the first page */
  while (position < DEC_PAGESIZE)
    {
      get_word (f);
      position++;
    }

  /* Map the remaining pages into memory */
  for (;;)
    {
      word_t *data;
      word_t page = file_map[position / DEC_PAGESIZE];

      data = malloc (DEC_PAGESIZE * sizeof *data);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      for (i = 0; i < DEC_PAGESIZE; i++)
	{
	  data[i] = get_word (f);
	  if (data[i] == -1)
	    {
	      free (data);
	      goto endfile;
	    }
	  position++;
	}

      if (page != -1)
	add_memory (memory, page * DEC_PAGESIZE, DEC_PAGESIZE, data);
      else
	free (data);
    }
 endfile:

  dec_info (memory, entry_vec_len, entry_vec_addr, cpu_model);
}

struct file_format exe_file_format = {
  "exe",
  read_exe,
  NULL
};
