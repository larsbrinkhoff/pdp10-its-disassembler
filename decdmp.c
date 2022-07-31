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
#include <unistd.h>
#include <string.h>

#include "dis.h"
#include "memory.h"

#define FBLOCK 04	/* First block to write on tape. */
#define LBLOCK 01077	/* Maximum last block to write. */
#define FIRST  010	/* First location to dump-1. */

#define TAPE_BLOCKS      01102
#define BLOCK_WORDS      128

static word_t image[TAPE_BLOCKS * BLOCK_WORDS];
static int verbose;

static word_t *
get_block (int block)
{
  return &image[block * BLOCK_WORDS];
}

static void
write_block (FILE *f, int n, int size)
{
  int i;
  word_t *x = get_block (n);
  for (i = 0; i < size; i++)
    write_word (f, *x++);
}

static void
load_file (FILE *f, struct pdp10_memory *memory)
{
  init_memory (memory);
  input_file_format->read (f, memory, 0);
  rewind_word (f);
}

static void
copy_file (char *name)
{
  struct pdp10_memory memory;
  word_t word;
  FILE *f;
  int i, end;

  f = fopen (name, "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening file %s.\n", name);
      exit (1);
    }

  load_file (f, &memory);
  end = memory.area[memory.areas-1].end;
  if (end > BLOCK_WORDS * ((LBLOCK - FBLOCK) + 1))
    {
      fprintf (stderr, "Program too large: %o\n", end);
      exit (1);
    }

  for (i = FIRST + 1; i < end; i++)
    {
      word = get_word_at (&memory, i);
      if (word == -1)
	word = 0;
      image[BLOCK_WORDS * FBLOCK + i - FIRST - 1] = word;
    }

  fclose (f);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s [-v] [-W<word format>]  [-F<file format>] <tape> <file>\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  int i;
  FILE *f;
  int opt;

  input_file_format = &csave_file_format;
  input_word_format = &aa_word_format;
  output_word_format = &dta_word_format;
  verbose = 0;

  output_file = fopen ("/dev/null", "w");

  while ((opt = getopt (argc, argv, "vF:W:")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  verbose++;
	  break;
	case 'W':
	  if (parse_input_word_format (optarg))
	    usage (argv[0]);
	  break;
	case 'F':
	  if (parse_input_file_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	  break;
	}
    }

  if (optind + 2 != argc)
    usage (argv[0]);

  f = fopen (argv[optind], "wb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening tape image file %s\n", argv[optind]);
      exit (1);
    }

  memset (image, 0, sizeof image);

  copy_file (argv[optind+1]);

  for (i = 0; i < TAPE_BLOCKS; i++)
    write_block (f, i, BLOCK_WORDS);
  fclose (f);

  return 0;
}
