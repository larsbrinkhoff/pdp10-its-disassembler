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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "memory.h"
#include "symbols.h"

#define NFASL 0124641635413LL  /* *FASL+ */
#define OFASL 0124641635412LL  /* *FASL* */

static int atom_index = 1;
static char *atomtable[1000];
static int type[1000];

static int address; //Current absolute address to load into.
static int offset;  //Relocation offset.

static int
fasl_magic (word_t word)
{
  return word == NFASL || word == OFASL;
}

static void
read_atomtable (FILE *f, word_t header)
{
  char string[100];
  int length;
  word_t data;
  char *p;

  //fprintf (stderr, "Atomtable type %llo:  ", (header >> 33) & 7);
  type[atom_index] = (header >> 33) & 7;
  switch (type[atom_index])
    {
    case 0: //Atom.
      p = string;
      length = header & 0777777;
      while (length--)
        {
          data = get_word (f);
          *p++ = (data >> 29) & 0177;
          *p++ = (data >> 22) & 0177;
          *p++ = (data >> 15) & 0177;
          *p++ = (data >>  8) & 0177;
          *p++ = (data >>  1) & 0177;
        }
      //fprintf (stderr, "Atom index %d pname \"%s\"\n", atom_index, string);
      atomtable[atom_index++] = strdup (string);
      string[5] = 0;
      break;
    case 1: //Fixnum.
      data = get_word (f);
      //fprintf (stderr, "Atom index %d fixnum %012llo\n", atom_index, data);
      snprintf (string, sizeof string, "Fixnum %012llo", data);
      atomtable[atom_index++] = strdup (string);
      break;
    case 2: //Flonum.
      data = get_word (f);
      //fprintf (stderr, "Atom index %d flonum %012llo\n", atom_index, data);
      snprintf (string, sizeof string, "Flonum %012llo", data);
      atomtable[atom_index++] = strdup (string);
      break;
    case 3: //Bignum.
      length = header & 0777777;
      while (length--)
        data = get_word (f);
      //fprintf (stderr, "Atom index %d bignum\n", atom_index);
      atomtable[atom_index++] = strdup ("Bignum");
      break;
    case 4: //Double-precision number.
      data = get_word (f);
      data = get_word (f);
      //fprintf (stderr, "Atom index %d double\n", atom_index);
      atomtable[atom_index++] = strdup ("Double");
      break;
    case 5: //Complex number.
      data = get_word (f);
      data = get_word (f);
      //fprintf (stderr, "Atom index %d complex\n", atom_index);
      atomtable[atom_index++] = strdup ("Complex");
      break;
    case 6: //Duplex number.
      data = get_word (f);
      data = get_word (f);
      data = get_word (f);
      data = get_word (f);
      //fprintf (stderr, "Atom index %d duplex\n", atom_index);
      atomtable[atom_index++] = strdup ("Duplex");
      break;
    case 7: //Unused.
      fprintf (stderr, "Bad value.\n");
      exit (1);
      break;
    }

}

static void 
load (struct pdp10_memory *memory, word_t data)
{
  word_t *p = malloc (sizeof data);
  *p = data;
  add_memory (memory, address++, 1, p);
}

static word_t
relocate (word_t word, word_t offset, word_t mask)
{
  return (word & ~mask) | ((word + offset) & mask);
}

static void
patch (struct pdp10_memory *memory, word_t data, word_t mask, const char *x)
{
  word_t word = get_word_at (memory, address - 1);
  word = relocate (word, data, mask);
  set_word_at (memory, address - 1 , word);
  (void)x;
  //fprintf (stderr, "%06o/ %012llo (%s)\n", address - 1, word, x);
}

static void 
discard (struct pdp10_memory *memory, word_t data)
{
  (void)memory;
  (void)data;
  //fprintf (stderr, "Discard.\n");
}

static void
read_sexp (FILE *f, word_t data, struct pdp10_memory *memory,
           void (*end) (struct pdp10_memory *, word_t))
{
  int items = 0;

  for (;;)
    {
      switch ((data >> 33) & 7)
        {
        case 0:
          //fprintf (stderr, "Push atom %lld onto stack.\n", data & 0777777);
          items++;
          break;
        case 2:
          //fprintf (stderr, "Make a list ended by atom popped off list.\n");
          items--;
          // Fall through.
        case 1:
          //fprintf (stderr, "Pop %lld items off stack and make a list.\n", data & 0777777);
          items -= data & 0777777;
          items++;
          break;
        case 3:
          //fprintf (stderr, "Evaluate top item on stack.\n");
          break;
        case 4:
          //fprintf (stderr, "Make a hunk from %lld stack items.\n", data & 0777777);
          items -= data & 0777777;
          items++;
          break;
        case 5:
        case 6:
          fprintf (stderr, "Bad value.\n");
          exit (1);
          break;
        case 7:
          if (items != 1)
            //fprintf (stderr, "Stack items now: %d\n", items);
          switch (data >> 18)
            {
            case 0777777:
              end (memory, data);
              break;              
            case 0777776:
              //fprintf (stderr, "Load into atomtable index %d.\n", atom_index);
              atomtable[atom_index++] = "(list)";
              break;              
            default:
              fprintf (stderr, "Bad value.\n");
              break;              
            }
          if (end == load)
            {
              data = get_word (f);
              //fprintf (stderr, "Hash key: %012llo.\n", data);
            }
          return;
        }
      data = get_word (f);
    }
}

static int
read_block (FILE *f, struct pdp10_memory *memory)
{
  char symbol[7];
  char string[100];
  word_t relocations;
  word_t data, value;
  int i;

  relocations = get_word (f);
  for (i = 0; i < 9; i++)
    {
      data = get_word (f);
      switch ((relocations >> 32) & 017)
        {
        case 000:
          load (memory, data);
          //fprintf (stderr, "%06o/ %012llo\n", address - 1, data);
          break;
        case 001:
          load (memory, data);
          data += offset;
          patch (memory, data, 0777777LL, "RELOCATABLE");
          break;
        case 002:
          snprintf (string, sizeof string, "SPECIAL %s", atomtable[data & 0777777]);
          data &= 0777777000000LL;
          data |= 0 & 0777777;
          load (memory, data);
          break;
        case 003:
          snprintf (string, sizeof string, "SMASHABLE CALL %s", atomtable[data & 0777777]);
          load (memory, data);
          //fprintf (stderr, "%06o/ %012llo (%s)\n", address - 1, data, string);
          break;
        case 004:
          snprintf (string, sizeof string, "QUOTED ATOM %s", atomtable[data & 0777777]);
          data = (data & 0777777000000LL) | 0;
          load (memory, data);
          //fprintf (stderr, "%06o/ %012llo (%s)\n", address - 1, data, string);
          break;
        case 005:
          //fprintf (stderr, "Type 05, QUOTED LIST\n");
          read_sexp (f, data, memory, load);
          break;
        case 006:
          snprintf (string, sizeof string, "GLOBALSYM index %llu", data & 0777777);
          data = 0;
          patch (memory, data, 0777777LL, string);
          break;
        case 007:
          if (data == 0777777777777LL)
            {
              patch (memory, (word_t)offset << 18, 0777777000000LL, "LH");
            }
          else
            {
              squoze_to_ascii (data & 037777777777LL, symbol);
              snprintf (string, sizeof string, "DDT SYMBOL %s", symbol);
              data = 0;
              patch (memory, data, 0777777LL, string);
            }
          break;
        case 010:
          snprintf (string, sizeof string, "ARRAY REFERENCE %s", atomtable[data & 0777777]);
          data = 0;
          patch (memory, data, 0777777LL, string);
          break;
        case 011:
          /* Unused. */
          fprintf (stderr, "Bad value.\n");
          exit (1);
          break;
        case 012:
          read_atomtable (f, data);
          break;
        case 013:
          /*fprintf (stderr, "DEFUN %s %s",
            atomtable[data >> 18], atomtable[data & 0777777]);*/
          value = get_word (f);
          /*fprintf (stderr, ", address %06llo args %06llo\n",
            value & 0777777, value >> 18);*/
          value = relocate (value & 0777777, offset, 0777777);
          add_symbol (atomtable[data >> 18], value, SYMBOL_HALFKILLED);
          break;
        case 014:
          address = relocate (data & 0777777, offset, 0777777);
          break;
        case 015:
          squoze_to_ascii (data & 037777777777LL, symbol);
          if (data & 0400000000000LL)
            value = get_word (f);
          else
            value = address;
          fprintf (stderr, "PUTDDTSYM %s = %012llo", symbol, value);
          if (data & 0100000000000LL)
            value = relocate (value, offset, 0777777);
          if (data & 0200000000000LL)
            value = relocate (value, (word_t)offset << 18, 0777777000000LL);
          //fprintf (stderr, "PUTDDTSYM %s = %012llo\n", symbol, value);
          fprintf (stderr, " -> %012llo\n", value);
          add_symbol (symbol, value, SYMBOL_HALFKILLED);
          break;
        case 016:
          //fprintf (stderr, "Type 05, EVAL MUNGEABLE\n");
          read_sexp (f, data, memory, discard);
          break;
        case 017:
          if (!fasl_magic (data))
            fprintf (stderr, "Not a valid FASL file.\n");
          return 0;
        }
      relocations <<= 4;
    }

  return 1;
}

static void
read_fasl (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  char version[7];
  word_t word;

  (void)cpu_model;

  fprintf (output_file, "FASL format\n");

  word = get_word (f);
  if (!fasl_magic (word))
    {
      fprintf (stderr, "Not a valid FASL file.\n");
      exit (1);
    }

  word = get_word (f);
  sixbit_to_ascii (word, version);
  fprintf (output_file, "  Lisp version: %s\n", version);

  // The conventional starting address.
  offset = 0100;

  address = offset;
  while (read_block (f, memory))
    ;
}

struct file_format fasl_file_format = {
  "fasl",
  read_fasl,
  NULL
};
