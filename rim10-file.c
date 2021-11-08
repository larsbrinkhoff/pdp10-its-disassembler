/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

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

/* This file format will read in a paper tape with a hardware read-in
   loader.  To do this, it will read in the loader and run it by
   emulating a tiny subset of PDP-10 instructions.  The subset is enough to
   support the ITS RIM10 and DEC RIM10B loaders. */

#include <stdio.h>
#include <stdlib.h>

#include "dis.h"
#include "memory.h"

/* Instructions used by the ITS RIM10 and DEC RIM10B loaders. */
#define OP_ROT        0241
#define OP_JRST       0254
#define OP_XCT        0256
#define OP_AOBJN      0253
#define OP_ADD        0270
#define OP_CAME       0312
#define OP_SKIPL      0331
#define OP_SOJA       0364
#define OP_HRRI       0541
#define OP_DATAI      070004
#define OP_CONO       070020
#define OP_CONSO      070034

static word_t midas_rim10[] =
{
  /*00*/ 0777761000000LL, /* -17,,0 */
  /*01*/ 0710600000060LL, /* CONO PTR,60 */
  /*02*/ 0541440000004LL, /* HRRI 11,4 */
  /*03*/ 0710740000010LL, /* CONSO PTR,10 */
  /*04*/ 0254000000003LL, /* JRST 3 */
  /*05*/ 0241011777776LL, /* ROT 0,-2(11) */
  /*06*/ 0710471000010LL, /* DATAI PTR,@10(11) */
  /*07*/ 0256011000010LL, /* XCT 10(11) */
  /*10*/ 0256011000013LL, /* XCT 13(11) */
  /*11*/ 0364440000000LL, /* SOJA 11,0 */
  /*12*/ 0312000000017LL, /* CAME 0,17 */
  /*13*/ 0270017000000LL, /* ADD 0,(17) */
  /*14*/ 0331740000000LL, /* SKIPL 17,0 */
  /*15*/ 0254200000001LL, /* JRST 4,1 */
  /*16*/ 0253740000003LL, /* AOBJN 17,3 */
  /*17*/ 0254000000002LL  /* JRST 2 */
};
  
static int
effective_address (word_t insn, struct pdp10_memory *memory)
{
  for (;;)
    {
      int x = (insn >> 18) & 017;
      int y = insn;
      if (x != 0)
	y += get_word_at (memory, x);
      y &= 0777777;
      if ((insn & 020000000) == 0)
	return y;
      insn = get_word_at (memory, y);
    }
}

static int
execute_iot (word_t insn, int ea, struct pdp10_memory *memory, FILE *f)
{
  switch ((insn >> 21) & 070034)
    {
    case OP_DATAI:
      insn = get_word (f);
      //fprintf (stderr, "DATAI %012llo -> %06o\n", insn, ea);
      set_word_at (memory, ea, insn);
      break;
    case OP_CONO:
      //fprintf (stderr, "CONO\n");
      break;
    case OP_CONSO:
      //fprintf (stderr, "CONSO\n");
      return 1;
    }

  return 0;
}

static int
execute (word_t insn, int pc, struct pdp10_memory *memory, FILE *f)
{
  int ea, ac;
  word_t ma;

  //fprintf (stderr, "%012llo\n", insn);
  ea = effective_address (insn, memory);
  ac = (insn >> 23) & 017;

  switch (insn >> 27) {
  case OP_ROT:
    ma = get_word_at (memory, ac);
    //fprintf (stderr, "ROT %o, %012llo -> ", ac, ma);
    if (ea & 0400000)
      {
	ea = 01000000 - ea;
	ma = (ma >> ea) | (ma << (36 - ea));
      }
    else
      ma = (ma << ea) | (ma >> (36 - ea));
    ma &= WORDMASK;
    //fprintf (stderr, "%012llo\n", ma);
    set_word_at (memory, ac, ma);
    break;
  case OP_AOBJN:
    ma = get_word_at (memory, ac);
    //fprintf (stderr, "AOBJN %o, %012llo -> ", ac, ma);
    ma += 01000001LL;
    ma &= WORDMASK;
    //fprintf (stderr, "%012llo\n", ma);
    set_word_at (memory, ac, ma);
    if (ma & 0400000000000LL)
      pc = ea;
    break;
  case OP_JRST:
    //fprintf (stderr, "JRST\n");
    switch ((insn >> 23) & 017)
      {
      case 0: pc = ea; break;
      case 4: fprintf (stderr, "HALT\n");
      default: exit (1);
      }
    break;
  case OP_XCT:
    insn = get_word_at (memory, ea);
    //fprintf (stderr, "XCT ");
    pc = execute (insn, pc, memory, f);
    break;
  case OP_ADD:
    ma = get_word_at (memory, ea);
    //fprintf (stderr, "ADD %o, %012llo + %012llo -> ", ac, get_word_at (memory, ac), ma);
    ma += get_word_at (memory, ac);
    ma &= WORDMASK;
    //fprintf (stderr, "%012llo\n", ma);
    set_word_at (memory, ac, ma);
    break;
  case OP_CAME:
    ma = get_word_at (memory, ea);
    //fprintf (stderr, "CAME %o, %012llo\n", ac, ma);
    if (ma == get_word_at (memory, ac))
      pc++;
    break;
  case OP_SKIPL:
    ma = get_word_at (memory, ea);
    //fprintf (stderr, "SKIPL %o,%012llo\n", ac, ma);
    if (ac)
      set_word_at (memory, ac, ma);
    if (ma & 0400000000000LL)
      pc++;
    break;
  case OP_SOJA:
    ma = get_word_at (memory, ac);
    //fprintf (stderr, "SOJA %o, %012llo -> ", ac, ma);
    ma--;
    ma &= WORDMASK;
    //fprintf (stderr, "%012llo\n", ma);
    set_word_at (memory, ac, ma);
    pc = ea;
    break;
  case OP_HRRI:
    ma = get_word_at (memory, ac);
    //fprintf (stderr, "HRRI %o, %012llo ->", ac, ma);
    ma &= 0777777000000LL;
    ma |= ea;
    //fprintf (stderr, "%012llo\n", ma);
    set_word_at (memory, ac, ma);
    break;
  case 0700: case 0701: case 0702: case 0703: case 0704: case 0705: case 0706:
  case 0707: case 0710: case 0711: case 0712: case 0713: case 0714: case 0715:
  case 0716: case 0717: case 0720: case 0721: case 0722: case 0723: case 0724:
  case 0725: case 0726: case 0727: case 0730: case 0731: case 0732: case 0733:
  case 0734: case 0735: case 0736: case 0737: case 0740: case 0741: case 0742:
  case 0743: case 0744: case 0745: case 0746: case 0747: case 0750: case 0751:
  case 0752: case 0753: case 0754: case 0755: case 0756: case 0757: case 0760:
  case 0761: case 0762: case 0763: case 0764: case 0765: case 0766: case 0767:
  case 0770: case 0771: case 0772: case 0773: case 0774: case 0775: case 0776:
  case 0777:
    pc += execute_iot (insn, ea, memory, f);
    break;
  default:
    fprintf (stderr, "Unsupported RIM10 loader instruction: %012llo\n", insn);
    exit (1);
  }

  return pc;
}

static void
read_rim10 (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  word_t insn;
  word_t *loader;
  int address, length;
  word_t word;
  int i;

  fprintf (output_file, "RIM10 format\n");

  word = get_word (f);
  address = (word & 0777777) + 1;
  length = 01000000 - (word >> 18);
  if (length > 16)
    {
      fprintf (stderr, "RIM10 loader longer than 16 words.\n");
      exit (1);
    }

  loader = malloc (16 * sizeof (word_t));
  if (loader == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }

  for (i = address; i < address + length; i++)
    loader[i] = get_word (f);
  add_memory (memory, 0, 16, loader);

  address = address + length - 1;
  while (address < 16)
    {
      //fprintf (stderr, "\n%06o: ", address);
      insn = get_word_at (memory, address++);
      address = execute (insn, address, memory, f);
    }

  switch (insn & 0777777000000LL)
    {
    case JRST:
    case JUMPA:
      start_instruction = insn;
      break;
    default:
      start_instruction = JRST + address;
      break;
    }
  fprintf (output_file, "Start address: %o\n", address);
}

static void
write_rim10 (FILE *f, struct pdp10_memory *memory)
{
  int i;

  for (i = 0; i < sizeof midas_rim10 / sizeof midas_rim10[0]; i++)
    write_word (f, midas_rim10[i]);

  write_sblk_core (f, memory);
  write_word (f, start_instruction);
}

struct file_format rim10_file_format = {
  "rim10",
  read_rim10,
  write_rim10
};
