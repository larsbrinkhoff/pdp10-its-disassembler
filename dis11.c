/* Print DEC PDP-11 instructions.
   Copyright (C) 2001-2022 Free Software Foundation, Inc.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include "opcode/pdp11.h"
#include "dis.h"
#include "memory.h"
#include "symbols.h"

#define AFTER_INSTRUCTION	"\t"
#define OPERAND_SEPARATOR	", "

#define JUMP	0x1000	/* Flag that this operand is used in a jump.  */

/* Sign-extend a 16-bit number in an int.  */
#define sign_extend(x) ((((x) & 0xffff) ^ 0x8000) - 0x8000)

FILE *output_file;

static int
read_word (int memaddr, int *word, struct pdp10_memory *memory)
{
  *word = get_word_at (memory, memaddr) & 0377;
  *word |= (get_word_at (memory, memaddr + 1) & 0377) << 8;
  return 0;
}

static void
print_signed_octal (int n)
{
  if (n < 0)
    fprintf (output_file, "-%o", -n);
  else
    fprintf (output_file, "%o", n);
}

static void
print_address (int address)
{
  fprintf (output_file, "%o", address);
}

static void
print_reg (int reg)
{
  /* Mask off the addressing mode, if any.  */
  reg &= 7;

  switch (reg)
    {
    case 0: case 1: case 2: case 3: case 4: case 5:
		fprintf (output_file, "r%d", reg); break;
    case 6:	fprintf (output_file, "sp"); break;
    case 7:	fprintf (output_file, "pc"); break;
    default: ;	/* error */
    }
}

static void
print_freg (int freg)
{
  fprintf (output_file, "fr%d", freg);
}

static int
print_operand (int *memaddr, int code, struct pdp10_memory *memory)
{
  int mode = (code >> 3) & 7;
  int reg = code & 7;
  int disp;

  switch (mode)
    {
    case 0:
      print_reg (reg);
      break;
    case 1:
      fprintf (output_file, "(");
      print_reg (reg);
      fprintf (output_file, ")");
      break;
    case 2:
      if (reg == 7)
	{
	  int data;

	  if (read_word (*memaddr, &data, memory) < 0)
	    return -1;
	  fprintf (output_file, "#");
	  print_signed_octal (sign_extend (data));
	  *memaddr += 2;
	}
      else
	{
	  fprintf (output_file, "(");
	  print_reg (reg);
	  fprintf (output_file, ")+");
	}
	break;
    case 3:
      if (reg == 7)
	{
	  int address;

	  if (read_word (*memaddr, &address, memory) < 0)
	    return -1;
	  fprintf (output_file, "@#%o", address);
	  *memaddr += 2;
	}
      else
	{
	  fprintf (output_file, "@(");
	  print_reg (reg);
	  fprintf (output_file, ")+");
	}
	break;
    case 4:
      fprintf (output_file, "-(");
      print_reg (reg);
      fprintf (output_file, ")");
      break;
    case 5:
      fprintf (output_file, "@-(");
      print_reg (reg);
      fprintf (output_file, ")");
      break;
    case 6:
    case 7:
      if (read_word (*memaddr, &disp, memory) < 0)
	return -1;
      *memaddr += 2;
      if (reg == 7)
	{
	  int address = *memaddr + sign_extend (disp);

	  if (mode == 7)
	    fprintf (output_file, "@");
	  if (!(code & JUMP))
	    fprintf (output_file, "#");
	  print_address (address);
	}
      else
	{
	  if (mode == 7)
	    fprintf (output_file, "@");
	  print_signed_octal (sign_extend (disp));
	  fprintf (output_file, "(");
	  print_reg (reg);
	  fprintf (output_file, ")");
	}
      break;
    }

  return 0;
}

static int
print_foperand (int *memaddr, int code, struct pdp10_memory *memory)
{
  int mode = (code >> 3) & 7;
  int reg = code & 7;

  if (mode == 0)
    print_freg (reg);
  else
    return print_operand (memaddr, code, memory);

  return 0;
}

/* Print the PDP-11 instruction at address MEMADDR in debugged memory,
   on output_stream.  Returns length of the instruction, in bytes.  */

int
print_insn_pdp11 (int memaddr, struct pdp10_memory *memory)
{
  int start_memaddr = memaddr;
  int opcode;
  int src, dst;
  int i;

  if (read_word (memaddr, &opcode, memory) != 0)
    return -1;
  memaddr += 2;

  src = (opcode >> 6) & 0x3f;
  dst = opcode & 0x3f;

  for (i = 0; i < pdp11_num_opcodes; i++)
    {
#define OP pdp11_opcodes[i]
      if ((opcode & OP.mask) == OP.opcode)
	switch (OP.type)
	  {
	  case PDP11_OPCODE_NO_OPS:
	    fprintf (output_file, "%s", OP.name);
	    goto done;
	  case PDP11_OPCODE_REG:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    print_reg (dst);
	    goto done;
	  case PDP11_OPCODE_OP:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    if (strcmp (OP.name, "jmp") == 0)
	      dst |= JUMP;
	    if (print_operand (&memaddr, dst, memory) < 0)
	      return -1;
	    goto done;
	  case PDP11_OPCODE_FOP:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    if (strcmp (OP.name, "jmp") == 0)
	      dst |= JUMP;
	    if (print_foperand (&memaddr, dst, memory) < 0)
	      return -1;
	    goto done;
	  case PDP11_OPCODE_REG_OP:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    print_reg (src);
	    fprintf (output_file, OPERAND_SEPARATOR);
	    if (strcmp (OP.name, "jsr") == 0)
	      dst |= JUMP;
	    if (print_operand (&memaddr, dst, memory) < 0)
	      return -1;
	    goto done;
	  case PDP11_OPCODE_REG_OP_REV:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    if (print_operand (&memaddr, dst, memory) < 0)
	      return -1;
	    fprintf (output_file, OPERAND_SEPARATOR);
	    print_reg (src);
	    goto done;
	  case PDP11_OPCODE_AC_FOP:
	    {
	      int ac = (opcode & 0xe0) >> 6;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      print_freg (ac);
	      fprintf (output_file, OPERAND_SEPARATOR);
	      if (print_foperand (&memaddr, dst, memory) < 0)
		return -1;
	      goto done;
	    }
	  case PDP11_OPCODE_FOP_AC:
	    {
	      int ac = (opcode & 0xe0) >> 6;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      if (print_foperand (&memaddr, dst, memory) < 0)
		return -1;
	      fprintf (output_file, OPERAND_SEPARATOR);
	      print_freg (ac);
	      goto done;
	    }
	  case PDP11_OPCODE_AC_OP:
	    {
	      int ac = (opcode & 0xe0) >> 6;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      print_freg (ac);
	      fprintf (output_file, OPERAND_SEPARATOR);
	      if (print_operand (&memaddr, dst, memory) < 0)
		return -1;
	      goto done;
	    }
	  case PDP11_OPCODE_OP_AC:
	    {
	      int ac = (opcode & 0xe0) >> 6;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      if (print_operand (&memaddr, dst, memory) < 0)
		return -1;
	      fprintf (output_file, OPERAND_SEPARATOR);
	      print_freg (ac);
	      goto done;
	    }
	  case PDP11_OPCODE_OP_OP:
	    fprintf (output_file, "%s", OP.name);
	    fprintf (output_file, AFTER_INSTRUCTION);
	    if (print_operand (&memaddr, src, memory) < 0)
	      return -1;
	    fprintf (output_file, OPERAND_SEPARATOR);
	    if (print_operand (&memaddr, dst, memory) < 0)
	      return -1;
	    goto done;
	  case PDP11_OPCODE_DISPL:
	    {
	      int displ = (opcode & 0xff) << 8;
	      int address = memaddr + (sign_extend (displ) >> 7);
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      print_address (address);
	      goto done;
	    }
	  case PDP11_OPCODE_REG_DISPL:
	    {
	      int displ = (opcode & 0x3f) << 10;
	      int address = memaddr - (displ >> 9);

	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      print_reg (src);
	      fprintf (output_file, OPERAND_SEPARATOR);
	      print_address (address);
	      goto done;
	    }
	  case PDP11_OPCODE_IMM8:
	    {
	      int code = opcode & 0xff;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      fprintf (output_file, "%o", code);
	      goto done;
	    }
	  case PDP11_OPCODE_IMM6:
	    {
	      int code = opcode & 0x3f;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      fprintf (output_file, "%o", code);
	      goto done;
	    }
	  case PDP11_OPCODE_IMM3:
	    {
	      int code = opcode & 7;
	      fprintf (output_file, "%s", OP.name);
	      fprintf (output_file, AFTER_INSTRUCTION);
	      fprintf (output_file, "%o", code);
	      goto done;
	    }
	  case PDP11_OPCODE_ILLEGAL:
	    {
	      fprintf (output_file, ".word");
	      fprintf (output_file, AFTER_INSTRUCTION);
	      fprintf (output_file, "%o", opcode);
	      goto done;
	    }
	  default:
	    /* TODO: is this a proper way of signalling an error? */
	    fprintf (output_file, "<internal error: unrecognized instruction type>");
	    return -1;
	  }
#undef OP
    }
 done:

  return memaddr - start_memaddr;
}

void
disassemble_word (struct pdp10_memory *memory, word_t word,
		  int address, int cpu_model)
{
  (void)memory;
  (void)word;
  (void)address;
  (void)cpu_model;
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-F<file format>] [-W<word format>] <file>\n\n", argv[0]);
  usage_file_format ();
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  struct pdp10_memory memory;
  int i, n, opt;
  FILE *file;
  int address;

  output_file = stdout;
  input_file_format = &palx_file_format;

  while ((opt = getopt (argc, argv, "F:W:")) != -1)
    {
      switch (opt)
	{
	case 'F':
	  if (parse_input_file_format (optarg))
	    usage (argv);
	  break;
	case 'W':
	  if (parse_input_word_format (optarg))
	    usage (argv);
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

  input_file_format->read (file, &memory, 0);
  printf ("\nDisassembly:\n\n");

  for (i = 0; i < memory.areas; i++)
    {
      address = memory.area[i].start;
      while (address < memory.area[i].end)
	{
	  const struct symbol *sym =
	    get_symbol_by_value (address, HINT_ADDRESS);
	  if (sym != NULL)
	    fprintf (output_file, "%s:\n", sym->name);

	  fprintf (output_file, "%06o:", address);
	  fprintf (output_file, AFTER_INSTRUCTION);
	  n = print_insn_pdp11 (address, &memory);
	  fprintf (output_file, "\n");
	  address += n;
	}
    }
}
