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
#include <string.h>

#include "dis.h"

#define TAPE_FILES 027
#define TAPE_BLOCKS 01100
#define BLOCK_WORDS 128
#define DIRECTORY_BLOCK 0100

word_t image[(TAPE_BLOCKS + 4) * BLOCK_WORDS];
int blocks;
int extract;
int verbose;

int mode[TAPE_FILES];
int extension[TAPE_FILES];
int block_area[TAPE_BLOCKS + 1];

/* Mode 0 = ASCII, written by TECO.
 * Mode 1 = DUMP, written by MACDMP.
 * Mode 2 = SBLK, written by MIDAS.
 * Mode 3 = RELOC, written by MIDAS. */

char *type = " !\"#";

static word_t *
get_block (int block)
{
  return &image[block * BLOCK_WORDS];
}

static word_t *
get_dir (int i)
{
  return get_block (DIRECTORY_BLOCK) + 2 * i;
}

static int read_block (FILE *f, word_t *buffer)
{
  int i;

  for (i = 0; i < BLOCK_WORDS; i++)
    {
      buffer[i] = get_word (f);
      if (buffer[i] == -1)
	return -1;
    }

  return 0;
}

static void
process (void)
{
  word_t *dir;
  int i, j;

  memset (extension, 0, sizeof extension);

  for (i = 0; i < TAPE_FILES; i++)
    {
      if (get_dir (i)[0] == 0)
	{
	  int x = get_dir (i)[1];
	  if (x > 0 && x <= TAPE_FILES)
	    extension[x] = i+1;
	}
    }

  memset (mode, 0, sizeof mode);

  dir = get_block (DIRECTORY_BLOCK) + 2*TAPE_FILES;
  for (i = 0; i < TAPE_FILES; i++)
    {
      if (*dir & 1)
	mode[i] |= 1;
      dir++;
    }
  for (i = 0; i < TAPE_FILES; i++)
    {
      if (*dir & 1)
	mode[i] |= 2;
      dir++;
    }

  dir = get_block (DIRECTORY_BLOCK);
  for (i = 0; i < 128 - 2*TAPE_FILES; i++)
    {
      word_t x = dir[i + 2*TAPE_FILES];
      for (j = 0; j < 7; j++)
	block_area[7*i + j + 1] = (x >> ((5 * (6-j)) + 1)) & 037;
    }

  if (verbose)
    {
      printf ("BLOCKS:");
      for (i = 0; i < TAPE_BLOCKS; i++)
	{
	  if ((i & 017) == 0)
	    printf ("\n %04o: ", i);
	  printf (" %02o", block_area[i]);
	}
      printf ("\n");
    }
}

static char *blocktype[040] =
  {
    "Free",
    "file1", "file2", "file3", "file4", "file5", "file6", "file7", "file10",
    "file11", "file12", "file13", "file14", "file15", "file14", "file17",
    "file20", "file21", "file22", "file23",  "file24",  "file25",  "file26",
    "file27", 
    "Unused (code 30)", "Unused (code 31)", "Unused (code 32)",
    "File directory",
    "Unused (code 34)",
    "Unfiled output",
    "Reserved",
    "End"
  };

static void
show_blocks (void)
{
  int blocks[040];
  word_t *dir = get_block (DIRECTORY_BLOCK);
  int empty = 1;
  int i, j;

  if (!verbose)
    return;

  memset (blocks, 0, sizeof blocks);

  for (i = 0; i < 128 - 2*TAPE_FILES; i++)
    {
      word_t x = dir[i + 2*TAPE_FILES];
      for (j = 0; j < 7; j++)
	blocks[(x >> ((5 * (6-j)) + 1)) & 037]++;
    }

  for (i = 0; i < 040; i++)
    {
      if (i >= 1 && i <= TAPE_FILES)
	continue;
      if (blocks[i] == 0)
	continue;
      if (i == 033 && blocks[i] == 1)
	continue; /* Expect 1 directory block. */
      if (i == 036 && blocks[i] == 7)
	continue; /* Expect 7 reserved blocks. */
      if (i == 037 && blocks[i] == 4)
	continue; /* Expect 4 end blocks. */

      if (empty)
	putchar ('\n');
      printf ("%s blocks: %d\n", blocktype[i], blocks[i]);
      empty = 0;
    }
}

static void
write_block (FILE *f, int n)
{
  int i;
  blocks++;
  word_t *x = get_block (n);
  for (i = 0; i < 128; i++)
    write_word (f, *x++);
}

static void write_file (int, FILE *);

static void
write_reverse_file (int n, FILE *f)
{
  int i;

  for (i = TAPE_BLOCKS-1; i >= 1; i--)
    {
      if (block_area[i] == n)
	write_block (f, i);
    }

  if (extension[n])
    write_file (extension[n], f);
}

static void
write_file (int n, FILE *f)
{
  int i;

  for (i = 1; i < TAPE_BLOCKS; i++)
    {
      if (block_area[i] == n)
	write_block (f, i);
    }

  if (extension[n])
    write_reverse_file (extension[n], f);
}

static void
extract_file (int i, char *name)
{
  if (!extract)
    return;

  FILE *f = fopen (name, "wb");
  blocks = 0;
  write_file (i, f);
  flush_word (f);
  fclose (f);
}

static void
show_name ()
{
  word_t *dir = get_block (DIRECTORY_BLOCK);
  char name[7];
  sixbit_to_ascii (dir[0177] & 0777777, name);
  printf ("%3s\n", name+3);
}

static void
show_files ()
{
  char filename[50];
  char fn1[7], fn2[7];
  int i;

  for (i = 0; i < TAPE_FILES; i++)
    {
      if (get_dir (i)[0] == 0)
	continue;

      sixbit_to_ascii (get_dir (i)[0], fn1);
      sixbit_to_ascii (get_dir (i)[1], fn2);
      printf ("%2d. %s %s  %c", i+1, fn1, fn2, type[mode[i]]);
      weenixpath (filename, -1LL, get_dir (i)[0], get_dir (i)[1]);
      extract_file (i+1, filename);
      printf ("   %4d\n", blocks);
    }

  extract_file (0, "free-blocks");
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s -x|-t <file>\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  word_t *buffer;
  FILE *f;

  input_word_format = &dta_word_format;
  output_word_format = &its_word_format;
  verbose = 1;

  if (argc != 3)
    usage (argv[0]);

  if (argv[1][0] != '-')
    usage (argv[0]);

  switch (argv[1][1])
    {
    case 't':
      extract = 0;
      break;
    case 'x':
      extract = 1;
      break;
    default:
      usage (argv[0]);
      break;
    }

  f = fopen (argv[2], "rb");
  if (f == NULL)
    {
      fprintf (stderr, "error\n");
      exit (1);
    }

  buffer = image;
  for (;;)
    {
      int n = read_block (f, buffer);
      if (n == -1)
	break;
      buffer += BLOCK_WORDS;
    }

  process ();
  show_name ();
  show_files ();
  show_blocks ();

  return 0;
}
