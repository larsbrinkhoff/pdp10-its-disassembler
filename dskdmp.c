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

word_t image[82534400 / 8 + 1];
int blocks;
int mblks;
int xblks;
int tblks;
int nblks;
int ntutbl;
int mfdblk;
int tutblk;
int mdnuds;

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

static int read_block (FILE *f, word_t *buffer)
{
  int i;

  for (i = 0; i < 1024; i++)
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
  fprintf (stderr, "NBLKS = %o\n", nblks);
  fprintf (stderr, "MFDBLK = %o\n", mfdblk);
  fprintf (stderr, "TUTBLK = %o\n", tutblk);
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
show_blocks (word_t *ufd, int undscp)
{
  word_t *d;
  int i, o, b, n, n2, n3;

  d = &ufd[11+undscp/6];
  o = undscp % 6;

  for (;;)
    {
      n = ildb (&d, &o);
      switch (n)
	{
	case 0:
	  goto end;
	case 1: case 2: case 3: case 4: case 5: case 6:
	case 7: case 8: case 9: case 10: case 11: case 12:
	  fprintf (stderr, " %o-%o", b, b + n - 1);
	  b += n;
	  break;
	case 13: case 14: case 15: case 16: case 17: case 18: case 19:
	case 20: case 21: case 22: case 23: case 24:
	case 25: case 26: case 27: case 28: case 29: case 30:
	  fprintf (stderr, " [SKIP %o]", n-12);
	  b += n - 12;
	  fprintf (stderr, " %o", b);
	  b++;
	  break;
	case 037:
	  fprintf (stderr, "[NOP]");
	  break;
	default:
	  n2 = ildb (&d, &o);
	  n3 = ildb (&d, &o);
	  b = ((n & 037) << 12) + (n2 << 6) + n3;
	  fprintf (stderr, " %o", b);
	  b++;
	  break;
	}
    }

 end:
  fprintf (stderr, "\n");
}

static void
show_ufd (int index, char *name)
{
  int b = (index - 02000 + 2*mdnuds) / 2;
  word_t *ufd = &image[b * 1024];
  char str[7];
  int i;

  fprintf (stderr, "\n--- UFD: %s ---\n", name);
  fprintf (stderr, "UDESCP = %llo\n", ufd[0]);
  fprintf (stderr, "UDNAMP = %llo\n", ufd[1]);
  sixbit_to_ascii (ufd[2], str);
  fprintf (stderr, "UDNAME = %s\n", str);
  fprintf (stderr, "UDBLKS = %llo\n", ufd[3]);
  fprintf (stderr, "UDALLO = %llo\n", ufd[4]);

  for (i = ufd[1]; i < 1024; i += 5)
    {
      sixbit_to_ascii (ufd[i], str);
      fprintf (stderr, "   %s", str);
      sixbit_to_ascii (ufd[i+1], str);
      fprintf (stderr, " %s", str);
      show_blocks (ufd, ufd[i+2] & 017777);
    }
}

static void
show_mfd (void)
{
  word_t *mfd = &image[mfdblk * 1024];
  char str[7];
  int i;

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

  for (i = mfd[1]; i < 1024; i += 2)
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
      buffer += 1024;
      blocks++;
    }

  fprintf (stderr, "%o blocks in image\n", blocks);

  ntutbl = 1;
  nblks = 011610;
  mfdblk = nblks/2-1;
  tutblk = mfdblk-ntutbl;

  show_disk ();
  show_mfd();

  return 0;
}
