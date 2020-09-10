/* Copyright (C) 2009 Lars Brinkhoff <lars@nocrew.org>

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

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_7track_word (FILE *f)
{
  word_t word;

  if (feof (f))
    return -1;

  word = (((word_t)get_byte (f) & 077) << 30) |
         (((word_t)get_byte (f) & 077) << 24) |
         (((word_t)get_byte (f) & 077) << 18) |
         (((word_t)get_byte (f) & 077) << 12) |
         (((word_t)get_byte (f) & 077) <<  6) |
          ((word_t)get_byte (f) & 077);

  return word;
}

static void
write_7track_word (FILE *f, word_t word)
{
  int i, c, p;
  
  for (i = 0; i < 6; i++)
    {
      c = (word >> 30) & 077;
      p = 0100 ^ (c << 1) ^ (c << 2) ^ (c << 3) ^ (c << 4) ^ (c << 5) ^ (c << 6);
      c |= p & 0100;
      fputc (c, f);
      word <<= 6;
    }
}

static int get_reclen (FILE *f)
{
  return get_byte (f) |
    ((word_t)get_byte (f) << 8) |
    (get_byte (f) << 16) |
    (get_byte (f) << 24);
}

static void write_reclen (FILE *f, int n)
{
  fputc (n & 0377, f);
  fputc ((n >> 8) & 0377, f);
  fputc ((n >> 16) & 0377, f);
  fputc ((n >> 24) & 0377, f);
}

int get_9track_record (FILE *f, word_t **buffer)
{
  int i, x, reclen;
  word_t *p;

  reclen = get_reclen (f);
  if (reclen == 0)
    return 0;
  if (reclen % 5)
    {
      fprintf (stderr, "Not a CORE DUMP tape image.\n"
	       "reclen = %d\n", reclen);
      exit (1);
    }
  
  *buffer = malloc (sizeof (word_t) * (reclen/5));
  if (*buffer == NULL)
    {
      fprintf (stderr, "Out of memory.\n");
      exit (1);
    }

  for (i = 0, p = *buffer; i < (reclen / 5); i++)
    *p++ = get_core_word (f);

  /* First try the E-11 tape format. */
  x = get_reclen (f);
  if (x != reclen)
    {
      /* Next try the SIMH tape format. */
      if (reclen & 1)
	x = (x >> 8) + (get_byte (f) << 24);

      if (x != reclen)
	{
	  fprintf (stderr, "Error in tape image format.\n"
		   "%d != %d\n", reclen, x);
	  exit (1);
	}
    }

  if (reclen == 0)
    {
      free (*buffer);
    }

  return reclen / 5;
}

int get_9track_rec (FILE *f, char **buffer)
{
  int i, x, reclen;
  char *p;

  reclen = get_reclen (f);
  if (reclen == 0)
    return 0;
  
  *buffer = malloc (reclen);
  if (*buffer == NULL)
    {
      fprintf (stderr, "Out of memory.\n");
      exit (1);
    }

  for (i = 0, p = *buffer; i < reclen; i++)
    *p++ = get_byte (f);

  /* First try the E-11 tape format. */
  x = get_reclen (f);
  if (x != reclen)
    {
      /* Next try the SIMH tape format. */
      if (reclen & 1)
	x = (x >> 8) + (get_byte (f) << 24);

      if (x != reclen)
	{
	  fprintf (stderr, "Error in tape image format.\n"
		   "%d != %d\n", reclen, x);
	  exit (1);
	}
    }

  if (reclen == 0)
    {
      free (*buffer);
    }

  return reclen;
}

void write_7track_record (FILE *f, word_t *buffer, int n)
{
  int i;

  write_reclen (f, 6 * n);
  if (n == 0)
    return;
  
  for (i = 0; i < n; i++)
    write_7track_word (f, *buffer++);

  write_reclen (f, 6 * n);
}

void write_9track_record (FILE *f, word_t *buffer, int n)
{
  int i;

  /* To write a tape record in the SIMH tape image format, first write
     a 32-bit record length, then data frames, then the length again.
     For PDP-10 36-bit data, the data words are written in the "core
     dump" format.  One word is written as five 8-bit frames, with
     four bits unused in the last frame. */

  write_reclen (f, 5 * n);

  /* A record of length zero is a tape mark, and the length is only
     written once. */
  if (n == 0)
    return;
  
  for (i = 0; i < n; i++)
    write_core_word (f, *buffer++);

  /* Pad out to make the record data an even number of octets. */
  if ((n * 5) & 1)
    fputc (0, f);

  write_reclen (f, 5 * n);
}

int get_7track_record (FILE *f, word_t **buffer)
{
  int i, x, reclen;
  word_t *p;

  reclen = get_reclen (f);
  if (reclen == 0)
    return 0;
  if (reclen % 6)
    {
      fprintf (stderr, "Not a 7-track tape image.\n"
	       "reclen = %d\n", reclen);
      exit (1);
    }
  
  *buffer = malloc (sizeof (word_t) * (reclen/6));
  if (*buffer == NULL)
    {
      fprintf (stderr, "Out of memory.\n");
      exit (1);
    }

  for (i = 0, p = *buffer; i < (reclen / 6); i++)
    *p++ = get_7track_word (f);

  x = get_reclen (f);
  if (x != reclen)
    {
      fprintf (stderr, "Error in tape image format.\n"
	       "%d != %d\n", reclen, x);
      exit (1);
    }

  if (reclen == 0)
    {
      free (*buffer);
    }

  return reclen / 6;
}

static word_t *buffer = NULL;
int n, words;
int end_of_file = 1;
int end_of_tape = 0;

static word_t
get_tape_word (FILE *f)
{
  word_t word;

  if (end_of_tape)
    return -1;

  if (buffer == NULL)
    {
      if (input_word_format == &tape_word_format)
	words = get_9track_record (f, &buffer);
      else
	words = get_7track_record (f, &buffer);
      if (words == 0)
	{
	  end_of_file = 1;
	  if (input_word_format == &tape_word_format)
	    words = get_9track_record (f, &buffer);
	  else
	    words = get_7track_record (f, &buffer);
	  if (words == 0)
	    {
	      end_of_tape = 1;
	      return -1;
	    }
	}
      n = 0;
    }

  word = buffer[n++];

  if (end_of_file)
    {
      word |= START_FILE;
      end_of_file = 0;
    }
  else if (n == 1)
    word |= START_RECORD;

  if (n == words)
    {
      free (buffer);
      buffer = NULL;
    }

  return word;
}

static void
rewind_tape_word (FILE *f)
{
  if (buffer != NULL)
    free (buffer);
  end_of_file = 1;
  end_of_tape = 0;
  buffer = NULL;
  rewind (f);
}

struct word_format tape_word_format = {
  "tape",
  get_tape_word,
  rewind_tape_word,
  NULL,
  NULL
};

struct word_format tape7_word_format = {
  "tape7",
  get_tape_word,
  rewind_tape_word,
  NULL,
  NULL
};
