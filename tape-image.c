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

#include <stdlib.h>
#include <setjmp.h>
#include "tape-image.h"

static int big_endian = 0;
static jmp_buf jb;

static void
exit_at_eot (void)
{
  printf ("Error reading tape image\n");
  exit (1);
}

static void (*eot) (void) = exit_at_eot;

void end_of_tape (void (*fn) (void))
{
  eot = fn;
}

static void
read_octets (FILE *f, uint8_t *buffer, int n)
{
  int c, i;

  for (i = 0; i < n; i++)
    {
      c = fgetc (f);
      if (c == EOF)
        longjmp (jb, 1);
      *buffer++ = c;
    }
}

static uint32_t
swap (uint32_t x)
{
  return ((x >> 24) & 0x000000FF) |
         ((x >>  8) & 0x0000FF00) |
         ((x <<  8) & 0x00FF0000) |
         ((x << 24) & 0xFF000000);
}

static uint32_t
read_16bits_l (uint8_t *start)
{
  return start[0] | (start[1] << 8);
}

static uint32_t
read_32bits_l (uint8_t *start)
{
  return read_16bits_l (start) | (read_16bits_l (start + 2) << 16);
}

static uint32_t
read_reclen (uint8_t *start)
{
  uint32_t x;
  x = read_32bits_l (start);
  if (big_endian)
    x = swap (x);
  return x;
}

uint32_t
read_record (FILE *f, uint8_t *buffer, uint32_t n)
{
  uint8_t size[5];
  uint32_t len, len2;

  if (setjmp (jb) != 0)
    eot ();

  read_octets (f, size, 4);
  len = read_reclen (size);
  if (len == 0)
    return len;
  if (len == 0xFFFFFFFF)
    return len;
  if ((len >> 24) == 0x80)
    return len;

  if (len > 100000)
    {
      len = swap (len);
      big_endian = !big_endian;
    }

  if ((len & 0x80000000) != 0)
    return len;
  if (len > 100000)
    {
      printf ("Bad record size: %u %x\n", len, len);
      exit (1);
    }

  if (len > 0)
    {
      if (len > n)
        {
          printf ("Buffer too small.\n");
          exit (1);
        }

      read_octets (f, buffer, len);
      read_octets (f, size, 4);
      len2 = read_reclen (size);
      if (len != len2)
        {
          if (len & 1) {
            read_octets (f, size + 4, 1);
            len2 = read_reclen (size + 1);
          }
          if (len != len2)
            {
              printf ("Size mismatch\n");
              printf ("Record size: %u %x\n", len, len);
              printf ("Second size: %u %x\n", len2, len2);
              exit (1);
            }
        }
    }

  return len;
}
