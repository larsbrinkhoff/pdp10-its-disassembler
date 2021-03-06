/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

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
#include <utime.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "dis.h"

#define FAILS  0124641515463LL
#define AFE    0414645LL

static word_t block[3740];
static int mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static int density;
static int extract = 0;
static int verbose = 0;

static FILE *list;
static FILE *info;

static FILE *output;
static word_t checksum;
static char file_path[100];
struct timeval timestamp[2];

static int
february (int year)
{
  if (year < 1964 || year > 1999)
    {
      fprintf (stderr, "Anachronistic timestamp: year %d\n", year);
      exit (1);
    }

  /* This is good for the range 1964-1999, which is all we care about. */
  if ((year % 4) == 0)
    return 29;
  else
    return 28;
}

static void
compute_date (word_t word, int *year, int *month, int *day)
{
  *day = word & 037777;
  *year = 1964;
  *month = 0;

  mdays[1] = february (*year);
  for (;;)
    {
      if (*day < mdays[*month])
	return;

      *day -= mdays[*month];
      (*month)++;
      if (*month == 12)
	{
	  *month = 0;
	  (*year)++;
	  mdays[1] = february (*year);
	}
    }
}

static void
unix_time (struct timeval *tv, word_t word)
{
  int minutes = (word >> 14) & 03777;
  struct tm tm;
  compute_date (word, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
  tm.tm_sec = 0;
  tm.tm_min = minutes % 60;
  tm.tm_hour = minutes / 60;
  tm.tm_mday++;
  tm.tm_year -= 1900;
  tm.tm_isdst = 0;
  tv->tv_sec = mktime (&tm);
  tv->tv_usec = 0;
}

static void
print_timestamp (FILE *f, word_t word)
{
  int year, month, days;
  int minutes = (word >> 14) & 03777;
  compute_date (word, &year, &month, &days);
  fprintf (f, "%4d-%02d-%02d %02d:%02d",
	   year, month + 1, days + 1,
	   minutes / 60, minutes % 60);
}

static void
print_ascii (FILE *f, word_t word)
{
  fputc ((word >> 29) & 0177, f);
  fputc ((word >> 22) & 0177, f);
  fputc ((word >> 15) & 0177, f);
  fputc ((word >>  8) & 0177, f);
  fputc ((word >>  1) & 0177, f);
}

static int
right (word_t word)
{
  return word & 0777777LL;
}

static int
left (word_t word)
{
  return right (word >> 18);
}

static void
expect_file (word_t word)
{
  if (word & START_FILE)
    return;
  fprintf (stderr, "EXPECTED TAPE FILE\n");
  exit (1);
}

static void
expect_record (word_t word)
{
  if (word & START_RECORD)
    return;
  fprintf (stderr, "EXPECTED TAPE RECORD\n");
  exit (1);
}

static void
expect_file_or_record (word_t word)
{
  if ((word & (START_FILE|START_RECORD)) != 0)
    return;
  fprintf (stderr, "EXPECTED TAPE FILE OR RECORD\n");
  exit (1);
}

static int
saveset_record (word_t word)
{
  int tito = left (word);
  return tito > 0 && tito < 30;
}

static int
data_record (word_t word)
{
  return left (word) == 0;
}

static int
header_record (word_t word)
{
  return left (word) == 0777777;
}

static int
file_record (word_t word)
{
  return data_record (word) || header_record (word);
}

static void
process_header (FILE *f, word_t word, int trailer)
{
  int count;

  if (trailer)
    expect_record (word);
  else
    expect_file (word);

  fprintf (info, "TITO version: %d\n", left (word));
  count = right (word);
  fprintf (info, "Header words: %d\n", count);

  word = get_word (f);
  if (word != FAILS)
    fprintf (stderr, "EXPECTED FAILSAFE MAGIC\n");
  word = get_word (f);
  if (left (word) != AFE)
    fprintf (stderr, "EXPECTED FAILSAFE MAGIC\n");
  fprintf (info, "Tape sequence #%d\n", right (word));
  
  word = get_word (f);
  if (word & 0400000000000LL)
    fprintf (info, "Continuation tapes follow\n");
  if (word & 0200000000000LL)
    fprintf (info, "User continued\n");
  if (word & 0100000000000LL)
    fprintf (info, "File continued\n");
  fprintf (info, "Saveset written: ");
  print_timestamp (info, word);
  fputc ('\n', info);

  word = get_word (f);
  if (word != 000001000002LL)
    fprintf (stderr, "EXPECTED 1,,2\n");
}

static void
get_block (FILE *f, word_t *buffer, int words)
{
  int i;
  for (i = 0; i < words; i++)
    {
      buffer[i] = get_word (f);
      if (buffer[i] & (START_FILE|START_RECORD))
	{
	  fprintf (stderr, "Record too short.\n");
	  exit (1);
	}
    }
}

static void
check_block_size (int n)
{
  switch (density)
    {
    case 800:
      if (n >= 512)
	fprintf (stderr, "EXPECTED < 512\n");
      break;
    case 1600:
      if (n >= 1916)
	fprintf (stderr, "EXPECTED < 1916\n");
      break;
    case 6250:
      if (n >= 3740)
	fprintf (stderr, "EXPECTED < 3740\n");
      break;
    }
}

static void
close_file (word_t x)
{
  checksum &= 0777777777777;
  fprintf (info, "Checksum: %012llo (%012llo)\n", checksum, x);
  fclose (output);
  output = NULL;
  utimes (file_path, timestamp);
}

static void
open_file (char *directory, char *name, char *ext)
{
  weenixname (directory);
  fprintf (info, "DIRECTORY: %s\n", directory);
  if (mkdir (directory, 0777) == -1 && errno != EEXIST)
    fprintf (stderr, "Error creating output directory %s: %s\n",
	     directory, strerror (errno));

  weenixname (name);
  weenixname (ext);
  strcpy (file_path, directory);
  strcat (file_path, "/");
  strcat (file_path, name);
  if (*ext != 0)
    {
      strcat (file_path, ".");
      strcat (file_path, ext);
    }

  fprintf (info, "FILE: %s\n", file_path);
  output = fopen (file_path, "wb");
  if (output == NULL)
    fprintf (stderr, "Error opening output file %s: %s\n",
	     file_path, strerror (errno));

  checksum = 0;
}

static void
write_data (word_t *data, int size)
{
  int i;
  for (i = 0; i < size; i++)
    {
      checksum += *data;
      write_word (output, *data++);
    }
}

static word_t
process_file_header (FILE *f, word_t word)
{
  char sixbit[7];
  char directory[14];
  char name[7];
  char ext[4];
  int size;
  word_t t;

  expect_file_or_record (word);
  if (left (word) != 0777777)
    fprintf (stderr, "EXPECTED 777777,,\n");
  size = right (word);
  get_block (f, block+1, size);
  density = left (block[075]);
  check_block_size (size);

  if (left (block[1]) != 0446353)
    fprintf (stderr, "EXPECTED 'DSK'\n");
  fprintf (info, "TITO version: %d\n", right (block[1]));
  if (left (block[2]) != 0)
    fprintf (stderr, "EXPECTED 0\n");
  fprintf (info, "Count for extended lookup: %d\n", right (block[2]));

  t = (block[5] >> 2) & 030000;
  t |= block[6] & 07777;
  t |= (block[6] << 2) & 0177740000LL;
  unix_time (&timestamp[0], t);
  unix_time (&timestamp[1], t);

  sixbit_to_ascii (block[071], sixbit);
  strcpy (directory, sixbit);
  sixbit_to_ascii (block[072], sixbit);
  strcat (directory, sixbit);

  if (left (block[5]) == 0654644)
    {
      fprintf (list, "   (UFD)          ");
      sixbit_to_ascii (block[031], sixbit);
      fprintf (list, "%s", sixbit);
      sixbit_to_ascii (block[032], sixbit);
      fprintf (list, "%s  ", sixbit);
      print_timestamp (list, t);
      fprintf (list, "   [%o,%o]\n", left (block[4]), right (block[4]));
    }
  else
    {
      sixbit_to_ascii (block[4], name);
      sixbit_to_ascii (block[5] & 0777777000000LL, ext);
      strcat (directory, " ");
      *strchr (directory, ' ') = ')';
      fprintf (list, "   (%s %s.%s ", directory, name, ext);
      print_timestamp (list, t);
      fprintf (list, "   [%o,%o]\n", left (block[3]), right (block[3]));
      *strchr (directory, ')') = ' ';
    }

  fprintf (info, "System: %s", sixbit);
  print_ascii (info, block[073]);
  print_ascii (info, block[074]);
  fputc ('\n', info);

  fprintf (info, "Tape density: %d\n", density);
  fprintf (info, "Tape tracks: %d\n", right (block[075]));
  switch (right (block[1]))
    {
    case 14:
      fprintf (info, "Tape drive serial #%d\n", right (block[076]));
      break;
    case 15:
      sixbit_to_ascii (block[076], sixbit);
      fprintf (info, "Device name: %s\n", sixbit);
      break;
    }
  fprintf (info, "Tape written: ");
  print_timestamp (info, block[077]);
  fputc ('\n', info);
  fprintf (info, "Tape sequence #%d\n", left (block[0100]));
  fprintf (info, "File #%d\n", right (block[0100]));

  /* Check the next record to see if this is the end of the current file. */
  word = get_word (f);
  if (extract && left (block[5]) != 0654644)
    {
      open_file (directory, name, ext);
      write_data (block + 0101, size - 0101);
      if (!data_record (word))
	close_file (block[size - 1]);
    }

  return word;
}

static word_t
process_data (FILE *f, word_t word)
{
  int size;

#if 0
  /* The file TUSR2.ARC on 169267.tape has a mark in the middle of data. */
  if (word & START_FILE)
    return word;
  expect_record (word);
#else
  expect_file_or_record (word);
#endif
  if (!data_record (word))
    return word;

  size = right (word);
  check_block_size (size);
  get_block (f, block, size);

  /* Check the next record to see if this is the end of the current file. */
  word = get_word (f);
  if (extract)
    write_data (block, size);

  if (!data_record (word) && extract)
    close_file (block[size - 1]);
  return word;
}

static word_t
process_file (FILE *f, word_t word)
{
  word = process_file_header (f, word);
  while (data_record (word))
    word = process_data (f, word);
  return word;
}

static word_t
process_user (FILE *f, word_t word)
{
  expect_file (word);
  for (;;)
    {
      word = process_file (f, word);
      if (saveset_record (word))
	return word;
    }
}

static void
process_saveset (FILE *f)
{
  word_t word = get_word (f);
  if (word == -1)
    exit (0);
  fprintf (info, "SAVESET\n");
  process_header (f, word, 0);
  word = get_word (f);
  while (file_record (word))
    word = process_user (f, word);
  process_header (f, word, 1);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s -t|-x [-v] [-f file]\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  FILE *f = NULL;
  int opt;

  input_word_format = &tape_word_format;
  output_word_format = &aa_word_format;

  if (argc == 1)
    usage (argv[0]);

  while ((opt = getopt (argc, argv, "tvxf:W:")) != -1)
    {
      switch (opt)
	{
	case 'f':
	  if (f != NULL)
	    {
	      fprintf (stderr, "Just one -f allowed.\n");
	      exit (1);
	    }
	  f = fopen (optarg, "rb");
	  if (f == NULL)
	    {
	      fprintf (stderr, "Error opening input %s: %s\n",
		       optarg, strerror (errno));
	      exit (1);
	    }
	  break;
	case 't':
	  verbose++;
	  break;
	case 'v':
	  verbose++;
	  break;
	case 'x':
	  extract = 1;
	  break;
	default:
	  usage (argv[0]);
	}
    }

  if (f == NULL)
    f = stdin;

  list = info = stdout;
  if (verbose == 0)
    list = info = fopen ("/dev/null", "w");
  else if (verbose == 1)
    info = fopen ("/dev/null", "w");

  for (;;)
    process_saveset (f);

  return 0;
}
