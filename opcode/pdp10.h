/* PDP-10 opcde list.
   Copyright (C) 2000 Free Software Foundation, Inc.

This file is part of GDB and GAS.

GDB and GAS are free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB and GAS are distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB or GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/*
 * PDP-10 instruction types.
 *
 * Most PDP-10 instructions have this format:
 *
 * +---------+----+-+----+------------------+
 * |  opcode |  A |I|  X |         Y        |
 * +---------+----+-+----+------------------+
 *    9 bits    4  1   4        18 bits
 *
 * A usually specifies the accumulator.
 * I indicates indirect addressing.
 * X specifies the index register.
 * Y specifies the address.
 *
 * I, X, and Y is used together to calculate E, the effective address.
 *
 * PDP10_A_OPCODE means that A is used together with the opcode field
 * to specify the operation.
 *
 * PDP10_A_UNUSED means that A is unused and should be set to zero.
 * When disassembled, the field will be ignored unless it's non-zero.
 *
 * PDP10_E_UNUSED means that X, I, and Y are unused and should be set to zero.
 * When disassembled, Y will be ignored unless it's non-zero.
 *
 * PDP10_IO means that the instruction format looks like this:
 *
 * +---+-------+---+-+----+------------------+
 * |op1| device|op2|I|  X |         Y        |
 * +---+-------+---+-+----+------------------+
 *   3   7 bits   3 1   4       18 bits
 *
 * op1 and op2 is used together to specify the operation.
 *
 * I, X, and Y as above.
 *
 * PDP10_EXTEND means that this the first word of a two-word
 * instruction.  The second word is located at the effective address,
 * E, of the first instruction word.
 */

#define PDP10_BASIC		0000
#define PDP10_A_OPCODE		0001
#define PDP10_A_UNUSED		0002
#define PDP10_E_UNUSED		0010
#define PDP10_A_E_UNUSED	(PDP10_A_UNUSED | PDP10_E_UNUSED)
#define PDP10_IO		0020
#define PDP10_EXTEND		0040

/*
 * PDP-10 CPU models.
 */

#define PDP10_NONE     0x000000
#define PDP6_166       0x000001	/* DEC PDP-6 Type 166 Arithmetic Processor */
#define PDP10_KA10     0x000002	/* DEC PDP-10 KA10 */
#define PDP10_KA10_ITS 0x000004	/* DEC PDP-10 KA10, modifications for ITS */
#define PDP10_KI10     0x000008	/* DEC PDP-10 KI10 */
#define PDP10_KL10     0x000010	/* DEC PDP-10 KL10 */
#define PDP10_KL10_ITS 0x000020	/* DEC PDP-10 KL10, ITS microcode */
#define PDP10_KL10_271 0x000040	/* DEC PDP-10 KL10, microcode version >= 271 */
#define PDP10_KS10     0x000080	/* DEC PDP-10 KS10 */
#define PDP10_KS10_ITS 0x000100	/* DEC PDP-10 KS10, ITS microcode */
#define PDP10_MAXC     0x000200	/* Xerox PARC MAXC */
#define PDP10_F1       0x000400	/* Foonly F-1 */
#define PDP10_F2       0x000800	/* Foonly F-2 */
#define PDP10_F3       0x001000	/* Foonly F-3 */
#define PDP10_F4       0x002000	/* Foonly F-4 */
#define PDP10_F5       0x004000	/* Foonly F-5 */
#define PDP10_SC20     0x008000	/* System Concepts SC-20 */
#define PDP10_SC25     0x010000	/* System Concepts SC-25 */
#define PDP10_SC30M    0x020000	/* System Concepts SC-30M */
#define PDP10_SC40     0x040000	/* System Concepts SC-40 */
#define PDP10_XKL1     0x080000	/* XKL TOAD-1 XKL-1 */
#define PDP10_ALL      0xffffff

/* Useful combinations of the above. */
#define PDP6_166_to_PDP10_KI10	(PDP6_166 | PDP10_KA10_to_KI10)
#define PDP10_KA10any		(PDP10_KA10 | PDP10_KA10_ITS)
#define PDP10_KA10_to_KI10	(PDP10_KA10 | PDP10_KI10)
#define PDP10_KA10_to_KL10	(PDP10_KA10_to_KI10 | PDP10_KL10any)
#define PDP10_KI10_to_KL10	(PDP10_KI10 | PDP10_KL10any)
#define PDP10_KA10up		(PDP10_KA10any | PDP10_KI10up)
#define PDP10_KI10up		(PDP10_KI10 | PDP10_KL10up)
#define PDP10_KL10any		(PDP10_KL10 | PDP10_KL10_ITS | \
				 PDP10_KL10_271up)
#define PDP10_KL10up		(PDP10_KL10 | PDP10_KL10_ITS | \
				 PDP10_KL10_271up | PDP10_KS10)
#define PDP10_KL10_271up	(PDP10_KL10_271 | PDP10_SC20up | PDP10_XKL1)
#define PDP10_KS10any		(PDP10_KS10 | PDP10_KS10_ITS)
#define PDP10_not_KS10		(PDP10_ALL & ~(PDP10_KS10any))
#define PDP10_SC20_to_SC25	(PDP10_SC20 | PDP10_SC25)
#define PDP10_SC20up		(PDP10_SC20_to_SC25 | PDP10_SC30M | PDP10_SC40)
#define PDP10_ITS		(PDP10_KA10_ITS | PDP10_KL10_ITS | \
				 PDP10_KS10_ITS)

struct pdp10_instruction
{
  const char *name;
  int opcode;
  int type;
  int model;
};

struct pdp10_device
{
  const char *name;
  int number;
  int model;
};

extern const struct pdp10_instruction pdp10_instruction[];
extern const struct pdp10_device pdp10_device[];
extern const struct pdp10_instruction pdp10_alias[];
extern const int pdp10_num_instructions, pdp10_num_devices, pdp10_num_aliases;

/* end of pdp10.h */
