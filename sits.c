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

#include <stdio.h>

#include "dis.h"

#define BLOCK_OCTETS 1024

/* Directory entry types. */
#define PRENT   000
#define DIREC   002
#define FILEX   004
#define LAST    006	
#define SELF    010
#define LINK    012
#define ALFORQ  014
#define BITS    016

/* Directory entry flags. */
#define MFISTB      1
#define TYPEM 0177761
#define SHRB     0020
#define ACCB     0100
#define EOFB     0200


typedef unsigned char octet;

octet image[2436 * BLOCK_OCTETS];
int blocks;

int directories[100];
int *dir = directories;

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? -1 : c;
}

static int read_block (FILE *f, octet *buffer)
{
  int i;

  for (i = 0; i < BLOCK_OCTETS; i++)
    {
      int x = get_byte (f);
      if (x == -1)
	return -1;
      buffer[i] = x;
    }

  return 0;
}

static octet *
get_block (int block)
{
  return &image[BLOCK_OCTETS * block];
}

static int
get_pdp11_word (octet *buffer)
{
  return (buffer[1] << 8) + buffer[0];
}

static void
show_block (octet *block)
{
  int i, j;

  for (i = 0; i < 512; i += 8 * 2)
    {
      printf ("%06o %06o %06o %06o %06o %06o %06o %06o ",
              get_pdp11_word (block + i),
              get_pdp11_word (block + i + 2),
              get_pdp11_word (block + i + 4),
              get_pdp11_word (block + i + 6),
              get_pdp11_word (block + i + 8),
              get_pdp11_word (block + i + 10),
              get_pdp11_word (block + i + 12),
              get_pdp11_word (block + i + 14));
      for (j = 0; j < 16; j++)
        {
          if (block[i+j] < 040 || block[i+j] > 0176)
            putchar ('.');
          else
            putchar (block[i+j]);
        }
      putchar ('\n');
    }
}

static void
show_flags (octet flags)
{
  printf ("Flags %03o ", flags);
  switch (flags & ~TYPEM)
    {
    case PRENT: printf ("parent "); break;
    case DIREC: printf ("directory "); break;
    case FILEX: printf ("file "); break;
    case LAST: printf ("(last) "); break;
    case SELF: printf ("self "); break;
    case LINK: printf ("link "); break;
    case ALFORQ: printf ("alforq "); break;
    case BITS: printf ("bits "); break;
    }
  if (flags & MFISTB)
    printf ("exists ");
  if (flags & SHRB)
    printf ("shared ");
  if (flags & ACCB)
    printf ("access ");
  if (flags & EOFB)
    printf ("eof ");
  putchar ('\n');
}

static int
show_name (octet *block, int i)
{
  printf ("Name: ");
  do
    putchar (block[i] & 0177);
  while ((block[i++] & 0200) == 0);
  putchar ('\n');
  return i;
}

static void
show_directory (octet *block)
{
  int flags;
  int dirend;
  int i = 0, j;
  int n;

  dirend = get_pdp11_word (block + 6);
  printf ("Directory ------------------\n");
  printf ("Dirend %06o\n", dirend);

  while (i < dirend)
    {
      n = block[i];
      flags = block[i+1];
      printf ("Number of bytes %03o\n", n);
      show_flags (flags);

      j = i + 2;
      printf ("Something %d\n", get_pdp11_word (block + j + 2));
      j += 2;
      if (flags & EOFB)
	{
	  printf ("Date %d\n", get_pdp11_word (block + j));
	  j += 2;
	  printf ("End of file %06o\n", get_pdp11_word (block + j));
	  j += 2;
	  printf ("Something else %d %d\n", get_pdp11_word (block + j), get_pdp11_word (block + j + 2));
	  j += 4;
	}

      if (flags & ACCB)
	{
	  do
	    j +=3;
	  while ((block[j-1] & 0200) == 0);
	}

      j = show_name (block, j);

      if ((flags & ~TYPEM) == DIREC)
	*++dir = block[j+1];

      n = (n + 1) & ~1;
      i += n;
      putchar ('\n');
    }

  while (dir > directories)
    {
      --dir;
      show_directory(get_block (dir[1]));
    }
}

int
main (int argc, char **argv)
{
  octet *buffer;
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
      buffer += BLOCK_OCTETS;
      blocks++;
    }

  fprintf (stderr, "%o blocks in image\n", blocks);

  //show_block (get_block (046));
  show_directory (get_block (046));

  return 0;
}
