/* Copyright (C) 2019 Lars Brinkhoff <lars@nocrew.org>

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
#include <unistd.h>
#include <sys/time.h>

#include "dis.h"

#define MAGIC ((word_t)(014777252031LL))
#define MASK ((word_t)(0557255727156LL)) /* Sixbit MZMZYN */
#define UMASK ((word_t)(0633126423275LL))

#define LEFT 0777777000000LL
#define RIGHT 0777777LL

/* Just allocate a full moby to hold the file. */
static word_t buffer[256 * 1024];

static void usage (const char *x)
{
  fprintf (stderr, "Usage: %s -t|-x[e] [-W<input word format>] [-X<output word format>] <file>\n", x);
  usage_word_format ();
  exit (1);
}

static void
massage (char *filename)
{
  char *x;

  filename[6] = ' ';
  x = filename + 12;
  while (*x == ' ')
    {
      *x = 0;
      x--;
    }

  x = filename;
  while (*x)
    {
      if (*x == '/')
	*x = '|';
      x++;
    }
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
timestamps (char *filename, word_t timestamp)
{
  struct timeval tv[2];
  unix_time (&tv[0], timestamp);
  tv[1] = tv[0];
  utimes (filename, tv);
}

static void
extract_file (char *filename, word_t *data, word_t length, word_t key)
{
  FILE *f;
  int i;

  f = fopen(filename, "wb");
  for (i = 0; i < length; i++)
    {
      write_word (f, *data++ ^ key);
    }

  flush_word (f);
  fclose (f);
}

int
main (int argc, char **argv)
{
  int ipak_size;
  int extract;
  char string[7];
  word_t word;
  word_t *p;
  word_t key = 0;
  FILE *f;
  int opt;
  int i;

  while ((opt = getopt (argc, argv, "etxW:X:")) != -1)
    {
      switch (opt)
	{
	case 'e':
	  key = MASK;
	  break;
	case 't':
	  extract = 0;
	  break;
	case 'x':
	  extract = 1;
	  break;
	case 'W':
	  if (parse_input_word_format (optarg))
	    usage (argv[0]);
	  break;
	case 'X':
	  if (parse_output_word_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	}
    }

  if (optind != argc - 1)
    usage (argv[0]);

  f = fopen (argv[optind], "rb");

  p = buffer;
  while ((word = get_word (f)) != -1)
    {
      *p++ = word;
    }
  fclose (f);

  ipak_size = p - buffer;

  if (buffer[0] == MAGIC)
    {
      fprintf (stderr, "Format: 1977\n");
      i = 0;
    }
  else if (buffer[1] == MAGIC)
    {
      fprintf (stderr, "Format: 1978\n");
      sixbit_to_ascii (buffer[0] ^ UMASK, string);
      fprintf (stderr, "User: %s\n", string);
      i = 1;
    }
  else if (buffer[4] == MAGIC)
    {
      fprintf (stderr, "Format: 1980\n");
      sixbit_to_ascii (buffer[1] ^ UMASK, string);
      fprintf (stderr, "User: %s\n", string);
      i = 4;
    }
  else if (buffer[5] == MAGIC)
    {
      sixbit_to_ascii (buffer[1] ^ UMASK, string);
      fprintf (stderr, "User: %s\n", string);
      i = 5;
    }

  fprintf (stderr, "\nFile name       Words  Timestamp\n");

  while (i < ipak_size)
    {
      char filename[14];
      word_t timestamp;

      if (buffer[i] != MAGIC)
        fprintf (stderr, "More magic?\n");

      sixbit_to_ascii(buffer[i+1], filename);
      fprintf (stderr, "%s ", filename);
      sixbit_to_ascii(buffer[i+2], filename + 7);
      fprintf (stderr, "%s  ", filename + 7);

      massage (filename);

      timestamp = buffer[i+3];
      word_t length = buffer[i+4];

      fprintf (stderr, "%6lld  ", length);
      print_datime (stderr, timestamp);
      fputc ('\n', stderr);

      if (extract)
	{
	  extract_file (filename, &buffer[i+5], length, key);
	  timestamps (filename, timestamp);
	}

      i += length + 5;
    }

  return 0;
}
