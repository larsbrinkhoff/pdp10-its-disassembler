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

#include <time.h>
#include <stdio.h>
#include <sys/time.h>

#include "dis.h"

#define VERY_OLD_ARC ((word_t)(0777777777777LL))
#define OLD_ARC ((word_t)(0416243010101LL)) /* Sixbit ARC!!! */
#define NEW_ARC ((word_t)(0416243210101LL)) /* Sixbit ARC1!! */

#define LEFT 0777777000000LL
#define RIGHT 0777777LL

static int old = 0;

/* Just allocate a full moby to hold the file. */
static word_t buffer[256 * 1024];

static void usage (const char *x)
{
  fprintf (stderr, "Usage: %s -x|-t <file>\n", x);
  exit (1);
}

static void
unix_time (struct timeval *tv, word_t t)
{
  struct tm tm;
  int seconds = (t & RIGHT) / 2;
  int date = (t >> 18);

  tm.tm_sec = seconds % 60;
  tm.tm_min = (seconds / 60) % 60;
  tm.tm_hour = seconds / 3600;
  tm.tm_mday = (date & 037);
  tm.tm_mon = ((date & 0740) >> 5) - 1;
  tm.tm_year = (date & 0777000) >> 9;
  tm.tm_isdst = 0;

  tv->tv_sec = mktime (&tm);
  tv->tv_usec = (t & 1) * 500000L;
}

static void
timestamps (char *filename, word_t modified, word_t referenced)
{
  struct timeval tv[2];
  unix_time (&tv[0], referenced);
  unix_time (&tv[1], modified);
  utimes (filename, tv);
}

static void
extract_file (char *filename, word_t *data, word_t length)
{
  FILE *f;
  int i;

  f = fopen(filename, "wb");
  for (i = 0; i < length; i++)
    {
      write_word (f, *data++);
    }

  flush_word (f);
  fclose (f);
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

static int
extract_block (FILE *f, word_t *block, int *b, int *count)
{
  word_t header = *block;
  int i, n;

  *b = header & 017777777;
  n = ((header >> 23) & 01777) + 1;
  *count += n;

  if (f)
    {
      block++;
      for (i = 0; i < n; i++)
	write_word (f, block[i]);
    }

  return (header & 0200000000000LL) == 0;
}

static int
extract_blocks (FILE *f, word_t *ufd, int undscp)
{
  word_t *d;
  int o, b, n, n2, n3;
  int count = 0;

  d = &ufd[11+undscp/6];
  o = undscp % 6;

  n = ildb (&d, &o);
  if (n != 040)
    {
      fprintf (stderr, "ERROR\n");
      exit (1);
    }
  
  n2 = ildb (&d, &o);
  n3 = ildb (&d, &o);
  b = ((n & 037) << 12) + (n2 << 6) + n3;

  b = buffer[02005+b];
  while (extract_block (f, &buffer[b], &b, &count))
    ;

  return count;
}

static int
extract_old_file (char *filename, int i, int extract)
{
  word_t *ufd = buffer;
  int undscp = ufd[i+2] & 017777;
  FILE *f = NULL;
  int n;

  if (extract)
    f = fopen(filename, "wb");

  n = extract_blocks (f, ufd, undscp);

  if (extract)
    {
      flush_word (f);
      fclose (f);
    }

  return n;
}

int
main (int argc, char **argv)
{
  int extract;
  char string[7];
  word_t word;
  word_t *p;
  FILE *f;

  input_word_format = &its_word_format;
  output_word_format = &its_word_format;

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

  p = buffer;
  while ((word = get_word (f)) != -1)
    {
      *p++ = word;
    }
  fclose (f);

  /* word_t arc_size = p - buffer; */

  if (buffer[0] == NEW_ARC)
    {
      /* fprintf (stderr, "New ARC1!! archive.\n") */ ;
    }
  else if (buffer[0] == OLD_ARC)
    {
      fprintf (stderr, "Old ARC!!! archive.\n");
      old = 1;
    }
  else if (buffer[0] == VERY_OLD_ARC)
    {
      fprintf (stderr, "Old 777777777777 archive.\n");
      old = 1;
    }
  else
    {
      sixbit_to_ascii(buffer[0], string);
      fprintf (stderr, "First word: %012llo \"%s\"\n", buffer[0], string);
      fprintf (stderr, "Not an ARC file.\n");
      exit (1);
    }

  word_t name_beg;
  if (old)
     name_beg = buffer[2];
  else
     name_beg = buffer[1];
  /* word_t data_end = buffer[2]; */

  fprintf (stderr, "Last cleanup: ");
  print_datime (stderr, buffer[3]);
  fputc ('\n', stderr);

  if (!old)
    {
      fprintf (stderr, "Created: ");
      print_datime (stderr, buffer[4]);
      fputc ('\n', stderr);

      word_t dumped = buffer[5];
      fprintf (stderr, "Dumped: %llo\n", dumped);
    }

  fprintf (stderr, "\nFile name       Words  Modified            Referenced    Byte\n");

  int i;
  for (i = name_beg; i < 02000; i += 5)
    {
      char filename[14];
      word_t modified, referenced;

      sixbit_to_ascii(buffer[i], filename);
      fprintf (stderr, "%s ", filename);
      sixbit_to_ascii(buffer[i+1], filename + 7);
      fprintf (stderr, "%s  ", filename + 7);

      /* File name for extraction. */
      weenixpath (filename, -1LL, buffer[i], buffer[i+1]);

      /* word_t flags = buffer[i+2] >> 18; */
      word_t data = buffer[i+2] & RIGHT;

      modified = buffer[i+3];
      referenced = (buffer[i+4] & LEFT);

      word_t length;
      if (old) {
	length = extract_old_file (filename, i, extract);
	timestamps (filename, modified, referenced);
      } else
	length = buffer[data] - 3;
      fprintf (stderr, "%6lld  ", length);

      print_datime (stderr, modified);
      fputc ('(', stderr);
      print_date (stderr, referenced);
      fputc (')', stderr);

      if (!old)
	{
	  int author = (buffer[i+4] >> 9) & 0777;
	  if (author != 0 && author != 0777)
	    fprintf (stderr, "  Author %03o", author);
	}

      int leftovers;
      fprintf (stderr, "  %d\n",
	       byte_size (buffer[i+4] & 0777, &leftovers));

      if (!old && extract)
	{
	  extract_file (filename, &buffer[data+3], length);
	  timestamps (filename, modified, referenced);
	}
    }

  return 0;
}
