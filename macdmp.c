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
#include <unistd.h>
#include <string.h>

#include "dis.h"

#define TAPE_FILES 027
#define TAPE_BLOCKS 01100
#define BLOCK_WORDS 128
#define DIRECTORY_BLOCK 0100

word_t image[(TAPE_BLOCKS + 4) * BLOCK_WORDS];
int blocks;
static void (*extract) (int, char *);
int verbose;

int mode[TAPE_FILES];
int extension[TAPE_FILES];
int block_area[TAPE_BLOCKS + 1];
int block_ptr;
int direction;

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

static int
read_block (FILE *f, word_t *buffer)
{
  int i;

  for (i = 0; i < BLOCK_WORDS; i++)
    {
      buffer[i] = get_word (f);
      if (buffer[i] == -1 && i == 0)
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

  if (verbose >= 2)
    {
      printf ("BLOCK AREA:");
      for (i = 0; i < TAPE_BLOCKS; i++)
	{
	  if ((i & 017) == 0)
	    printf ("\n %04o: ", i);
	  printf (" %02o", block_area[i]);
	}
      printf ("\n");
    }
}

static void
unprocess (void)
{
  word_t *dir = get_block (DIRECTORY_BLOCK);
  int i, j;

  block_ptr = 1;
  for (i = 2*TAPE_FILES; i < BLOCK_WORDS; i++)
    {
      word_t x = 0;
      for (j = 0; j < 7; j++)
	{
	  x <<= 5;
	  x |= block_area[block_ptr];
	  block_ptr++;
	}
      dir[i] = x << 1;
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
  if (f == NULL)
    return;
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
  FILE *f = fopen (name, "wb");
  blocks = 0;
  write_file (i, f);
  flush_word (f);
  fclose (f);
}

static void
list_file (int i, char *name)
{
  (void)name;
  blocks = 0;
  write_file (i, NULL);
}

static void
show_name ()
{
  word_t *dir = get_block (DIRECTORY_BLOCK);
  char name[7];
  if (verbose)
    {
      sixbit_to_ascii (dir[0177] & 0777777, name);
      printf ("%3s\n", name+3);
    }
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
	{
	  if (get_dir (i)[1] == 0 && verbose >= 2) {
	    list_file (i+1, NULL);
	    if (blocks > 0)
	      printf ("%2d. No file            %4d\n", i+1, blocks);
	  }
	  if (get_dir (i)[1] != 0 && verbose >= 2)
	    printf ("%2d. Extension for file %llo\n", i+1, get_dir (i)[1]);
	  continue;
	}

      weenixpath (filename, -1LL, get_dir (i)[0], get_dir (i)[1]);
      extract (i+1, filename);
      if (verbose)
	{
	  sixbit_to_ascii (get_dir (i)[0], fn1);
	  sixbit_to_ascii (get_dir (i)[1], fn2);
	  printf ("%2d. %s %s  %c", i+1, fn1, fn2, type[mode[i]]);
	  printf ("   %4d\n", blocks);
	}
    }

  extract (0, "free-blocks");
}

static int
allocate_dir (word_t fn1, word_t fn2)
{
  char s1[7], s2[7];
  int i;
  for (i = 0; i < TAPE_FILES; i++)
    {
      if (get_dir (i)[0] == 0 && get_dir (i)[1] == 0)
	{
	  get_dir (i)[0] = fn1;
	  get_dir (i)[1] = fn2;
	  return i + 1;
	}
    }

  sixbit_to_ascii (fn1, s1);  
  sixbit_to_ascii (fn2, s2);  
  fprintf (stderr, "Directory full: %s %s doesn't fit.\n", s1, s2);
  exit (1);
}

static int
allocate_block (int *i)
{
  while (block_ptr > 0 && block_ptr < 01070)
    {
      if (block_area[block_ptr] == 0)
	{
	  block_area[block_ptr] = *i;
	  return block_ptr;
	}
      block_ptr += direction;
    }

  *i = allocate_dir (0, *i);
  direction = -direction;
  block_ptr += direction;
  return allocate_block (i);
}

static void
read_file (FILE *f, int i)
{
  word_t buf[BLOCK_WORDS];
  int n;

  while (!feof (f))
    {
      if (read_block (f, buf) == -1)
	return;
      n = allocate_block (&i);
      memcpy (get_block (n), buf, BLOCK_WORDS * sizeof (word_t));
    }
}

static void
create_file (char **name, int n)
{
  word_t fn1, fn2;
  FILE *f;
  int i;

  if (n == 0)
    return;

  f = fopen (*name, "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening file %s.\n", *name);
      exit (1);
    }

  block_ptr = 1;
  direction = 1;

  winningname (&fn1, &fn2, *name);
  i = allocate_dir (fn1, fn2);
  read_file (f, i);
  fclose (f);

  create_file (name + 1, n - 1);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s [-v] [-W<word format>] -x|-t <tape>, or -c <tape> <files...>\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  char *image_file;
  int i, create = 0;
  word_t *buffer;
  FILE *f;
  int opt;

  image_file = NULL;
  input_word_format = &dta_word_format;
  output_word_format = &its_word_format;
  verbose = 0;

  while ((opt = getopt (argc, argv, "vc:t:x:W:")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  verbose++;
	  break;
	case 'c':
	  if (image_file)
	    usage (argv[0]);
	  create = 1;
	  image_file = optarg;
	  break;
	case 't':
	  if (image_file)
	    usage (argv[0]);
	  verbose++;
	  extract = list_file;
	  image_file = optarg;
	  break;
	case 'x':
	  if (image_file)
	    usage (argv[0]);
	  extract = extract_file;
	  image_file = optarg;
	  break;
	case 'W':
	  if (parse_output_word_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	  break;
	}
    }

  if (!create && optind != argc)
    usage (argv[0]);

  f = fopen (image_file, create ? "wb" : "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening tape image file %s\n", image_file);
      exit (1);
    }

  if (create)
    {
      struct word_format *tmp = input_word_format;
      input_word_format = output_word_format;
      output_word_format = tmp;

      memset (image, 0, sizeof image);
      memset (block_area, 0, sizeof block_area);
      memset (get_block (DIRECTORY_BLOCK), 0, sizeof (word_t) * BLOCK_WORDS);
      memset (extension, 0, sizeof extension);
      memset (mode, 0, sizeof mode);

      block_area[1] = 036;
      block_area[2] = 036;
      block_area[3] = 036;
      block_area[4] = 036;
      block_area[5] = 036;
      block_area[6] = 036;
      block_area[7] = 036;
      block_area[DIRECTORY_BLOCK] = 033;
      block_area[01070] = 037;
      block_area[01071] = 037;
      block_area[01072] = 037;
      block_area[01073] = 037;
      block_area[01074] = 037;
      block_area[01075] = 037;
      block_area[01076] = 037;
      block_area[01077] = 037;

      create_file (argv + optind, argc - optind);
      unprocess ();
      for (i = 0; i < TAPE_BLOCKS; i++)
	write_block (f, i);

      return 0;
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
