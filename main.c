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
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "dis.h"
#include "opcode/pdp10.h"
#include "memory.h"

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-r] <file>\n", argv[0]);
  exit (1);
}

int
main (int argc, char **argv)
{
  int cpu_model = PDP10_KS10_ITS;
  struct pdp10_memory memory;
  FILE *file;
  word_t word;
  int opt;
  int raw_format = 0;

  while ((opt = getopt (argc, argv, "r")) != -1)
    {
      switch (opt)
	{
	case 'r':
	  raw_format = 1;
	  break;
	default:
	  usage (argv);
	}
    }

  if (optind != argc - 1)
    usage (argv);

  file = fopen (argv[optind], "rb");
  if (file == NULL)
    {
      fprintf (stderr, "%s: Error opening %s: %s\n",
	       argv[0], argv[optind], strerror (errno));
      return 1;
    }

  init_memory (&memory);

  word = get_word (file);
  rewind_word (file);
  if (raw_format)
    read_raw (file, &memory, cpu_model);
  else if (word == 0)
    read_pdump (file, &memory, cpu_model);
  else
    read_sblk (file, &memory, cpu_model);

  while ((word = get_word (file)) != -1)
    printf ("(extra word: %012llo)\n", word);

  printf ("\nDisassembly:\n\n");
  dis (&memory, cpu_model);

  return 0;
}
