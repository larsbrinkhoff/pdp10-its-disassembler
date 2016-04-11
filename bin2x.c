/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>

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
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct buf
{
  FILE *file;
  int bits;
  int data;
};

void
init_buf (struct buf *buf, FILE *f)
{
  buf->file = f;
  buf->bits = 0;
}

void
fill_buf (struct buf *buf, int bits_wanted)
{
  while (buf->bits < bits_wanted)
    {
      int c = fgetc (buf->file);
      if (c == EOF)
	exit (0);
      buf->data = (buf->data << 8) | c;
      buf->bits += 8;
    }
}

int
get_bits (struct buf *buf, int bits_wanted)
{
  int bits;
  int mask;

  fill_buf (buf, bits_wanted);

  mask = (1 << bits_wanted) - 1;
  bits = (buf->data >> (buf->bits - bits_wanted)) & mask;
  buf->bits -= bits_wanted;

  return bits;
}

int
main (int argc, char **argv)
{
  static const int bits_in_byte[5] = { 4, 8, 8, 8, 8 };
  int byte_in_word;
  struct buf buf;
  int bits, data;
  FILE *f;

  if (argc == 2)
    {
      f = fopen (argv[1], "rb");
      if (f == NULL)
	{
	  fprintf (stderr, "error opening file %s: %s",
		   argv[1], strerror (errno));
	}
    }
  else
    f = stdin;

  init_buf (&buf, f);
  byte_in_word = 0;

  while (!feof (f))
    {
      bits = bits_in_byte[byte_in_word];
      data = get_bits (&buf, bits);

      putchar (data);

      byte_in_word++;
      if (byte_in_word == 5)
	byte_in_word = 0;
    }

  return 0;
}
