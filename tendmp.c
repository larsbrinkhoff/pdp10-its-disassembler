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

#define TAPE_BLOCKS      01102
#define BLOCK_WORDS      128
#define DATA_WORDS       127
#define DIRECTORY_BLOCK  100
#define TAPE_FILES       026
#define FILE_NAME        0123
#define FILE_EXT         (FILE_NAME + TAPE_FILES)
#define TAPE_LABEL       0177

#define DATE(X)          ((X) & 07777)
#define CORE(X)          (((X) >> 12) & 077)

#define SIZE(X)          ((X) & 0377)
#define FIRST(X)         (((X) >> 8) & 01777)
#define LINK(X)          (((X) >> 18) & 0777777)

#define SWP   0636760000000LL
#define SAV   0634166000000LL

static word_t image[TAPE_BLOCKS * BLOCK_WORDS];
static int blocks;
static void (*visit) (int, char *);
static int verbose;
static int spacing;
static void (*start_alloc) (void);

static int timestamp[TAPE_FILES];
static int block_area[TAPE_BLOCKS + 4];
static int block_ptr;
static int direction;

static word_t *
get_block (int block)
{
  return &image[block * BLOCK_WORDS];
}

static int
read_block (FILE *f, word_t *buffer, int size)
{
  int i;
  for (i = 0; i < size; i++)
    {
      if ((buffer[i] = get_word (f)) == -1)
	break;
      buffer[i] &= 0777777777777LL;
    }
  return i;
}

static void
process (void)
{
  word_t *dir;
  int i, j;

  memset (timestamp, 0, sizeof timestamp);

  dir = get_block (DIRECTORY_BLOCK);
  for (i = 0; i < TAPE_FILES; i++)
    {
      timestamp[i] = DATE (dir[FILE_EXT + i]);
      if (dir[i] & 1)
	timestamp[i] |= 010000;
      if (dir[i + TAPE_FILES] & 1)
	timestamp[i] |= 020000;
      if (dir[i + 2*TAPE_FILES] & 1)
	timestamp[i] |= 040000;
    }

  for (i = 0; i < 0123; i++)
    {
      word_t x = dir[i];
      for (j = 0; j < 7; j++)
	block_area[7*i + j + 1] = (x >> ((5 * (6-j)) + 1)) & 037;
    }

  if (verbose < 2)
    return;

  printf ("BLOCK AREA:");
  for (i = 1; i < TAPE_BLOCKS + 4; i++)
    {
      if ((i % (2*7)) == 1)
	printf ("\n %04o: ", i);
      printf (" %02o", block_area[i]);
    }
  printf ("\n");
}

static void
unprocess (char *name)
{
  word_t *dir = get_block (DIRECTORY_BLOCK);
  int i, j;

  block_ptr = 1;
  for (i = 0; i < 0123; i++)
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

  for (i = 0; i < TAPE_FILES; i++)
    {
      if (timestamp[i] & 010000)
	dir[i] |= 1;
      if (timestamp[i] & 020000)
	dir[TAPE_FILES + i] |= 1;
      if (timestamp[i] & 040000)
	dir[TAPE_FILES*2 + i] |= 1;
    }

  if (name)
    dir[BLOCK_WORDS - 1] = ascii_to_sixbit (name);
}

static char *blocktype[040] =
  {
    "Free",
    "File1", "File2", "File3", "File4", "File5", "File6", "File7", "File8",
    "File9", "File10", "File11", "File12", "File13", "File14", "File15",
    "File16", "File17", "File18", "File19",  "File20",  "File21", "File22",
    "Unused27", "Unused30", "Unused31", "Unused32", "Unused33", "Unused34",
    "Unused35",
    "Reserved",
    "End"
  };

static void
show_blocks (void)
{
  int blocks[040];
  word_t *dir = get_block (DIRECTORY_BLOCK);
  int i, j;

  if (!verbose)
    return;

  memset (blocks, 0, sizeof blocks);

  for (i = 0; i < 0123; i++)
    {
      word_t x = dir[i];
      for (j = 0; j < 7; j++)
	blocks[(x >> ((5 * (6-j)) + 1)) & 037]++;
    }

  for (i = 0; i < 040; i++)
    {
      if (i >= 1 && i <= TAPE_FILES)
	continue;
      if (blocks[i] == 0)
	continue;
      if (i == 036 && blocks[i] == 3)
	continue; /* Expect 3 reserved blocks. */
      if (i == 037 && blocks[i] == 4)
	continue; /* Expect 4 end blocks. */
      printf ("%s blocks: %d\n", blocktype[i], blocks[i]);
    }
}

static void
show_link (int block, char *visited)
{
  word_t header = get_block (block)[0];
  int link = FIRST (header);
  int n = 0;

  if (link == 0)
    return;

  do
    {
      if (LINK (header) >= TAPE_BLOCKS
	  || FIRST (header) >= TAPE_BLOCKS
	  || SIZE (header) > 0177)
	{
	  printf (" Block %d header invalid: %012llo\n", block, header);
	  return;
	}

      block = link;
      if (visited[block])
	{
	  printf (" Block %d part of more than one file\n", block);
	  return;
	}
      visited[block] = 1;
      if (n == 0)
	printf ("  ");
      n += printf ("%d ", block);
      if (n > 60)
	{
	  printf ("\n    -> ");
	  n = 1;
	}
      header = get_block (block)[0];
      link = LINK (header);
    }
  while (link != 0);

  printf ("\n");
}

static void
show_links (void)
{
  char visited[TAPE_BLOCKS];
  int i;

  if (verbose < 2)
    return;

  printf ("FILE LINKS:\n");

  memset (visited, 0, sizeof visited);
  visited[0] = visited[1] = visited[2] = visited[DIRECTORY_BLOCK] = 1;

  for (i = 0; i < TAPE_BLOCKS; i++)
    {
      if (visited[i])
	continue;
      show_link (i, visited);
    }
}

static void
write_block (FILE *f, int n, int offset, int size)
{
  int i;
  blocks++;
  if (f == NULL)
    return;
  word_t *x = get_block (n) + offset;
  for (i = 0; i < size; i++)
    write_word (f, *x++);
}

static int
find (int file)
{
  int i;

  for (i = DIRECTORY_BLOCK - 1; i > 0; i--)
    {
      if (block_area[i] == file)
	return FIRST (get_block (i)[0]);
    }

  for (i = DIRECTORY_BLOCK + 1; i < TAPE_BLOCKS; i++)
    {
      if (block_area[i] == file)
	return FIRST (get_block (i)[0]);
    }

  return 0;
}

static void
write_file (int n, FILE *f)
{
  int block = find (n);
  word_t header;
  if (block == 0)
    {
      if (f != NULL)
	fprintf (stderr, "File %d first block not found.\n", n + 1);
      return;
    }
  while (block > 0)
    {
      header = get_block (block)[0];
      write_block (f, block, 1, SIZE (header));
      block = LINK (header);
    }
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
delete_file (int i, char *name)
{
}

static void
show_label ()
{
  word_t word = get_block (DIRECTORY_BLOCK)[TAPE_LABEL];
  char label[7];
  if (verbose && word != 0 && word != 0777777777777LL)
    {
      sixbit_to_ascii (word, label);
      printf ("Label: %6s\n", label);
    }
}

static void
show_files ()
{
  char filename[50];
  char fn1[7], fn2[7];
  word_t *dir;
  int i, core;

  dir = get_block (DIRECTORY_BLOCK);
  for (i = 0; i < TAPE_FILES; i++)
    {
      if (dir[FILE_NAME + i] == 0)
	{
	  if (dir[FILE_EXT + i] == 0 && verbose >= 2) {
	    list_file (i+1, NULL);
	    if (blocks > 0)
	      printf ("%2d. No file            %4d\n", i+1, blocks);
	  }
	  continue;
	}

      weenixpath (filename, -1LL,
		  dir[FILE_NAME + i], dir[FILE_EXT + i] & 0777777000000LL);
      visit (i+1, filename);
      if (verbose)
	{
	  sixbit_to_ascii (dir[FILE_NAME + i], fn1);
	  sixbit_to_ascii (dir[FILE_EXT + i] & 0777777000000LL, fn2);
	  printf ("%2d. %s %s", i+1, fn1, fn2);
	  printf ("   %4d ", blocks);
	  core = CORE (dir[FILE_EXT + i]);
	  if (core == 0)
	    printf ("     ");
	  else
	    printf ("%3dK ", core + 1);
	  if (timestamp[i])
	    print_dec_timestamp (stdout, timestamp[i]);
	  printf ("\n");
	}
    }
}

static int
allocate_dir (word_t fn1, word_t fn2)
{
  word_t *dir = get_block (DIRECTORY_BLOCK);
  char s1[7], s2[7];
  int i;
  for (i = 0; i < TAPE_FILES; i++)
    {
      if (dir[FILE_NAME + i] == 0 && dir[FILE_EXT + i] == 0)
	{
	  dir[FILE_NAME + i] = fn1;
	  dir[FILE_EXT + i] = fn2;
	  return i + 1;
	}
    }

  sixbit_to_ascii (fn1, s1);
  sixbit_to_ascii (fn2 & 0777777000000LL, s2);
  fprintf (stderr, "Directory full: %s.%s doesn't fit.\n", s1, s2);
  exit (1);
}

static int
allocate_block (int i)
{
  int block, pass;

  for (pass = 0; pass < 3; pass++)
    {
      while (block_ptr > 0 && block_ptr < TAPE_BLOCKS)
	{
	  if (block_area[block_ptr] == 0)
	    {
	      block = block_ptr;
	      block_area[block] = i;
	      block_ptr += spacing * direction;
	      return block;
	    }
	  block_ptr += direction;
	}
      direction = -direction;
      block_ptr += 2*direction;
    }

  fprintf (stderr, "File %d doesn't fit: out of blocks\n", i);
  exit (1);
}

static void
read_file (FILE *f, int i)
{
  word_t buf[BLOCK_WORDS];
  word_t *link = NULL;
  word_t first = 0;
  word_t n;
  int words;

  for (;;)
    {
      words = read_block (f, buf + 1, DATA_WORDS);
      if (words == 0)
	return;
      n = allocate_block (i);
      if (!first)
	first = n;
      buf[0] = (first << 8) | words;
      if (link)
	link[0] |= n << 18;
      link = get_block (n);
      memcpy (link, buf, sizeof buf);
    }
}

static int
core_size (FILE *f)
{
  struct pdp10_memory memory;
  init_memory (&memory);
  input_file_format->read (f, &memory, 0);
  rewind_word (f);
  return memory.area[memory.areas-1].end - memory.area[0].start;
}

static int
csave_type (FILE *f, word_t name)
{
  word_t word;

  switch (name)
    {
    case SWP:
    case SAV:
      break;
    default:
      return 0;
    }

  /* Check that this looks like a compressed save file. */
  word = get_word (f);
  rewind_word (f);
  if ((word & 0777777000000LL) < 0700000000000LL)
    return 0;

  return 1;
}

static void
start_temdmp (void)
{
  /* TENDMP can only load files with increasing block numbers.
     Might as well begin from the start of the tape. */
  direction = 1;
  block_ptr = 1;

  /* I order to fit TENEX files without changing direction, use
     a smaller spacing than default. */
  spacing = 2;
}

static void
start_default (void)
{
  /* The standard format says to start allocating below the directory,
     going towards the start of the tape. */
  direction = -1;
  block_ptr = DIRECTORY_BLOCK - 1;
  /* Standard spacing. */
  spacing = 4;
}

static void
create_file (char *name)
{
  word_t fn1, fn2;
  FILE *f;
  char *p;
  int i, core = 0;

  f = fopen (name, "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening file %s.\n", name);
      exit (1);
    }

  start_alloc ();

  p = strrchr (name, '/');
  if (p)
    name = p + 1;
  winningname (&fn1, &fn2, name);
  fn2 &= 0777777000000LL;
  if (csave_type (f, fn2))
    core = (core_size (f) + 1023) / 1024 - 1;
  fn2 |= core << 12;
  i = allocate_dir (fn1, fn2);
  read_file (f, i);
  fclose (f);
}

static void
boot_block (char *name)
{
  word_t word, *ptr;
  FILE *f;
  int i;

  f = fopen (name, "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening file %s.\n", name);
      exit (1);
    }

  ptr = get_block (0);
  for (i = 0; i < 3 * BLOCK_WORDS; i++)
    {
      word = get_word (f);
      if (word == -1)
	break;
      *ptr++ = word;
    }

  fclose (f);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s [-v] [-W<word format>] -x|-t <tape>,\n", x);
  fprintf (stderr, "or [-T] [-L<label>] [-b<boot blocks>] -[cdu] <tape> <files...>\n");
  exit (1);
}

int
main (int argc, char **argv)
{
  const char *mode = NULL;
  char *label = NULL;
  char *image_file, *boot_file = NULL;
  int i, create = 0;
  word_t *buffer;
  FILE *f;
  int opt;

  image_file = NULL;
  input_file_format = &csave_file_format;
  input_word_format = &dta_word_format;
  output_word_format = &aa_word_format;
  start_alloc = start_default;
  verbose = 0;

  output_file = fopen ("/dev/null", "w");

  while ((opt = getopt (argc, argv, "vc:t:x:W:L:b:T")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  verbose++;
	  break;
	case 'c':
	  if (image_file)
	    usage (argv[0]);
	  mode = "wb";
	  create = 1;
	  image_file = optarg;
	  break;
	case 'd':
	  if (image_file)
	    usage (argv[0]);
	  /* delete */
	  mode = "r+b";
	  visit = delete_file;
	  image_file = optarg;
	  break;
	case 'u':
	  if (image_file)
	    usage (argv[0]);
	  /* update */
	  mode = "r+b";
	  visit = delete_file;
	  image_file = optarg;
	  break;
	case 't':
	  if (image_file)
	    usage (argv[0]);
	  mode = "rb";
	  verbose++;
	  visit = list_file;
	  image_file = optarg;
	  break;
	case 'x':
	  if (image_file)
	    usage (argv[0]);
	  mode = "rb";
	  visit = extract_file;
	  image_file = optarg;
	  break;
	case 'b':
	  if (boot_file)
	    usage (argv[0]);
	  boot_file = optarg;
	  break;
	case 'W':
	  if (parse_output_word_format (optarg))
	    usage (argv[0]);
	  break;
	case 'L':
	  label = optarg;
	  break;
	case 'T':
	  start_alloc = start_temdmp;
	  break;
	default:
	  usage (argv[0]);
	  break;
	}
    }

  if (!create && optind != argc)
    usage (argv[0]);

  f = fopen (image_file, create ? mode);
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
      memset (timestamp, 0, sizeof timestamp);

      block_area[1] = 036;
      block_area[2] = 036;
      block_area[DIRECTORY_BLOCK] = 036;
      block_area[01102] = 037;
      block_area[01103] = 037;
      block_area[01104] = 037;
      block_area[01105] = 037;

      if (boot_file)
	boot_block (boot_file);
      for (; optind < argc; optind++)
	create_file (argv[optind]);
      unprocess (label);
      for (i = 0; i < TAPE_BLOCKS; i++)
	write_block (f, i, 0, BLOCK_WORDS);

      return 0;
    }

  buffer = image;
  for (i = 0; i < TAPE_BLOCKS; i++)
    {
      if (read_block (f, buffer, BLOCK_WORDS) == 0)
	break;
      buffer += BLOCK_WORDS;
    }

  process ();
  show_label ();
  show_files ();
  show_links ();
  show_blocks ();

  return 0;
}
