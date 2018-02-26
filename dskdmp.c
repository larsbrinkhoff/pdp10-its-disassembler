/* Copyright (C) 2017 Lars Brinkhoff <lars@nocrew.org>

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

#include "dis.h"

#define RP06_IMAGE_SIZE 316047360
#define SECTOR_WORDS 128
#define BLOCK_WORDS 1024
#define BLOCK_SECTORS (BLOCK_WORDS / SECTOR_WORDS)

/* UFD, URNDM */
#define UNLINK 0000001000000LL
#define UNREAP 0000002000000LL
#define UNWRIT 0000004000000LL
#define UNMARK 0000010000000LL
#define DELBTS 0000020000000LL
#define UNIGFL 0000024000000LL
#define UNDUMP 0400000000000LL

word_t image[RP06_IMAGE_SIZE / 8 + 1];
int blocks;
int mblks;
int xblks;
int tblks;
int nblks;
int nblksc;
int nsecsc;
int ntutbl;
int mfdblk;
int tutblk;
int mdnuds;
char *type;

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? -1 : c;
}

static word_t get_disk_word (FILE *f)
{
  int i, x;
  word_t w = 0;

  for (i = 0; i < 8; i++)
    {
      x = get_byte (f);
      if (x == -1)
	return -1;

      w += (word_t)x << (i * 8);
    }

  return w;
}

static word_t *
get_block (int block)
{
  int cylinder = block / nblksc;
  int sector = cylinder * nsecsc;
  return &image[sector * SECTOR_WORDS
		+ (block % nblksc) * BLOCK_WORDS];
}

static int read_block (FILE *f, word_t *buffer)
{
  int i;

  for (i = 0; i < BLOCK_WORDS; i++)
    {
      buffer[i] = get_disk_word (f);
      if (buffer[i] == -1)
	return -1;
    }

  return 0;
}

static void
show_disk (void)
{
  fprintf (stderr, "\n--- Disk info ---\n");
  fprintf (stderr, "Type: %s\n", type);
  fprintf (stderr, "NBLKS = %o\n", nblks);
  fprintf (stderr, "MFDBLK = %o\n", mfdblk);
  fprintf (stderr, "TUTBLK = %o\n", tutblk);
}

static void
show_tut (void)
{
  word_t *tut = get_block (tutblk);
  char str[7];

  fprintf (stderr, "\n--- TUT info ---\n");
  fprintf (stderr, "QPKNUM = %llo\n", tut[0]);
  sixbit_to_ascii (tut[1], str);
  fprintf (stderr, "QPAKID = %s\n", str);
  fprintf (stderr, "QTUTP = %llo\n", tut[2]);
  fprintf (stderr, "QSWAPA = %llo\n", tut[3]);
  fprintf (stderr, "QFRSTB = %llo\n", tut[4]);
  fprintf (stderr, "QLASTB = %llo\n", tut[5]);
  fprintf (stderr, "QTRSRV = %llo\n", tut[6]);
}

static int
ildb (word_t **w, int *p)
{
  word_t b = **w;
  b = b >> (30 - 6*(*p));
  b &= 077;

  (*p)++;
  if (*p == 6)
    {
      *p = 0;
      (*w)++;
    }

  return (int)b;
}

static void
print_blocks (int start, int end, int end_words)
{
  int i, j, n = 1024;

  return;

  for (i = start; i <= end; i++)
    {
      if (i == end)
	n = end_words;
      for (j = 0; j < n; j++)
	{
	  word_t w = get_block (i)[j];
	  putchar ((w >> 29) & 0177);
	  putchar ((w >> 22) & 0177);
	  putchar ((w >> 15) & 0177);
	  putchar ((w >>  8) & 0177);
	  putchar ((w >>  1) & 0177);
	}
    }
}

static int
show_blocks (word_t *ufd, int undscp, int end_words, int print)
{
  word_t *d;
  int i, o, b, n, n2, n3;
  int count = 0;
  int start = -1, end;

  d = &ufd[11+undscp/6];
  o = undscp % 6;

  for (;;)
    {
      n = ildb (&d, &o);
      if (start != -1)
	print_blocks (start, end, n == 0 ? end_words : 1024);
      start = -1;
      switch (n)
	{
	case 0:
	  goto end;
	case 1: case 2: case 3: case 4: case 5: case 6:
	case 7: case 8: case 9: case 10: case 11: case 12:
	  if (print)
	    fprintf (stderr, " %o-%o", b, b + n - 1);
	  start = b;
	  end = b + n - 1;
	  b += n;
	  count += n;
	  break;
	case 13: case 14: case 15: case 16: case 17: case 18: case 19:
	case 20: case 21: case 22: case 23: case 24:
	case 25: case 26: case 27: case 28: case 29: case 30:
	  b += n - 12;
	  if (print)
	    {
	      fprintf (stderr, " [SKIP %o]", n-12);
	      fprintf (stderr, " %o", b);
	    }
	  start = end = b;
	  count++;
	  b++;
	  break;
	case 037:
	  if (print)
	    fprintf (stderr, "[NOP]");
	  break;
	default:
	  n2 = ildb (&d, &o);
	  n3 = ildb (&d, &o);
	  b = ((n & 037) << 12) + (n2 << 6) + n3;
	  if (print)
	    fprintf (stderr, " %o", b);
	  start = end = b;
	  count++;
	  b++;
	  break;
	}
    }

 end:
  if (print)
    fprintf (stderr, "\n");
  return count;
}

extern int supress_warning;

static void
show_ufd (int index, char *name)
{
  int b = (index - 02000 + 2*mdnuds) / 2;
  word_t *ufd = get_block (b);
  char str[7];
  int i, n;

  fprintf (stderr, "\n--- UFD: %s ---\n", name);
  fprintf (stderr, "UDESCP = %llo\n", ufd[0]);
  fprintf (stderr, "UDNAMP = %llo\n", ufd[1]);
  sixbit_to_ascii (ufd[2], str);
  fprintf (stderr, "UDNAME = %s\n", str);
  fprintf (stderr, "UDBLKS = %llo\n", ufd[3]);
  fprintf (stderr, "UDALLO = %llo\n", ufd[4]);

  for (i = ufd[1]; i < BLOCK_WORDS; i += 5)
    {
      if (ufd[i+2] & UNLINK)
	fprintf (stderr, "  L   ");
      else
	{
	  fprintf (stderr, "%c", (ufd[i+2] & UNIGFL) == 0 ? ' ' : '*');
	  fprintf (stderr, " %-2llo  ", (ufd[i+2] >> 13) & 037);
	}

      sixbit_to_ascii (ufd[i], str);
      fprintf (stderr, "%s", str);
      sixbit_to_ascii (ufd[i+1], str);
      fprintf (stderr, " %s", str);

      if (ufd[i+2] & UNLINK)
	{
	  // Print link target.
	}
      else
	{
	  n = show_blocks (ufd, ufd[i+2] & 017777, (ufd[i+2] >> 24) & 01777, 0);
	  fprintf (stderr, " %d +%-4lld ", n - 1, (ufd[i+2] >> 24) & 01777);

	  fprintf (stderr, "%s", (ufd[i+2] & UNREAP) ? "$" : "");
	  fprintf (stderr, "%s", (ufd[i+2] & UNDUMP) ? "  " : "! ");

	  if ((ufd[i+4] & 0777LL) != 0777LL)
	    {
	      int b, c;
	      b = byte_size ((int)(ufd[i+4] & 0777LL), &c);
	      fprintf (stderr, " [%d, %d] ", b, c);
	    }

	  supress_warning = 1;
	  print_datime (stderr, ufd[i+3]);

	  if ((ufd[i+4] & 0777777000000LL) != 0777777000000LL)
	    {
	      fprintf (stderr, " (");
	      print_date (stderr, ufd[i+4]);
	      fprintf (stderr, ")");
	    }

	  if ((ufd[i+4] & 0777000LL) == 0777000LL)
	    fprintf (stderr, " -\?\?-");
	  else if ((ufd[i+4] & 0777000LL) != 0)
	    fprintf (stderr, " FOOBAR");
	}

      fprintf (stderr, "\n");
    }
}

static void
show_mfd (void)
{
  word_t *mfd = get_block (mfdblk);
  char str[7];
  int i;

  if (mfd[5] != 0551646164416)
    {
      fprintf (stderr, "MFDCLB\n");
      exit (1);
    }

  fprintf (stderr, "\n--- MFD info ---\n");
  fprintf (stderr, "MDNUM = %llo\n", mfd[0]);
  fprintf (stderr, "MDNAMP = %llo\n", mfd[1]);
  fprintf (stderr, "MDYEAR = %llo\n", mfd[2]);
  fprintf (stderr, "MPDOFF = %llo\n", mfd[3]);
  fprintf (stderr, "MPDWDK = %llo\n", mfd[4]);
  sixbit_to_ascii (mfd[5], str);
  fprintf (stderr, "MDCKH = %s\n", str);
  mdnuds = mfd[6];
  fprintf (stderr, "MDNUDS = %o\n", mdnuds);
  fprintf (stderr, "LMIBLK = %llo\n", mfd[7]);

  for (i = mfd[1]; i < BLOCK_WORDS; i += 2)
    {
      sixbit_to_ascii (mfd[i], str);
      show_ufd (i, str);
    }
}

int
main (int argc, char **argv)
{
  word_t *buffer;
  FILE *f;
  int i;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  buffer = image;
  blocks = 0;
  for (;;)
    {
      int n = read_block (f, buffer);
      if (n == -1)
	break;
      buffer += BLOCK_WORDS;
      blocks++;
    }

  fprintf (stderr, "%o blocks in image\n", blocks);

  switch (blocks)
    {
    case 10075:
      type = "RP03";
      ntutbl = 1;
      nblks = 011610;
      nsecsc = 10 * 20;
      break;
    case 38580:
      type = "RP06";
      ntutbl = 4;
      nblks = 0112424;
      nsecsc = 19 * 20;
      break;
    default:
      fprintf (stderr, "Unknown disk type.\n");
      exit (1);
    }

  nblksc = nsecsc / BLOCK_SECTORS;
  mfdblk = nblks/2-1;
  tutblk = mfdblk-ntutbl;

  show_disk ();
  show_tut();
  show_mfd();

  return 0;
}
