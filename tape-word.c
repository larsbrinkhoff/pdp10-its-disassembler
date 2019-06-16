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

extern word_t get_core_word (FILE *f);

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static int get_reclen (FILE *f)
{
  return get_byte (f) |
    ((word_t)get_byte (f) << 8) |
    (get_byte (f) << 16) |
    (get_byte (f) << 24);
}

static int get_record (FILE *f, word_t **buffer)
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

  if (reclen & 1)
    get_byte (f);

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

  return reclen / 5;
}

static word_t *buffer = NULL;
int n, words;
int end_of_file = 1;
int end_of_tape = 0;

word_t
get_tape_word (FILE *f)
{
  word_t word;

  if (end_of_tape)
    return -1;

  if (buffer == NULL)
    {
      words = get_record (f, &buffer);
      if (words == 0)
	{
	  end_of_file = 1;
	  words = get_record (f, &buffer);
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

void
rewind_tape_word (FILE *f)
{
  if (buffer != NULL)
    free (buffer);
  end_of_file = 1;
  end_of_tape = 0;
  buffer = NULL;
  rewind (f);
}
