/* Opcode table for PDP-10.
   Copyright 2000
   Free Software Foundation, Inc.

This file is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "opcode/pdp10.h"

/*
 * Instruction set decription for the PDP-10 architecture.
 *
 * Much of this is derived from DECsystem-10 / DECSYSTEM-20 Processor
 * Reference Manual (AA-H391A-TK, AD-H391A-T1) (updated 1982), which
 * describese these PDP-10 processors: KA10, KI10, KL10, and KS10.
 *
 * PDP-6 Arithmetic Processor 166 Instruction Manual and Appendix E of
 * Hardware Reference Manual (DEC-10-XSRNA-A-D) has been used for
 * information about the PDP-6 Type 166 Arithmetic Processor.
 *
 * KS10 ITS system instructions according to ITS documention file
 * KSHACK;KSDEFS 193 (updated 1987).
 *
 * KA10, KL10, and KS10 ITS system instructions and device codes
 * according to ITS source code version 1644 (updated 1990).
 *
 * XKL-1 system instructions according to TOAD-1 System Architecture
 * Reference Manual, Revision 02 (updated 1997).
 *
 * These PDP-10 processors are known to exist, but information about
 * them has not been encoded in this table: Xerox MAXC, Foonly F-x,
 * System Concept SC-xx.
 */

/*
 * When disassembling, please scan the table linearly from start to end.
 * FIXME: explain why.
 */

/*
From: "Stewart Nelson" <sn@scgroup.com>
Date: Mon, 24 Jul 2000 16:31:50 +0200

In User Mode, our machines are essentially identical to the
extended KL-10.  There are no new instructions.  There are
some differences in intermediate results when instructions
are interrupted, and our floating point answers are often
more accurate when unnormalized operands are used.
It is intended that all KL-10 user applications will run
on our machines.  The reverse is also true, if the needed
resources are available.

In Executive Mode, there are many differences, which allow
for larger virtual and physical address spaces, and which
provide access to new devices, e.g. SCSI.  However, these
enhancements are implemented by making additional operand
formats available to existing I/O operations for paging,
device control, etc.  Again, the opcode set is identical
to that of the KL-10B.  If equipped with appropriate ancient
DEC I/O devices, our machines can run unmodified binaries
for both TOPS-10 and TOPS-20.
*/

/*
From: "Stewart Nelson" <sn@scgroup.com>
Date: Tue, 25 Jul 2000 13:38:33 +0200
Subject: Re: Information about the SC-40 CPU

> By the way, what are the SC-xx CPUs called?

They don't really have a name.

In the SC-25 and SC-30, the CPU is 11 boards of TTL logic;
each board has a part number.

The SC-40 has a single CPU board with a custom chip, which is
simply marked "SC-40 CPU".
*/

/*
From: alderson@netcom.com (Richard M. Alderson III)
Subject: Re: PDP-10/20 Clones
Date: Thu, 3 Nov 1994 19:06:59 GMT

I believe that the original configuration, the SC-30M, is no longer available.
The same processor, in a 19-inch rack, constitutes the SC-25; a half-speed
version is the SC-20; and the latest version, with faster floating-point, is
the SC-40.

The design goal of the SC systems was to provide a replacement CPU in a PDP-10
shop, while allowing the customer to keep original peripherals.  Thus, they
provided Massbus-, CI-, and NI-compatible interfaces (bug-for-bug compatible),
along with a version of the SA10 interface (their original product) to allow
the use of IBM bus-and-tag peripherals.  (I have been told, though I have never
seen one, that they eventually provided a SCSI interface as well.)
*/

/*
From: vsocci@best.com (Vance Socci)
Date: Thu, 15 Jun 95 04:07:11 GMT
Subject: Re: Emulator

The F-4 was an F-3 converted to be largely KL-10 compatible (with KI
style pagin no extended addressing).  The F-4 was originally created
from the F-3 by Foonly, for Tymshare, Inc. and as far as I know
Tymshare was the only place that bought any (but there may be other
customers that I didn't know about).  (I helped write and debug the
microcode for the KI style paging, re-wrote the tape controller
microcode to allow interrupts during tape transfers, and implemented
the KL only user level instructions like ADJBP, DMOVE, DADD, etc.)

The F-3 was originally designed to be the front end for the
Super-Foonly, the F-1.  I think only one Super Foonly ever existed,
and was developed for and owned by III down in Los Angeles somewhere.

The device controllers for the F-3 and F-4 were done in microcode, as
well as the Tymnet base interface.  They were unique to the F-4.

What ever happened to Dave Poole?  Time was when the PDP-10 designers
at DEC would go out to visit Poole and (Phil) Petit to brainstorm
before attempting the next PDP-10 design.  The KL-10 cache was a
variant on some ideas espoused by Poole and Petit.

And yes, PXCT worked in all its flavors on the F-4.  PXCT is not all
that hard to once you understand what its trying to accomplish and the
nature of its target i.  For instructions that have only one operand,
the only two bits that operate in P are 1) the bit that says to
calculate the effective address in the previous context, and 2) the
bit that says to fetch or store the operand itself in the previous
context.

The other two bits are for specifying previous context for the second
operand (B in byte instructions) and for the effective address of byte
instructions.  These also affect the context for EXTEND type instructions.
*/

const struct pdp10_instruction pdp10_instruction[] =
{
  /* name,	opcode,	type,		models */

#if 1 /* ITS MUUOs */
  { ".iot",	0040,	PDP10_BASIC,	PDP10_ITS },
  { ".open",	0041,	PDP10_BASIC,	PDP10_ITS },
  { ".oper",	0042,	PDP10_BASIC,	PDP10_ITS },
  { ".call",	004300,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".dismis",	004304,	PDP10_A_OPCODE,	PDP10_ITS },
#if 1
  { ".lose",	004310,	PDP10_A_OPCODE,	PDP10_ITS },
#else
  { ".trans",	004310,	PDP10_A_OPCODE,	PDP10_ITS },
#endif
  { ".tranad",	004314,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".value",	004320,	PDP10_A_OPCODE|PDP10_E_UNUSED, PDP10_ITS },
  { ".utran",	004324,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".core",	004330,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".trand",	004334,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".dstart",	004340,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".fdele",	004344,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".dstrtl",	004350,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".suset",	004354,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".ltpen",	004360,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".vscan",	004364,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".potset",	004370,	PDP10_A_OPCODE,	PDP10_ITS },
  { ".uset",	0044,	PDP10_BASIC,	PDP10_ITS },
  { ".break",	0045,	PDP10_BASIC,	PDP10_ITS },
  { ".status",	0046,	PDP10_BASIC,	PDP10_ITS },
  { ".access",	0047,	PDP10_BASIC,	PDP10_ITS },
#endif

  /* TOPS-xx instruction */
  { "ujen",	0100,	PDP10_BASIC,	PDP10_KI10up },

/*{ "",		0101,	PDP10_BASIC,	PDP10_NONE },*/
  { "gfad",	0102,	PDP10_BASIC,	PDP10_KL10_271 },
  { "gfsb",	0103,	PDP10_BASIC,	PDP10_KL10_271 },

  /* TOPS-20 instruction */
  { "jsys",	0104,	PDP10_BASIC,	PDP10_KI10up },

  { "adjsp",	0105,	PDP10_BASIC,	PDP10_KL10up },
  { "gfmp",	0106,	PDP10_BASIC,	PDP10_KL10_271 },
  { "gfdv",	0107,	PDP10_BASIC,	PDP10_KL10_271 },
  { "dfad",	0110,	PDP10_BASIC,	PDP10_KI10up },
  { "dfsb",	0111,	PDP10_BASIC,	PDP10_KI10up },
  { "dfmp",	0112,	PDP10_BASIC,	PDP10_KI10up },
  { "dfdv",	0113,	PDP10_BASIC,	PDP10_KI10up },
  { "dadd",	0114,	PDP10_BASIC,	PDP10_KL10up },
  { "dsub",	0115,	PDP10_BASIC,	PDP10_KL10up },
  { "dmul",	0116,	PDP10_BASIC,	PDP10_KL10up },
  { "ddiv",	0117,	PDP10_BASIC,	PDP10_KL10up },
  { "dmove",	0120,	PDP10_BASIC,	PDP10_KI10up },
  { "dmovn",	0121,	PDP10_BASIC,	PDP10_KI10up },
  { "fix",	0122,	PDP10_BASIC,	PDP10_KI10up },
  { "extend",	0123,	PDP10_EXTEND,	PDP10_KL10up },
  { "dmovem",	0124,	PDP10_BASIC,	PDP10_KI10up },
  { "dmovnm",	0125,	PDP10_BASIC,	PDP10_KI10up },
  { "fixr",	0126,	PDP10_BASIC,	PDP10_KI10up },
  { "fltr",	0127,	PDP10_BASIC,	PDP10_KI10up },
  { "ufa",	0130,	PDP10_BASIC,	PDP10_KA10_to_KI10 },
  { "dfn",	0131,	PDP10_BASIC,	PDP10_KA10_to_KI10 },
  { "fsc",	0132,	PDP10_BASIC,	PDP10_ALL },
  { "ibp",	013300,	PDP10_A_OPCODE,	PDP10_ALL },
  { "adjbp",	0133,	PDP10_BASIC,	PDP10_KL10up },
  { "ildb",	0134,	PDP10_BASIC,	PDP10_ALL },
  { "ldb",	0135,	PDP10_BASIC,	PDP10_ALL },
  { "idpb",	0136,	PDP10_BASIC,	PDP10_ALL },
  { "dpb",	0137,	PDP10_BASIC,	PDP10_ALL },
  { "fad",	0140,	PDP10_BASIC,	PDP10_ALL },
  { "fadl",	0141,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10 },
  { "fadm",	0142,	PDP10_BASIC,	PDP10_ALL },
  { "fadb",	0143,	PDP10_BASIC,	PDP10_ALL },
  { "fadr",	0144,	PDP10_BASIC,	PDP10_ALL },
  { "fadri",	0145,	PDP10_BASIC,	PDP10_KA10up },
  { "fadrl",	0145,	PDP10_BASIC,	PDP6_166 },
  { "fadrm",	0146,	PDP10_BASIC,	PDP10_ALL },
  { "fadrb",	0147,	PDP10_BASIC,	PDP10_ALL },
  { "fsb",	0150,	PDP10_BASIC,	PDP10_ALL },
  { "fsbl",	0151,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10 },
  { "fsbm",	0152,	PDP10_BASIC,	PDP10_ALL },
  { "fsbb",	0153,	PDP10_BASIC,	PDP10_ALL },
  { "fsbr",	0154,	PDP10_BASIC,	PDP10_ALL },
  { "fsbri",	0155,	PDP10_BASIC,	PDP10_KA10up },
  { "fsbrl",	0155,	PDP10_BASIC,	PDP6_166 },
  { "fsbrm",	0156,	PDP10_BASIC,	PDP10_ALL },
  { "fsbrb",	0157,	PDP10_BASIC,	PDP10_ALL },
  { "fmp",	0160,	PDP10_BASIC,	PDP10_ALL },
  { "fmpl",	0161,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10 },
  { "fmpm",	0162,	PDP10_BASIC,	PDP10_ALL },
  { "fmpb",	0163,	PDP10_BASIC,	PDP10_ALL },
  { "fmpr",	0164,	PDP10_BASIC,	PDP10_ALL },
  { "fmpri",	0165,	PDP10_BASIC,	PDP10_KA10up },
  { "fmprl",	0165,	PDP10_BASIC,	PDP6_166 },
  { "fmprm",	0166,	PDP10_BASIC,	PDP10_ALL },
  { "fmprb",	0167,	PDP10_BASIC,	PDP10_ALL },
  { "fdv",	0170,	PDP10_BASIC,	PDP10_ALL },
  { "fdvl",	0171,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10 },
  { "fdvm",	0172,	PDP10_BASIC,	PDP10_ALL },
  { "fdvb",	0173,	PDP10_BASIC,	PDP10_ALL },
  { "fdvr",	0174,	PDP10_BASIC,	PDP10_ALL },
  { "fdvri",	0175,	PDP10_BASIC,	PDP10_KA10up },
  { "fdvrl",	0175,	PDP10_BASIC,	PDP6_166 },
  { "fdvrm",	0176,	PDP10_BASIC,	PDP10_ALL },
  { "fdvrb",	0177,	PDP10_BASIC,	PDP10_ALL },
  { "move",	0200,	PDP10_BASIC,	PDP10_ALL },
  { "movei",	0201,	PDP10_BASIC,	PDP10_ALL },
  { "movem",	0202,	PDP10_BASIC,	PDP10_ALL },
  { "moves",	0203,	PDP10_BASIC,	PDP10_ALL },
  { "movs",	0204,	PDP10_BASIC,	PDP10_ALL },
  { "movsi",	0205,	PDP10_BASIC,	PDP10_ALL },
  { "movsm",	0206,	PDP10_BASIC,	PDP10_ALL },
  { "movss",	0207,	PDP10_BASIC,	PDP10_ALL },
  { "movn",	0210,	PDP10_BASIC,	PDP10_ALL },
  { "movni",	0211,	PDP10_BASIC,	PDP10_ALL },
  { "movnm",	0212,	PDP10_BASIC,	PDP10_ALL },
  { "movns",	0213,	PDP10_BASIC,	PDP10_ALL },
  { "movm",	0214,	PDP10_BASIC,	PDP10_ALL },
  { "movmi",	0215,	PDP10_BASIC,	PDP10_ALL },
  { "movmm",	0216,	PDP10_BASIC,	PDP10_ALL },
  { "movms",	0217,	PDP10_BASIC,	PDP10_ALL },
  { "imul",	0220,	PDP10_BASIC,	PDP10_ALL },
  { "imuli",	0221,	PDP10_BASIC,	PDP10_ALL },
  { "imulm",	0222,	PDP10_BASIC,	PDP10_ALL },
  { "imulb",	0223,	PDP10_BASIC,	PDP10_ALL },
  { "mul",	0224,	PDP10_BASIC,	PDP10_ALL },
  { "muli",	0225,	PDP10_BASIC,	PDP10_ALL },
  { "mulm",	0226,	PDP10_BASIC,	PDP10_ALL },
  { "mulb",	0227,	PDP10_BASIC,	PDP10_ALL },
  { "idiv",	0230,	PDP10_BASIC,	PDP10_ALL },
  { "idivi",	0231,	PDP10_BASIC,	PDP10_ALL },
  { "idivm",	0232,	PDP10_BASIC,	PDP10_ALL },
  { "idivb",	0233,	PDP10_BASIC,	PDP10_ALL },
  { "div",	0234,	PDP10_BASIC,	PDP10_ALL },
  { "divi",	0235,	PDP10_BASIC,	PDP10_ALL },
  { "divm",	0236,	PDP10_BASIC,	PDP10_ALL },
  { "divb",	0237,	PDP10_BASIC,	PDP10_ALL },
  { "ash",	0240,	PDP10_BASIC,	PDP10_ALL },
  { "rot",	0241,	PDP10_BASIC,	PDP10_ALL },
  { "lsh",	0242,	PDP10_BASIC,	PDP10_ALL },
  { "jffo",	0243,	PDP10_BASIC,	PDP10_KA10up },
  { "ashc",	0244,	PDP10_BASIC,	PDP10_ALL },
  { "rotc",	0245,	PDP10_BASIC,	PDP10_ALL },
  { "lshc",	0246,	PDP10_BASIC,	PDP10_ALL },
/*{ "",		0247,	PDP10_BASIC,	PDP10_NONE },*/
  { "exch",	0250,	PDP10_BASIC,	PDP10_ALL },
  { "blt",	0251,	PDP10_BASIC,	PDP10_ALL },
  { "aobjp",	0252,	PDP10_BASIC,	PDP10_ALL },
  { "aobjn",	0253,	PDP10_BASIC,	PDP10_ALL },

  /*
   * JRST instruction family.
   *
   * Mnemonics for the most common instructions are recognized.  The
   * rest are caught by the last entry.
   */
  { "portal",	025404,	PDP10_A_OPCODE,	PDP10_ALL },
  { "jrstf",	025410,	PDP10_A_OPCODE,	PDP10_ALL },
  { "halt",	025420,	PDP10_A_OPCODE,	PDP10_ALL },
  { "xjrstf",	025424,	PDP10_A_OPCODE,	PDP10_KL10up },
  { "xjen",	025430,	PDP10_A_OPCODE,	PDP10_KL10up },
  { "xpcw",	025434,	PDP10_A_OPCODE,	PDP10_KL10up },
/*{ "jrstil",	025440,	PDP10_A_OPCODE,	PDP10_ALL },*/
  { "jen",	025450,	PDP10_A_OPCODE,	PDP10_ALL },
  { "sfm",	025460,	PDP10_A_OPCODE,	PDP10_KL10up },
  { "jrst",	0254,	PDP10_A_UNUSED,	PDP10_ALL },

  /*
   * JFCL instruction family.
   *
   * Mnemonics for common flags (floating overflow, carry 0, carry 1,
   * both carries, overflow) are recognized.  Exotic combinations are
   * caught by the last entry.
   */
  { "nop",	025500,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_ALL },
/*{ "jpcch",	025504,	PDP10_A_OPCODE,	PDP6_166 },*/
  { "jfov",	025504,	PDP10_A_OPCODE,	PDP10_KA10up },
  { "jcry1",	025510,	PDP10_A_OPCODE,	PDP10_ALL },
  { "jcry0",	025520,	PDP10_A_OPCODE,	PDP10_ALL },
  { "jcry",	025530,	PDP10_A_OPCODE,	PDP10_ALL },
  { "jov",	025540,	PDP10_A_OPCODE,	PDP10_ALL },
  { "jfcl",	0255,	PDP10_BASIC,	PDP10_ALL },

  { "xct",	0256,	PDP10_A_UNUSED,	PDP10_ALL },

  /* TOPS-10 instruction; nop otherwise */
  { "map",	0257,	PDP10_BASIC,	PDP10_KA10_to_KI10 },

  { "pushj",	0260,	PDP10_BASIC,	PDP10_ALL },
  { "push",	0261,	PDP10_BASIC,	PDP10_ALL },
  { "pop",	0262,	PDP10_BASIC,	PDP10_ALL },
  { "popj",	0263,	PDP10_E_UNUSED,	PDP10_ALL },
  { "jsr",	0264,	PDP10_A_UNUSED,	PDP10_ALL },
  { "jsp",	0265,	PDP10_BASIC,	PDP10_ALL },
  { "jsa",	0266,	PDP10_BASIC,	PDP10_ALL },
  { "jra",	0267,	PDP10_BASIC,	PDP10_ALL },
  { "add",	0270,	PDP10_BASIC,	PDP10_ALL },
  { "addi",	0271,	PDP10_BASIC,	PDP10_ALL },
  { "addm",	0272,	PDP10_BASIC,	PDP10_ALL },
  { "addb",	0273,	PDP10_BASIC,	PDP10_ALL },
  { "sub",	0274,	PDP10_BASIC,	PDP10_ALL },
  { "subi",	0275,	PDP10_BASIC,	PDP10_ALL },
  { "subm",	0276,	PDP10_BASIC,	PDP10_ALL },
  { "subb",	0277,	PDP10_BASIC,	PDP10_ALL },
  { "cai",	0300,	PDP10_BASIC,	PDP10_ALL },
  { "cail",	0301,	PDP10_BASIC,	PDP10_ALL },
  { "caie",	0302,	PDP10_BASIC,	PDP10_ALL },
  { "caile",	0303,	PDP10_BASIC,	PDP10_ALL },
  { "caia",	0304,	PDP10_A_E_UNUSED,	PDP10_ALL },
  { "caige",	0305,	PDP10_BASIC,	PDP10_ALL },
  { "cain",	0306,	PDP10_BASIC,	PDP10_ALL },
  { "caig",	0307,	PDP10_BASIC,	PDP10_ALL },
  { "cam",	0310,	PDP10_BASIC,	PDP10_ALL },
  { "caml",	0311,	PDP10_BASIC,	PDP10_ALL },
  { "came",	0312,	PDP10_BASIC,	PDP10_ALL },
  { "camle",	0313,	PDP10_BASIC,	PDP10_ALL },
  { "cama",	0314,	PDP10_BASIC,	PDP10_ALL },
  { "camge",	0315,	PDP10_BASIC,	PDP10_ALL },
  { "camn",	0316,	PDP10_BASIC,	PDP10_ALL },
  { "camg",	0317,	PDP10_BASIC,	PDP10_ALL },
  { "jump",	0320,	PDP10_BASIC,	PDP10_ALL },
  { "jumpl",	0321,	PDP10_BASIC,	PDP10_ALL },
  { "jumpe",	0322,	PDP10_BASIC,	PDP10_ALL },
  { "jumple",	0323,	PDP10_BASIC,	PDP10_ALL },
  { "jumpa",	0324,	PDP10_BASIC,	PDP10_ALL },
  { "jumpge",	0325,	PDP10_BASIC,	PDP10_ALL },
  { "jumpn",	0326,	PDP10_BASIC,	PDP10_ALL },
  { "jumpg",	0327,	PDP10_BASIC,	PDP10_ALL },
  { "skip",	0330,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skipl",	0331,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skipe",	0332,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skiple",	0333,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skipa",	0334,	PDP10_A_E_UNUSED, PDP10_ALL },
  { "skipge",	0335,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skipn",	0336,	PDP10_A_UNUSED,	PDP10_ALL },
  { "skipg",	0337,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aoj",	0340,	PDP10_BASIC,	PDP10_ALL },
  { "aojl",	0341,	PDP10_BASIC,	PDP10_ALL },
  { "aoje",	0342,	PDP10_BASIC,	PDP10_ALL },
  { "aojle",	0343,	PDP10_BASIC,	PDP10_ALL },
  { "aoja",	0344,	PDP10_BASIC,	PDP10_ALL },
  { "aojge",	0345,	PDP10_BASIC,	PDP10_ALL },
  { "aojn",	0346,	PDP10_BASIC,	PDP10_ALL },
  { "aojg",	0347,	PDP10_BASIC,	PDP10_ALL },
  { "aos",	0350,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosl",	0351,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aose",	0352,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosle",	0353,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosa",	0354,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosge",	0355,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosn",	0356,	PDP10_A_UNUSED,	PDP10_ALL },
  { "aosg",	0357,	PDP10_A_UNUSED,	PDP10_ALL },
  { "soj",	0360,	PDP10_BASIC,	PDP10_ALL },
  { "sojl",	0361,	PDP10_BASIC,	PDP10_ALL },
  { "soje",	0362,	PDP10_BASIC,	PDP10_ALL },
  { "sojle",	0363,	PDP10_BASIC,	PDP10_ALL },
  { "soja",	0364,	PDP10_BASIC,	PDP10_ALL },
  { "sojge",	0365,	PDP10_BASIC,	PDP10_ALL },
  { "sojn",	0366,	PDP10_BASIC,	PDP10_ALL },
  { "sojg",	0367,	PDP10_BASIC,	PDP10_ALL },
  { "sos",	0370,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosl",	0371,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sose",	0372,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosle",	0373,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosa",	0374,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosge",	0375,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosn",	0376,	PDP10_A_UNUSED,	PDP10_ALL },
  { "sosg",	0377,	PDP10_A_UNUSED,	PDP10_ALL },
  { "setz",	0400,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setzi",	0401,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setzm",	0402,	PDP10_A_UNUSED,	PDP10_ALL },
  { "setzb",	0403,	PDP10_BASIC,	PDP10_ALL },
  { "and",	0404,	PDP10_BASIC,	PDP10_ALL },
  { "andi",	0405,	PDP10_BASIC,	PDP10_ALL },
  { "andm",	0406,	PDP10_BASIC,	PDP10_ALL },
  { "andb",	0407,	PDP10_BASIC,	PDP10_ALL },
  { "andca",	0410,	PDP10_BASIC,	PDP10_ALL },
  { "andcai",	0411,	PDP10_BASIC,	PDP10_ALL },
  { "andcam",	0412,	PDP10_BASIC,	PDP10_ALL },
  { "andcab",	0413,	PDP10_BASIC,	PDP10_ALL },
  { "setm",	0414,	PDP10_BASIC,	PDP10_ALL },
  { "xmovei",	0415,	PDP10_BASIC,	PDP10_KL10up }, /* setmi */
  { "setmm",	0416,	PDP10_A_UNUSED,	PDP10_ALL },
  { "setmb",	0417,	PDP10_BASIC,	PDP10_ALL },
  { "andcm",	0420,	PDP10_BASIC,	PDP10_ALL },
  { "andcmi",	0421,	PDP10_BASIC,	PDP10_ALL },
  { "andcmm",	0422,	PDP10_BASIC,	PDP10_ALL },
  { "andcmb",	0423,	PDP10_BASIC,	PDP10_ALL },
  { "seta",	0424,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setai",	0425,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setam",	0426,	PDP10_BASIC,	PDP10_ALL },
  { "setab",	0427,	PDP10_BASIC,	PDP10_ALL },
  { "xor",	0430,	PDP10_BASIC,	PDP10_ALL },
  { "xori",	0431,	PDP10_BASIC,	PDP10_ALL },
  { "xorm",	0432,	PDP10_BASIC,	PDP10_ALL },
  { "xorb",	0433,	PDP10_BASIC,	PDP10_ALL },
  { "or",	0434,	PDP10_BASIC,	PDP10_ALL },
  { "ori",	0435,	PDP10_BASIC,	PDP10_ALL },
  { "orm",	0436,	PDP10_BASIC,	PDP10_ALL },
  { "orb",	0437,	PDP10_BASIC,	PDP10_ALL },
  { "andcb",	0440,	PDP10_BASIC,	PDP10_ALL },
  { "andcbi",	0441,	PDP10_BASIC,	PDP10_ALL },
  { "andcbm",	0442,	PDP10_BASIC,	PDP10_ALL },
  { "andcbb",	0443,	PDP10_BASIC,	PDP10_ALL },
  { "eqv",	0444,	PDP10_BASIC,	PDP10_ALL },
  { "eqvi",	0445,	PDP10_BASIC,	PDP10_ALL },
  { "eqvm",	0446,	PDP10_BASIC,	PDP10_ALL },
  { "eqvb",	0447,	PDP10_BASIC,	PDP10_ALL },
  { "setca",	0450,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setcai",	0451,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setcam",	0452,	PDP10_BASIC,	PDP10_ALL },
  { "setcab",	0453,	PDP10_BASIC,	PDP10_ALL },
  { "orca",	0454,	PDP10_BASIC,	PDP10_ALL },
  { "orcai",	0455,	PDP10_BASIC,	PDP10_ALL },
  { "orcam",	0456,	PDP10_BASIC,	PDP10_ALL },
  { "orcab",	0457,	PDP10_BASIC,	PDP10_ALL },
  { "setcm",	0460,	PDP10_BASIC,	PDP10_ALL },
  { "setcmi",	0461,	PDP10_BASIC,	PDP10_ALL },
  { "setcmm",	0462,	PDP10_A_UNUSED,	PDP10_ALL },
  { "setcmb",	0463,	PDP10_BASIC,	PDP10_ALL },
  { "orcm",	0464,	PDP10_BASIC,	PDP10_ALL },
  { "orcmi",	0465,	PDP10_BASIC,	PDP10_ALL },
  { "orcmm",	0466,	PDP10_BASIC,	PDP10_ALL },
  { "orcmb",	0467,	PDP10_BASIC,	PDP10_ALL },
  { "orcb",	0470,	PDP10_BASIC,	PDP10_ALL },
  { "orcbi",	0471,	PDP10_BASIC,	PDP10_ALL },
  { "orcbm",	0472,	PDP10_BASIC,	PDP10_ALL },
  { "orcbb",	0473,	PDP10_BASIC,	PDP10_ALL },
  { "seto",	0474,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setoi",	0475,	PDP10_E_UNUSED,	PDP10_ALL },
  { "setom",	0476,	PDP10_A_UNUSED,	PDP10_ALL },
  { "setob",	0477,	PDP10_BASIC,	PDP10_ALL },
  { "hll",	0500,	PDP10_BASIC,	PDP10_ALL },
  { "xhlli",	0501,	PDP10_BASIC,	PDP10_KL10up }, /* hlli */
  { "hllm",	0502,	PDP10_BASIC,	PDP10_ALL },
  { "hlls",	0503,	PDP10_BASIC,	PDP10_ALL },
  { "hrl",	0504,	PDP10_BASIC,	PDP10_ALL },
  { "hrli",	0505,	PDP10_BASIC,	PDP10_ALL },
  { "hrlm",	0506,	PDP10_BASIC,	PDP10_ALL },
  { "hrls",	0507,	PDP10_BASIC,	PDP10_ALL },
  { "hllz",	0510,	PDP10_BASIC,	PDP10_ALL },
  { "hllzi",	0511,	PDP10_BASIC,	PDP10_ALL },
  { "hllzm",	0512,	PDP10_BASIC,	PDP10_ALL },
  { "hllzs",	0513,	PDP10_BASIC,	PDP10_ALL },
  { "hrlz",	0514,	PDP10_BASIC,	PDP10_ALL },
  { "hrlzi",	0515,	PDP10_BASIC,	PDP10_ALL },
  { "hrlzm",	0516,	PDP10_BASIC,	PDP10_ALL },
  { "hrlzs",	0517,	PDP10_BASIC,	PDP10_ALL },
  { "hllo",	0520,	PDP10_BASIC,	PDP10_ALL },
  { "hlloi",	0521,	PDP10_BASIC,	PDP10_ALL },
  { "hllom",	0522,	PDP10_BASIC,	PDP10_ALL },
  { "hllos",	0523,	PDP10_BASIC,	PDP10_ALL },
  { "hrlo",	0524,	PDP10_BASIC,	PDP10_ALL },
  { "hrloi",	0525,	PDP10_BASIC,	PDP10_ALL },
  { "hrlom",	0526,	PDP10_BASIC,	PDP10_ALL },
  { "hrlos",	0527,	PDP10_BASIC,	PDP10_ALL },
  { "hlle",	0530,	PDP10_BASIC,	PDP10_ALL },
  { "hllei",	0531,	PDP10_BASIC,	PDP10_ALL },
  { "hllem",	0532,	PDP10_BASIC,	PDP10_ALL },
  { "hlles",	0533,	PDP10_BASIC,	PDP10_ALL },
  { "hrle",	0534,	PDP10_BASIC,	PDP10_ALL },
  { "hrlei",	0535,	PDP10_BASIC,	PDP10_ALL },
  { "hrlem",	0536,	PDP10_BASIC,	PDP10_ALL },
  { "hrles",	0537,	PDP10_BASIC,	PDP10_ALL },
  { "hrr",	0540,	PDP10_BASIC,	PDP10_ALL },
  { "hrri",	0541,	PDP10_BASIC,	PDP10_ALL },
  { "hrrm",	0542,	PDP10_BASIC,	PDP10_ALL },
  { "hrrs",	0543,	PDP10_BASIC,	PDP10_ALL },
  { "hlr",	0544,	PDP10_BASIC,	PDP10_ALL },
  { "hlri",	0545,	PDP10_BASIC,	PDP10_ALL },
  { "hlrm",	0546,	PDP10_BASIC,	PDP10_ALL },
  { "hlrs",	0547,	PDP10_BASIC,	PDP10_ALL },
  { "hrrz",	0550,	PDP10_BASIC,	PDP10_ALL },
  { "hrrzi",	0551,	PDP10_BASIC,	PDP10_ALL },
  { "hrrzm",	0552,	PDP10_BASIC,	PDP10_ALL },
  { "hrrzs",	0553,	PDP10_BASIC,	PDP10_ALL },
  { "hlrz",	0554,	PDP10_BASIC,	PDP10_ALL },
  { "hlrzi",	0555,	PDP10_BASIC,	PDP10_ALL },
  { "hlrzm",	0556,	PDP10_BASIC,	PDP10_ALL },
  { "hlrzs",	0557,	PDP10_BASIC,	PDP10_ALL },
  { "hrro",	0560,	PDP10_BASIC,	PDP10_ALL },
  { "hrroi",	0561,	PDP10_BASIC,	PDP10_ALL },
  { "hrrom",	0562,	PDP10_BASIC,	PDP10_ALL },
  { "hrros",	0563,	PDP10_BASIC,	PDP10_ALL },
  { "hlro",	0564,	PDP10_BASIC,	PDP10_ALL },
  { "hlroi",	0565,	PDP10_BASIC,	PDP10_ALL },
  { "hlrom",	0566,	PDP10_BASIC,	PDP10_ALL },
  { "hlros",	0567,	PDP10_BASIC,	PDP10_ALL },
  { "hrre",	0570,	PDP10_BASIC,	PDP10_ALL },
  { "hrrei",	0571,	PDP10_BASIC,	PDP10_ALL },
  { "hrrem",	0572,	PDP10_BASIC,	PDP10_ALL },
  { "hrres",	0573,	PDP10_BASIC,	PDP10_ALL },
  { "hlre",	0574,	PDP10_BASIC,	PDP10_ALL },
  { "hlrei",	0575,	PDP10_BASIC,	PDP10_ALL },
  { "hlrem",	0576,	PDP10_BASIC,	PDP10_ALL },
  { "hlres",	0577,	PDP10_BASIC,	PDP10_ALL },
  { "trn",	0600,	PDP10_BASIC,	PDP10_ALL },
  { "tln",	0601,	PDP10_BASIC,	PDP10_ALL },
  { "trne",	0602,	PDP10_BASIC,	PDP10_ALL },
  { "tlne",	0603,	PDP10_BASIC,	PDP10_ALL },
  { "trna",	0604,	PDP10_BASIC,	PDP10_ALL },
  { "tlna",	0605,	PDP10_BASIC,	PDP10_ALL },
  { "trnn",	0606,	PDP10_BASIC,	PDP10_ALL },
  { "tlnn",	0607,	PDP10_BASIC,	PDP10_ALL },
  { "tdn",	0610,	PDP10_BASIC,	PDP10_ALL },
  { "tsn",	0611,	PDP10_BASIC,	PDP10_ALL },
  { "tdne",	0612,	PDP10_BASIC,	PDP10_ALL },
  { "tsne",	0613,	PDP10_BASIC,	PDP10_ALL },
  { "tdna",	0614,	PDP10_BASIC,	PDP10_ALL },
  { "tsna",	0615,	PDP10_BASIC,	PDP10_ALL },
  { "tdnn",	0616,	PDP10_BASIC,	PDP10_ALL },
  { "tsnn",	0617,	PDP10_BASIC,	PDP10_ALL },
  { "trz",	0620,	PDP10_BASIC,	PDP10_ALL },
  { "tlz",	0621,	PDP10_BASIC,	PDP10_ALL },
  { "trze",	0622,	PDP10_BASIC,	PDP10_ALL },
  { "tlze",	0623,	PDP10_BASIC,	PDP10_ALL },
  { "trza",	0624,	PDP10_BASIC,	PDP10_ALL },
  { "tlza",	0625,	PDP10_BASIC,	PDP10_ALL },
  { "trzn",	0626,	PDP10_BASIC,	PDP10_ALL },
  { "tlzn",	0627,	PDP10_BASIC,	PDP10_ALL },
  { "tdz",	0630,	PDP10_BASIC,	PDP10_ALL },
  { "tsz",	0631,	PDP10_BASIC,	PDP10_ALL },
  { "tdze",	0632,	PDP10_BASIC,	PDP10_ALL },
  { "tsze",	0633,	PDP10_BASIC,	PDP10_ALL },
  { "tdza",	0634,	PDP10_BASIC,	PDP10_ALL },
  { "tsza",	0635,	PDP10_BASIC,	PDP10_ALL },
  { "tdzn",	0636,	PDP10_BASIC,	PDP10_ALL },
  { "tszn",	0637,	PDP10_BASIC,	PDP10_ALL },
  { "trc",	0640,	PDP10_BASIC,	PDP10_ALL },
  { "tlc",	0641,	PDP10_BASIC,	PDP10_ALL },
  { "trce",	0642,	PDP10_BASIC,	PDP10_ALL },
  { "tlce",	0643,	PDP10_BASIC,	PDP10_ALL },
  { "trca",	0644,	PDP10_BASIC,	PDP10_ALL },
  { "tlca",	0645,	PDP10_BASIC,	PDP10_ALL },
  { "trcn",	0646,	PDP10_BASIC,	PDP10_ALL },
  { "tlcn",	0647,	PDP10_BASIC,	PDP10_ALL },
  { "tdc",	0650,	PDP10_BASIC,	PDP10_ALL },
  { "tsc",	0651,	PDP10_BASIC,	PDP10_ALL },
  { "tdce",	0652,	PDP10_BASIC,	PDP10_ALL },
  { "tsce",	0653,	PDP10_BASIC,	PDP10_ALL },
  { "tdca",	0654,	PDP10_BASIC,	PDP10_ALL },
  { "tsca",	0655,	PDP10_BASIC,	PDP10_ALL },
  { "tdcn",	0656,	PDP10_BASIC,	PDP10_ALL },
  { "tscn",	0657,	PDP10_BASIC,	PDP10_ALL },
  { "tro",	0660,	PDP10_BASIC,	PDP10_ALL },
  { "tlo",	0661,	PDP10_BASIC,	PDP10_ALL },
  { "troe",	0662,	PDP10_BASIC,	PDP10_ALL },
  { "tloe",	0663,	PDP10_BASIC,	PDP10_ALL },
  { "troa",	0664,	PDP10_BASIC,	PDP10_ALL },
  { "tloa",	0665,	PDP10_BASIC,	PDP10_ALL },
  { "tron",	0666,	PDP10_BASIC,	PDP10_ALL },
  { "tlon",	0667,	PDP10_BASIC,	PDP10_ALL },
  { "tdo",	0670,	PDP10_BASIC,	PDP10_ALL },
  { "tso",	0671,	PDP10_BASIC,	PDP10_ALL },
  { "tdoe",	0672,	PDP10_BASIC,	PDP10_ALL },
  { "tsoe",	0673,	PDP10_BASIC,	PDP10_ALL },
  { "tdoa",	0674,	PDP10_BASIC,	PDP10_ALL },
  { "tsoa",	0675,	PDP10_BASIC,	PDP10_ALL },
  { "tdon",	0676,	PDP10_BASIC,	PDP10_ALL },
  { "tson",	0677,	PDP10_BASIC,	PDP10_ALL },

#if 1
  /*
   * KA10 ITS system instructions.
   */

  { "lpm",	010200,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
  { "spm",	010204,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
  { "lpmr",	010210,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
  { "lpmri",	010230,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
  { "xctr",	010300,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
  { "xctri",	010320,	PDP10_A_OPCODE,	PDP10_KA10_ITS },
#endif

#if 1
  /*
   * KL10 ITS system instructions.
   */

  { "lpm",	010200,	PDP10_A_OPCODE,	PDP10_KL10_ITS },
  { "spm",	010204,	PDP10_A_OPCODE,	PDP10_KL10_ITS },
  { "lpmr",	010210,	PDP10_A_OPCODE,	PDP10_KL10_ITS },
  { "lpmri",	010230,	PDP10_A_OPCODE,	PDP10_KL10_ITS },

  { "xctr",	0074,	PDP10_BASIC,	PDP10_KL10_ITS },
  { "xctri",	0075,	PDP10_BASIC,	PDP10_KL10_ITS },
  { "lpmr",	0076,	PDP10_BASIC,	PDP10_KL10_ITS },
  { "spm",	0077,	PDP10_BASIC,	PDP10_KL10_ITS },
#endif

  /*
   * KL10 I/O and system instructions.
   */

  { "aprid",	070000,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_KS10 |
					PDP10_XKL1 },
  { "wrfil",	070010,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "rdera",	070040,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_KS10 |
					PDP10_XKL1 },
  { "swpia",	070144,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1 },
  { "swpva",	070150,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1 },
  { "swpua",	070154,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1 },
  { "swpio",	070164,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1 },
  { "swpvo",	070170,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1 },
  { "swpuo",	070174,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1 },
  { "rdperf",	070200,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "rdtime",	070204,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "wrpae",	070210,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "rdmact",	070240,	PDP10_A_OPCODE,	PDP10_KL10any },
  { "rdeact",	070244,	PDP10_A_OPCODE,	PDP10_KL10any },

#if 1
  /*
   * KS10 ITS system instructions.
   */

  { "xctr",	0102,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "xctri",	0103,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "aprid",	070000,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
#if 0
  /* CONO and CONI appear to be preferred to these mnemonics in */
  /* ITS source code. */
  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdapr",	070024,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrpi",	070060,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdpi",	070064,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
#endif
  { "clrcsh",	070100,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdubr",	070104,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrubr",	070114,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrebr",	070120,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdebr",	070124,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "sdbr1",	070200,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "sdbr2",	070204,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "sdbr3",	070210,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "sdbr4",	070214,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdtim",	070220,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdint",	070224,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "rdhsb",	070230,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "spm",	070234,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "ldbr1",	070240,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "ldbr2",	070244,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "ldbr3",	070250,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "ldbr4",	070254,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrtim",	070260,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrint",	070264,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "wrhsb",	070270,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "lpmr",	070274,	PDP10_A_OPCODE,	PDP10_KS10_ITS },
  { "umove",	0704,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "umovem",	0705,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iordi",	0710,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iordq",	0711,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iord",	0712,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowr",	0713,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowri",	0714,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowrq",	0715,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "bltbu",	0716,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "bltub",	0717,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iordbi",	0720,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iordbq",	0721,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iordb",	0722,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowrb",	0723,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowrbi",	0724,	PDP10_BASIC,	PDP10_KS10_ITS },
  { "iowrbq",	0725,	PDP10_BASIC,	PDP10_KS10_ITS },

#else
  /*
   * KS10 system instructions.
   */

  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdapr",	070024,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrpi",	070060,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdpi",	070064,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdubr",	070104,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrubr",	070114,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrebr",	070120,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "rdebr",	070124,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "rdspb",	070200,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdcsb",	070204,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdpur",	070210,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdcstm",	070214,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "rdtim",	070220,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "rdint",	070224,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "rdhsb",	070230,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "wrspb",	070240,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrcsb",	070244,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrpur",	070250,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrcstm",	070254,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1 },
  { "wrtim",	070260,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "wrint",	070264,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "wrhsb",	070270,	PDP10_A_OPCODE,	PDP10_KS10 },
  { "umove",	0704,	PDP10_BASIC,	PDP10_KS10 },
  { "umovem",	0705,	PDP10_BASIC,	PDP10_KS10 },
  { "tioe",	0710,	PDP10_BASIC,	PDP10_KS10 },
  { "tion",	0711,	PDP10_BASIC,	PDP10_KS10 },
  { "rdio",	0712,	PDP10_BASIC,	PDP10_KS10 },
  { "wrio",	0713,	PDP10_BASIC,	PDP10_KS10 },
  { "bsio",	0714,	PDP10_BASIC,	PDP10_KS10 },
  { "bcio",	0715,	PDP10_BASIC,	PDP10_KS10 },
  { "tioeb",	0720,	PDP10_BASIC,	PDP10_KS10 },
  { "tionb",	0721,	PDP10_BASIC,	PDP10_KS10 },
  { "rdiob",	0722,	PDP10_BASIC,	PDP10_KS10 },
  { "wriob",	0723,	PDP10_BASIC,	PDP10_KS10 },
  { "bsiob",	0724,	PDP10_BASIC,	PDP10_KS10 },
  { "bciob",	0725,	PDP10_BASIC,	PDP10_KS10 },
#endif

  /*
   * XKL-1 system instructions.
   */

  { "rdadb",	070004,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "sysid",	070010,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wradb",	070014,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "szapr",	070030,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "snapr",	070034,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wctrlf",	070040,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rctrlf",	070044,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "simird",	070050,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrkpa",	070054,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "szpi",	070070,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "snpi",	070074,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "apr0",	0700,	PDP10_BASIC,	PDP10_XKL1 },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrerr",	070120,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rderr",	070124,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrctx",	070130,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rdctx",	070134,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rddcsh",	070140,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "dwrcsh",	070160,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "swpio",	070164,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "swpvo",	070170,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "swpuo",	070174,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "apr1",	0701,	PDP10_BASIC,	PDP10_XKL1 },
  { "rditm",	070220,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rdtime",	070224,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "drdptb",	070230,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrtime",	070234,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "writm",	070260,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "dwrptb",	070270,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "apr2",	0702,	PDP10_BASIC,	PDP10_XKL1 },
  { "rdcty",	070304,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrcty",	070314,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "wrctys",	070320,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "rdctys",	070324,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "szcty",	070330,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "sncty",	070334,	PDP10_A_OPCODE,	PDP10_XKL1 },
  { "apr3",	0703,	PDP10_BASIC,	PDP10_XKL1 },
  { "pmove",	0704,	PDP10_BASIC,	PDP10_XKL1 },
  { "pmovem",	0705,	PDP10_BASIC,	PDP10_XKL1 },
  { "nmove",	0706,	PDP10_BASIC,	PDP10_XKL1 },
  { "nmovem",	0707,	PDP10_BASIC,	PDP10_XKL1 },
  { "ldlpn",	0710,	PDP10_BASIC,	PDP10_XKL1 },
  { "rdcfg",	0711,	PDP10_BASIC,	PDP10_XKL1 },
  { "amove",	0714,	PDP10_BASIC,	PDP10_XKL1 },
  { "amovem",	0715,	PDP10_BASIC,	PDP10_XKL1 },
  { "umove",	0716,	PDP10_BASIC,	PDP10_XKL1 },
  { "umovem",	0717,	PDP10_BASIC,	PDP10_XKL1 },

  /*
   * 166 / KA10 / KI10 / KL10 I/O instructions.
   *
   * These should come after the more specific instructions above.
   */

#define PDP10_not_KS10_or_XKL1 (PDP10_ALL & ~(PDP10_KS10 | \
				PDP10_XKL1))

  { "blki",	070000,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "datai",	070004,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "blko",	070010,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "datao",	070014,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "cono",	070020,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "coni",	070024,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "consz",	070030,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
  { "conso",	070034,	PDP10_IO,	PDP10_not_KS10_or_XKL1 },
};

/*
 * Internal devices.
 * (External devices are listed, but disabled.)
 */

const struct pdp10_device pdp10_device[] =
{
  /* name,	code,	models */
  { "apr",	0000,	PDP10_KA10_to_KL10 },	/* Arithmetic processor */
  { "pi",	0004,	PDP10_KA10_to_KL10 },	/* Priority interrupt */
  { "pag",	0010,	PDP10_KI10_to_KL10 },	/* Pager */
  { "cca",	0014,	PDP10_KL10any },	/* Cache */
#if 0
  { "dsdev",	0020,	PDP10_KA10_ITS },    	/* Deselection device */
  { "?",	0024,	PDP10_KA10_ITS },       /* ? */
#endif
  { "tim",	0020,	PDP10_KL10any },	/* Timer */
  { "mtr",	0024,	PDP10_KL10any },	/* Meters */
  { "dlb",	0060,	PDP10_KA10_to_KL10 },	/* DL10 base */
  { "dlc",	0064,	PDP10_KA10_to_KL10 },	/* DL10 control */
  { "stk",	0070,	PDP10_KA10_ITS },	/* Stanford keyboard */
  { "ptp",	0100,	PDP10_KA10 },   	/* Paper tape punch */
  { "ptr",	0104,	PDP10_KI10 },		/* Paper tape reader */
  { "tty",	0120,	PDP10_KA10 },   	/* Console TTY */
#if 0
  { "olpt",	0124,	PDP10_ITS },		/* Line printer */
  { "dis",	0130,	PDP10_KA10_ITS },	/* 340 display */
  { "dpc",	0250,	PDP10_ITS },		/* RP10 disk control */
  { "dsk",	0270,	PDP10_ITS },		/* RH10 disk control */
  { "dtc",	0320,	PDP10_KA10_ITS },	/* DECtape control */
  { "dts",	0324,	PDP10_KA10_ITS },	/* DECtape status */
  { "mtc",	0340,	PDP10_ITS },		/* Mag tape control */
  { "mts",	0344,	PDP10_ITS },		/* Mag tape status */
  { "mty",	0400,	PDP10_ITS },
  { "fi",	0424,	PDP10_ITS },		/* IMP hardware */
  { "imp",	0460,	PDP10_KA10_ITS |
			PDP10_KL10_ITS },	/* IMP interface */
  { "nlpt",	0464,	PDP10_ITS },		/* Line printer */
  { "pdclk",	0500,	PDP10_ITS },		/* De-Coriolis clock */
  { "tipdev",	0504,	PDP10_ITS },		/* TIP break device */
  { "rbtcon",	0514,	PDP10_ITS },		/* Robot console */
  { "ompx",	0570,	PDP10_ITS },		/* Output multiplexor */
  { "mpx",	0574,	PDP10_ITS },		/* Input multiplexor */
  { "nty",	0600,	PDP10_ITS },		/* Knight TTY kludge */
  { "dpk",	0604,	PDP10_ITS },		/* Data point kludge */
  { "dc0",	0610,	PDP10_ITS },		/* 2314 disk control */
  { "dc1",	0614,	PDP10_ITS },		/* 2314 disk control */
  { "nvdx",	0620,	PDP10_ITS },		/* New vidi x */
  { "nvdy",	0624,	PDP10_ITS },		/* New vidi y */
  { "nvdt",	0630,	PDP10_ITS },		/* New vidi t */
  { "plt",	0654,	PDP10_ITS },		/* Cal comp plotter */
  { "clk1",	0710,	PDP10_ITS },		/* Holloway clock */
  { "clk2",	0714,	PDP10_ITS },		/* Holloway clock */
#endif
};

const struct pdp10_instruction pdp10_alias[] =
{
  /* name,	opcode,	type,		models */
  { "pxct",	0256,	PDP10_BASIC,	PDP10_ALL }, /* A_SPECIAL_CODE */
  { "setmi",	0415,	PDP10_BASIC,	PDP10_ALL },
  { "ior",	0434,	PDP10_BASIC,	PDP10_ALL },
  { "iori",	0435,	PDP10_BASIC,	PDP10_ALL },
  { "iorm",	0436,	PDP10_BASIC,	PDP10_ALL },
  { "iorb",	0437,	PDP10_BASIC,	PDP10_ALL },
  { "hlli",	0501,	PDP10_BASIC,	PDP10_ALL },
};

const struct pdp10_instruction pdp10_extended_instruction[] =
{
  /* name,	opcode,	type,			models */
  { "cmpsl",	0001,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cmpse",	0002,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cmpsle",	0003,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "edit",	0004,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cmpsge",	0005,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cmpsn",	0006,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cmpsg",	0007,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "cvtdbo",	0010,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "cvtdbt",	0011,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "cvtbdo",	0012,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "cvtbdt",	0013,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "movso",	0014,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "movst",	0015,	PDP10_A_UNUSED,		PDP10_KL10up },
  { "movslj",	0016,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "movsrj",	0017,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "xblt",	0020,	PDP10_A_E_UNUSED,	PDP10_KL10up },
  { "gsngl",	0021,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gdble",	0022,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gdfix",	0023,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gdfixr",	0025,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gfix",	0024,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gfixr",	0026,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "dgfltr",	0027,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gfltr",	0030,	PDP10_A_UNUSED,		PDP10_KL10_271 },
  { "gfsc",	0031,	PDP10_A_UNUSED,		PDP10_KL10_271 },
};

const int pdp10_num_instructions = sizeof pdp10_instruction /
                                   sizeof pdp10_instruction[0];
const int pdp10_num_devices = sizeof pdp10_device / sizeof pdp10_device[0];
const int pdp10_num_aliases = sizeof pdp10_alias / sizeof pdp10_alias[0];
