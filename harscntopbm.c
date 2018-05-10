/* Copyright (C) 2018 Adam Sampson <ats@offog.org>

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

/* Read "Harvard scan" files, used at MIT for Gould printer-plotters,
   and output a PBM file. The format is documented in VERSA 210.

   (It's not the same as NetPBM's gouldtoppm, which is a colour
   format.) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dis.h"

/* Page width in bits -- 11" */
#define WIDTH 2112
/* Page height in bits -- "approximately 1700" */
#define HEIGHT 1700

static int even = 1;
static word_t word;

/* Two PDP-11 words per PDP-10 word, right-justified in each halfword. */
static unsigned short get_next (FILE *f)
{
  unsigned short r;

  if (even)
    {
      word = get_word (f);
      r = (word >> 18) & 0xFFFF;
    }
  else
    {
      r = word & 0xFFFF;
    }

  even = 1 - even;
  return r;
}

static int xpos;
static int ypos;
static unsigned char buf[HEIGHT][WIDTH / 8];
static int want_eject = 0;

static void clear_page ()
{
  xpos = 0;
  ypos = 0;

  memset (buf, 0, sizeof buf);
}

static void eject_page (int last)
{
  int y;

  if (last && xpos == 0 && ypos == 0)
    return;

  printf ("P4\n");
  printf ("%d %d\n", WIDTH, HEIGHT);
  for (y = 0; y < HEIGHT; y++)
    {
      fwrite (&buf[y], sizeof (buf[0][0]), WIDTH / 8, stdout);
    }
  fflush (stdout);

  clear_page ();
}

static void out_bit (int b)
{
  buf[ypos][xpos / 8] |= b << (7 - (xpos % 8));

  ++xpos;
  if (xpos == WIDTH)
    {
      xpos = 0;
      ++ypos;

      if (want_eject || ypos == HEIGHT)
	{
	  eject_page (0);
	  want_eject = 0;
	}
    }
}

static void out_bits (int b, int count)
{
  int i;

  for (i = 0; i < count; i++)
    out_bit (b);
}

int convert (FILE *f)
{
  clear_page ();

  while (1)
    {
      unsigned short w;
      int i;

      w = get_next (f);
      if (feof (f))
	break;

      switch (w)
	{
	case 0:
	  /* Print all-0s words. */
	  w = get_next (f);
	  out_bits (0, w * 16);
	  break;

	case 0177777:
	  w = get_next (f);
	  switch (w)
	    {
	    case 0177776:
	      /* Form feed at end of current line.
		 In practice, VERSA emits this at the end of the line
		 anyway. */
	      if (xpos == 0)
		eject_page (0);
	      else
		want_eject = 1;
	      break;

	    case 0177775:
	      /* Print blank line.
		 The documentation says this isn't implemented, and
		 doesn't say whether it's "fill to end of line" or
		 "print a line's worth". */
	      out_bits (1, WIDTH);
	      break;

	    case 0177774:
	      /* Print blank lines.
	         As above. */
	      w = get_next (f);
	      out_bits (1, w * WIDTH);
	      break;

	    default:
	      /* Print all-1s words. */
	      out_bits (1, w * 16);
	      break;
	    }
	  break;

	default:
	  for (i = 15; i >= 0; i--)
	    {
	      out_bit ((w >> i) & 1);
	    }
	  break;
	}
    }

  eject_page (1);

  return 0;
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-W<word format>] [<file> ...]\n\n", argv[0]);
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  int opt, i;

  while ((opt = getopt (argc, argv, "W:")) != -1)
    {
      switch (opt)
	{
	case 'W':
	  if (parse_word_format (optarg))
	    usage (argv);
	  break;
	default:
	  usage (argv);
	}
    }

  if (optind >= argc)
    convert (stdin);
  else
    for (i = optind; i < argc; i++)
      {
	FILE *f = fopen (argv[i], "rb");
	if (f == NULL)
	  {
	    fprintf (stderr, "Cannot open %s\n", argv[i]);
	    exit (1);
	  }

	convert (f);
	fclose (f);
      }

  return 0;
}
