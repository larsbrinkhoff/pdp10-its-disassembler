/* Copyright (C) 2020 Adam Sampson <ats@offog.org>

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
#include <string.h>

#include "dis.h"
#include "memory.h"
#include "symbols.h"

/* Muddle's page map uses the ITS page size, even on TOPS-20. */
#define MDL_PAGESIZE ITS_PAGESIZE

static void
word_to_ascii7 (word_t w, char *string)
{
  string[0] = (w >> 29) & 0177;
  string[1] = (w >> 22) & 0177;
  string[2] = (w >> 15) & 0177;
  string[3] = (w >>  8) & 0177;
  string[4] = (w >>  1) & 0177;
  string[5] = 0;
}

static void
strip_spaces (char *string)
{
  char *p = string + strlen (string) - 1;

  while (p >= string && *p == ' ')
    *p-- = 0;
}

static void
add_global (const char *name, word_t value)
{
  printf ("    Symbol %-6s = %llo\n", name, value);

  add_symbol (name, value, SYMBOL_GLOBAL);
}

/* Add a subset of the symbols from a given version of the Muddle
   interpreter. Some of these are necessary in order to restore the
   page map; others just help to make RSUBR code more readable. */
static void
define_mdl_symbols (const char *version)
{
  /* These are the same in all versions. */
  add_global ("a", 01);
  add_global ("b", 02);
  add_global ("c", 03);
  add_global ("d", 04);
  add_global ("e", 05);
  add_global ("pvp", 06);
  add_global ("tvp", 07);
  add_global ("sp", 010);
  add_global ("ab", 011);
  add_global ("tb", 012);
  add_global ("tp", 013);
  add_global ("frm", 014);
  add_global ("m", 015);
  add_global ("r", 016);
  add_global ("p", 017);
  add_global ("hibot", 0700000);

  /* XXX Add helpers used by RSUBR code: FINIS, MPOPJ... */
  switch (atoi (version))
    {
    case 54:
      /* TS MUD54 from MIT */
      add_global ("purtop", 0123);
      add_global ("pmapb", 0126);
      add_global ("globsp", 01364);
      add_global ("glotop", 01372);
      break;

    case 104:
      /* mdl104.exe from UChicago/LCM+L */
      add_global ("purtop", 0163);
      add_global ("pmapb", 0166);
      add_global ("globsp", 01474);
      add_global ("glotop", 01502);
      break;

    case 105:
      /* mdl105.exe from Panda and LCM+L */
      add_global ("purtop", 0165);
      add_global ("pmapb", 0170);
      add_global ("globsp", 01674);
      add_global ("glotop", 01666);
      break;

    case 56:
      /* TS MUD56 built in ITS repo */
      add_global ("purtop", 0167);
      add_global ("pmapb", 0175);
      add_global ("globsp", 01566);
      add_global ("glotop", 01574);
      break;

    case 106:
      /* mdl106.exe from LCM+L */
      add_global ("purtop", 0217);
      add_global ("pmapb", 0222);
      add_global ("globsp", 01677);
      add_global ("glotop", 01705);
      break;

    default:
      fprintf (stderr, "Unsupported Muddle version\n");
      exit (1);
    }
}

static void
load_to_memory (FILE *f, struct pdp10_memory *memory,
		int address, int length, const char *description)
{
  word_t *data;
  int i;

  data = malloc (length * sizeof *data);
  if (data == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }

  for (i = 0; i < length; i++)
    data[i] = get_word (f);

  if (data[length - 1] == -1)
    {
      fprintf (stderr, "End of file during %s\n", description);
      exit (1);
    }

  add_memory (memory, address, length, data);
}

static void
read_mdl (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  (void) cpu_model; /* Not used */

  word_t word;
  char version[6];
  word_t partop_v, purtop_v, hibot, pmapb;
  int i;

  printf ("Muddle save format\n\n");

  word_to_ascii7 (get_word (f), version);
  strip_spaces (version);
  printf ("Muddle version: \"%s\"\n\n", version);

  printf ("Interpreter symbols:\n");
  define_mdl_symbols (version);

  word = get_word (f);
  printf ("\nValue of p.top  = %llo\n", word);

  word = get_word (f);
  if (word != 0)
    {
      /* This would be VECTOP in the old format. */
      fprintf (stderr, "Muddle slow save format not supported\n");
      exit (1);
    }
  printf ("Fast save format\n");

  word = get_word (f);
  printf ("Value of vectop = %llo\n", word);

  partop_v = get_word (f);
  printf ("Value of partop = %llo\n", partop_v);

  /* Impure memory, from location 5 to partop. */
  load_to_memory (f, memory, 5, partop_v - 5, "impure memory");

  purtop_v = get_word_at (memory, get_symbol_value ("purtop"));
  hibot = get_symbol_value ("hibot");
  pmapb = get_symbol_value ("pmapb");

  /* Pure memory in the page map. Only pages that are marked in the
     page map as purified are written out. The page map has two bits
     per page. */
  printf ("\nPage map:\n");
  for (i = purtop_v / MDL_PAGESIZE; i < (hibot / MDL_PAGESIZE); i++)
    {
      word_t entry, mask;
      int purified;

      entry = get_word_at (memory, pmapb + (i / 16));
      mask = (1ll << 35) >> ((2 * (i % 16)) + 1);
      purified = (entry & mask) != 0;
      printf ("Page %03o: %s\n", i, purified ? "pure" : "not pure");

      if (purified)
	{
	  load_to_memory (f, memory, i * MDL_PAGESIZE, MDL_PAGESIZE,
			  "pure pages");
	}
    }

  printf ("\n");
}

struct file_format mdl_file_format = {
  "mdl",
  read_mdl,
  NULL
};
