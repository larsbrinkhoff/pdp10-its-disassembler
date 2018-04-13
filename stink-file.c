/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>

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

#include "dis.h"
#include "memory.h"

static word_t checksum = 0;

static word_t
get_stink_checksummed_word (FILE *f)
{
  word_t word = get_word (f);

  if (word < 0)
    return word;

  checksum += word;
  if (checksum & 01000000000000ULL)
    {
      checksum++;
      checksum &= 0777777777777ULL;
    }

  return word;
}

static void
check_stink_checksum (FILE *f)
{
  word_t word = get_word (f);
  checksum = ~checksum & 0777777777777LL;
  if (word != -1 && checksum != word)
    printf ("  Bad checksum: %012llo /= %012llo\n", word, checksum);
}

static word_t block[128];

static void
standard_data (int length)
{
  word_t codes, data, value = 0;
  int i, j, code;
  

  for (i = 0; i < length; )
    {
      codes = block[i++];
      for (j = 33; j > 0; j -= 3)
        {
          code = (codes >> j) & 7;

          if (code == 7)
            {
              j -= 3;
              code = 010 + (codes & 7);
            }

          switch (code)
            {
            case 000: // do not relocate word
              value = 0;
              break;
            case 001: // relocate right half
              value = 0;
              break;
            case 002: // relocate left half
              value = 0;
              break;
            case 003: // relocate both halves
              value = 0;
              break;
            case 004: break; // global symbol
            case 005: break; // minus global symbol
            case 006: break; // link
            case 010: break; // define symbol
            case 011: break; // COMMON
            case 012: break; // local to global recovery
            case 013: break; // library request
            case 014: break; // redefine symbol
            case 015: break; // global multiplied by n
            case 016: break; // define symol as .
            case 017: break; // illegal
            }
        }
    }
}

static void local_symbols (int count)
{
  int i;

  for (i = 0; i < count; i += 2)
    {
      char str[7];
      squoze_to_ascii (block[i], str);
      printf ("    %s = %12o (local)\n", str, block[i+1]);
    }
}

static void global_symbols (int count)
{
  int i;

  for (i = 1; i < count; i += 2)
    {
      char str[7];
      squoze_to_ascii (block[i], str);
      printf ("    %s = %12o (global)\n", str, block[i+1]);
    }
}

void
read_stink (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int address;
  word_t word;
  int i;

  printf ("Stinking REL format\n");

  address = 0;
  while ((word = get_word (f)) != -1)
    {
      int eof = (word >> 35) & 1;
      int lcb = (word >> 32) & 7;
      int type = (word >> 25) & 0177;
      int count = (word >> 18) & 0177;
      int adr = word & 0777777;
      char str[7];

      checksum = word;

      //printf ("Block type %o, address %o\n", type, adr);
      if (eof)
        printf ("  End Of File\n");
      else
        {
          for (i = 0; i < count; i++)
            {
              block[i] = get_stink_checksummed_word (f);
              if (block[i] < 0)
                break;
              //printf ("  %012llo\n", block[i]);
            }


          switch (type)
            {
            case 000: printf ("  Type: illegal\n"); break;
            case 001: printf ("  Type: loader command\n"); break;
            case 002: printf ("  Type: code (absolute)\n"); break;
            case 003: printf ("  Type: code (relocated)\n"); break;
            case 004: 
              squoze_to_ascii (block[0], str);
              printf ("  Program name: %s\n", str);
              break;
            case 005: printf ("  Type: library search\n"); break;
            case 006: printf ("  Type: COMMON\n"); break;
            case 007: printf ("  Type: global parameter assignment\n"); break;
            case 010:
              local_symbols (count);
              break;
            case 011: printf ("  Type: load time conditional\n"); break;
            case 012: printf ("  Type: end load time conditional\n"); break;
            case 013: printf ("  Type: local symbols to be half-killed\n"); break;
            case 014: printf ("  Type: end of program (for libraries)\n"); break;
            case 015: printf ("  Type: entries\n"); break;
            case 016: printf ("  Type: external references\n"); break;
            case 017: printf ("  Type: load if needed\n"); break;
            case 020:
              global_symbols (count);
              break;
            case 021:
              printf ("  Type: fixups\n");
              standard_data (count);
              break;
            case 022: printf ("  Type: polish fixups\n"); break;
            default: printf ("  Type: unknown\n"); break;
            }
        }

      check_stink_checksum (f);

#if 0
      word_t *data = malloc (5);
      if (data == NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (1);
	}

      *data = word;
      add_memory (memory, address++, 1, data);
#endif
    }
}

struct file_format stink_file_format = {
  "stink",
  read_stink,
  NULL
};
