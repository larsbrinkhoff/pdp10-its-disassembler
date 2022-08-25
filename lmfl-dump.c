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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static uint8_t buffer[1024 * 1024];

static uint8_t
read_frame (void)
{
  int c;
  c = getchar ();
  if (c == EOF)
    {
      printf ("Physical end of tape.\n");
      exit (0);
    }
  return c;
}

static uint32_t
read_size (void)
{
  uint8_t x1, x2, x3, x4;
  x1 = read_frame ();
  x2 = read_frame ();
  x3 = read_frame ();
  x4 = read_frame ();
  return x1 | (x2 << 8) | (x3 << 16) | (x4 << 24);
}

static uint32_t
read_record (uint8_t *buffer)
{
  uint32_t i, size;

  size = read_size ();
  if (size & 0x80000000)
    return size;
  else if (size > 0)
    {
      for (i = 0; i < size; i++)
        *buffer++ = read_frame ();
      if (size != read_size ())
        {
          printf ("Error in image.\n");
          exit (1);
        }
    }

  return size;
}

static void
read_header (void)
{
  uint32_t len;
  len = read_record (buffer);
  if (len == 0)
    {
      printf ("Logical end of tape.\n");
      exit (0);
    }
  if (len & 0x80000000)
    {
      printf ("Tape error.\n");
      return;
    }
  buffer[len] = 0;
  printf ("Header: %s\n", buffer);
}

static void
read_file (void)
{
  read_header ();

  for (;;)
    {
      uint32_t n = read_record (buffer);
      if (n == 0)
        {
          printf ("Tape mark.\n");
          return;
        }
      if (n & 0x80000000)
        {
          printf ("Tape error.\n");
          return;
        }
      else
        {
          printf ("Data: %u octets.\n", n);
        }
    }
}

int
main (void)
{
  for (;;)
    read_file ();
}
