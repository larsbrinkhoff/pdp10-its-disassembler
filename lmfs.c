/* Copyright (C) 2020 Lars Brinkhoff <lars@nocrew.org>

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
#include <string.h>

#include "dis.h"

static char *buffer;

extern int get_9track_rec (FILE *, char **);

static void read_info (void)
{
  char *p, *x;

  x = strchr (buffer, 0215);
  *x = 0;
  p = x + 1;
  if ((x = strchr (buffer, ' ')) != NULL)
    {
      *x = 0;
      fprintf (stderr, "%s\n", x + 1);
    }
  for (;;)
    {
      x = strchr (p, 0215);
      *x = 0;
      //fprintf (stderr, " > %s\n", p);
      if (strncmp (p, "END", 3) == 0)
        break;
      p = x + 1;
    }
}

static void read_eof (FILE *f)
{
  int n;
  while ((n = get_9track_rec (f, &buffer)) > 0)
    {
      free (buffer);
    }
}

static void read_prelude (FILE *f, int n)
{
  //fprintf (stderr, "Prelude:\n");
  read_info ();
  free (buffer);
  read_eof (f);
}

static void read_directory (FILE *f, int n)
{
  //fprintf (stderr, "Directory:\n");
  read_info ();
  free (buffer);
  read_eof (f);
}

static void read_file (FILE *f, int n)
{
  //fprintf (stderr, "File:\n");
  read_info ();
  free (buffer);
  read_eof (f);
}

static void read_tape (FILE *f)
{
  int n;
  n = get_9track_rec (f, &buffer);
  if (n == 0)
    exit (0);
  else if (strncmp (buffer, "PRELUDE", 7) == 0)
    read_prelude (f, n);
  else if (strncmp (buffer, "DIRECTORY", 9) == 0)
    read_directory (f, n);
  else if (strncmp (buffer, "FILE", 4) == 0)
    read_file (f, n);
  else
    fprintf (stderr, "UNKNOWN: %s\n", buffer);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s ...\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  FILE *f;

  if (argc != 2)
    usage (argv[0]);

  f = fopen (argv[1], "rb");
  if (f == NULL)
    {
      fprintf (stderr, "error\n");
      exit (1);
    }

  for (;;)
    read_tape (f);

  return 0;
}
