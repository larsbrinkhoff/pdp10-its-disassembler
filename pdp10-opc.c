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
#include "symbols.h"

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
  /* name,	opcode,	type,		models,   hint */

#if 1 /* ITS MUUOs */
  { ".iot",	0040,	PDP10_BASIC,	PDP10_ITS, HINT_CHANNEL, 0 },
  { ".open",	0041,	PDP10_BASIC,	PDP10_ITS, HINT_CHANNEL, 0 },
  { ".oper",	0042,	PDP10_BASIC,	PDP10_ITS, 0, 0 },
  { ".call",	004300,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".dismis",	004304,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
#if 1
  { ".lose",	004310,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
#else
  { ".trans",	004310,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
#endif
  { ".tranad",	004314,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".value",	004320,	PDP10_A_OPCODE|PDP10_E_UNUSED, PDP10_ITS, 0, 0 },
  { ".utran",	004324,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".core",	004330,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".trand",	004334,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".dstart",	004340,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".fdele",	004344,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".dstrtl",	004350,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".suset",	004354,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".ltpen",	004360,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".vscan",	004364,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".potset",	004370,	PDP10_A_OPCODE,	PDP10_ITS, 0, 0 },
  { ".uset",	0044,	PDP10_BASIC,	PDP10_ITS, HINT_CHANNEL, 0 },
  { ".break",	0045,	PDP10_BASIC,	PDP10_ITS, HINT_NUMBER, 0 },
  { ".status",	0046,	PDP10_BASIC,	PDP10_ITS, HINT_CHANNEL, 0 },
  { ".access",	0047,	PDP10_BASIC,	PDP10_ITS, HINT_CHANNEL, 0 },
#endif

  /* WAITS UUOs */
  { "call",	0040,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "init",	0041,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "spcwar",	0043,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "calli",	0047,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "open",	0050,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "inchrw",	005100,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "outchr",	005104,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "inchrs",	005110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "outstr",	005114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "inchwl",	005120,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "inchsl",	005124,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "getlin",	005130,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "setlin",	005134,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "rescan",	005140,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "clrbfi",	005144,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "clrbfo",	005150,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "inskip",	005154,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "inwait",	005160,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "setact",	005164,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ttread",	005170,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "outfiv",	005174,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "rename",	0055,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "in",	0056,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "out",	0057,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "setsts",	0060,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "stato",	0061,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "getsts",	0062,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "statz",	0063,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "inbuf",	0064,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "outbuf",	0065,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "input",	0066,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "output",	0067,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "close",	0070,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "releas",	0071,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "mtape",	0072,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "ugetf",	0073,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "useti",	0074,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "useto",	0075,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "lookup",	0076,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "enter",	0077,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "dpyclr",	0701,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "ppsel",	070200,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ppact",	070204,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "dpypos",	070210,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "dpysiz",	070214,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "pprel",	070220,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ppinfo",	070224,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "leypos",	070230,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "pphld",	070234,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "cursor",	070240,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "upgiot",	0703,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "uinbf",	0704,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "uoutbf",	0705,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "send",	071000,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "wrcv",	071004,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "srcv",	071010,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "skpme",	071014,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "skphim",	071020,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "skpsen",	071024,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptyget",	071100,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptyrel",	071104,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptifre",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptocnt",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptrd1s",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptrd1w",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptwr1s",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptwr1w",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptrds",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptwrs7",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptwrs9",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptgetl",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptsetl",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptload",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptjobx",	071110,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ptl7w9",	071114,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "points",	0712,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "upgmve",	0713,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "upgmvm",	0714,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "pgsel",	071500,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "pgact",	071504,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "pgclr",	071510,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "ddupg",	071514,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "pginfo",	071520,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "chnsts",	0716,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "clkint",	0717,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "intmsk",	0720,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "imskst",	0721,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "imskcl",	0722,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "intdej",	072300,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "imstw",	072304,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "iwkmsk",	072310,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "intdmp",	072314,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "intipi",	072320,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "imskcr",	072324,	PDP10_A_OPCODE,	PDP10_SAIL, 0, 0 },
  { "iopush",	0724,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "iopop",	0725,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },
  { "iopdl",	0726,	PDP10_BASIC,	PDP10_SAIL, 0, 0 },

  /* TOPS-xx instruction */
  { "ujen",	0100,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },

/*{ "",		0101,	PDP10_BASIC,	PDP10_NONE },*/
  { "gfad",	0102,	PDP10_BASIC,	PDP10_KL10_271, 0, 0 },
  { "gfsb",	0103,	PDP10_BASIC,	PDP10_KL10_271, 0, 0 },

  /* TOPS-20 instruction */
  { "jsys",	0104,	PDP10_BASIC,	PDP10_T20,   0, 0 },

  { "adjsp",	0105,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "gfmp",	0106,	PDP10_BASIC,	PDP10_KL10_271, 0, 0 },
  { "gfdv",	0107,	PDP10_BASIC,	PDP10_KL10_271, 0, 0 },
  /* The ITS DECUUO emulator accepts opcode 110 as FIX. */
//{ "fix",	0110,	PDP10_BASIC,	PDP10_KA10_ITS, 0, 0 },
  { "dfad",	0110,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dfsb",	0111,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dfmp",	0112,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dfdv",	0113,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dadd",	0114,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "dsub",	0115,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "dmul",	0116,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "ddiv",	0117,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "dmove",	0120,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dmovn",	0121,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "fix",	0122,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "extend",	0123,	PDP10_EXTEND,	PDP10_KL10up, 0, 0 },
  { "dmovem",	0124,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "dmovnm",	0125,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "fixr",	0126,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "fltr",	0127,	PDP10_BASIC,	PDP10_KI10up, 0, 0 },
  { "ufa",	0130,	PDP10_BASIC,	PDP10_KA10_to_KI10, 0, 0 },
  { "dfn",	0131,	PDP10_BASIC,	PDP10_KA10_to_KI10, 0, 0 },
  { "fsc",	0132,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "ibp",	013300,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "adjbp",	0133,	PDP10_BASIC,	PDP10_KL10up, 0, 0 },
  { "ildb",	0134,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "ldb",	0135,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "idpb",	0136,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "dpb",	0137,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fad",	0140,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fadl",	0141,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10, 0, 0 },
  { "fadm",	0142,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fadb",	0143,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fadr",	0144,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fadri",	0145,	PDP10_BASIC,	PDP10_KA10up, 0, HINT_FLOAT },
  { "fadrl",	0145,	PDP10_BASIC,	PDP6_166, 0, 0 },
  { "fadrm",	0146,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fadrb",	0147,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsb",	0150,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsbl",	0151,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10, 0, 0 },
  { "fsbm",	0152,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsbb",	0153,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsbr",	0154,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsbri",	0155,	PDP10_BASIC,	PDP10_KA10up, 0, HINT_FLOAT },
  { "fsbrl",	0155,	PDP10_BASIC,	PDP6_166, 0, 0 },
  { "fsbrm",	0156,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fsbrb",	0157,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmp",	0160,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmpl",	0161,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10, 0, 0 },
  { "fmpm",	0162,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmpb",	0163,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmpr",	0164,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmpri",	0165,	PDP10_BASIC,	PDP10_KA10up, 0, HINT_FLOAT },
  { "fmprl",	0165,	PDP10_BASIC,	PDP6_166, 0, 0 },
  { "fmprm",	0166,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fmprb",	0167,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdv",	0170,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdvl",	0171,	PDP10_BASIC,	PDP6_166_to_PDP10_KI10, 0, 0 },
  { "fdvm",	0172,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdvb",	0173,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdvr",	0174,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdvri",	0175,	PDP10_BASIC,	PDP10_KA10up, 0, HINT_FLOAT },
  { "fdvrl",	0175,	PDP10_BASIC,	PDP6_166, 0, 0 },
  { "fdvrm",	0176,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fdvrb",	0177,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "move",	0200,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movei",	0201,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "movem",	0202,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "moves",	0203,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movs",	0204,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movsi",	0205,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "movsm",	0206,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movss",	0207,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movn",	0210,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movni",	0211,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "movnm",	0212,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movns",	0213,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movm",	0214,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movmi",	0215,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "movmm",	0216,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "movms",	0217,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "imul",	0220,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "imuli",	0221,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "imulm",	0222,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "imulb",	0223,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "mul",	0224,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "muli",	0225,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "mulm",	0226,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "mulb",	0227,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "idiv",	0230,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "idivi",	0231,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "idivm",	0232,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "idivb",	0233,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "div",	0234,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "divi",	0235,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "divm",	0236,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "divb",	0237,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "ash",	0240,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "rot",	0241,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "lsh",	0242,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jffo",	0243,	PDP10_BASIC,	PDP10_KA10up, 0, 0 },
  { "ashc",	0244,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "rotc",	0245,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "lshc",	0246,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "fix",	0247,	PDP10_BASIC,	PDP10_KA10_SAIL, 0, 0 },
  { "exch",	0250,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "blt",	0251,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aobjp",	0252,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aobjn",	0253,	PDP10_BASIC,	PDP10_ALL, 0, 0 },

  /*
   * JRST instruction family.
   *
   * Mnemonics for the most common instructions are recognized.  The
   * rest are caught by the last entry.
   */
  { "portal",	025404,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "jrstf",	025410,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "halt",	025420,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "xjrstf",	025424,	PDP10_A_OPCODE,	PDP10_KL10up, 0, 0 },
  { "xjen",	025430,	PDP10_A_OPCODE,	PDP10_KL10up, 0, 0 },
  { "xpcw",	025434,	PDP10_A_OPCODE,	PDP10_KL10up, 0, 0 },
/*{ "jrstil",	025440,	PDP10_A_OPCODE,	PDP10_ALL },*/
  { "jen",	025450,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "sfm",	025460,	PDP10_A_OPCODE,	PDP10_KL10up, 0, 0 },
  { "jrst",	0254,	PDP10_A_UNUSED,	PDP10_ALL, HINT_NUMBER, 0 },

  /*
   * JFCL instruction family.
   *
   * Mnemonics for common flags (floating overflow, carry 0, carry 1,
   * both carries, overflow) are recognized.  Exotic combinations are
   * caught by the last entry.
   */
  { "jfcl",	025500,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_ALL, 0, 0 },
/*{ "jpcch",	025504,	PDP10_A_OPCODE,	PDP6_166 },*/
  { "jfov",	025504,	PDP10_A_OPCODE,	PDP10_KA10up, 0, 0 },
  { "jcry1",	025510,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "jcry0",	025520,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "jcry",	025530,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "jov",	025540,	PDP10_A_OPCODE,	PDP10_ALL, 0, 0 },
  { "jfcl",	0255,	PDP10_BASIC,	PDP10_ALL, HINT_NUMBER, 0 },

  { "xct",	0256,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },

  /* TOPS-10 instruction, CONS at SAIL, nop otherwise */
  { "map",	0257,	PDP10_BASIC,	PDP10_KA10_to_KI10, 0, 0 },
  { "cons",	0257,	PDP10_BASIC,	PDP10_KA10_SAIL, 0, 0 },

  { "pushj",	0260,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "push",	0261,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "pop",	0262,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "popj",	0263,	PDP10_E_UNUSED,	PDP10_ALL, 0, 0 },
  { "jsr",	0264,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "jsp",	0265,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jsa",	0266,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jra",	0267,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "add",	0270,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "addi",	0271,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "addm",	0272,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "addb",	0273,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sub",	0274,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "subi",	0275,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "subm",	0276,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "subb",	0277,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "cai",	0300,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "cail",	0301,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "caie",	0302,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "caile",	0303,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "caia",	0304,	PDP10_A_E_UNUSED, PDP10_ALL, 0, HINT_IMMEDIATE },
  { "caige",	0305,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "cain",	0306,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "caig",	0307,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "cam",	0310,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "caml",	0311,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "came",	0312,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "camle",	0313,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "cama",	0314,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "camge",	0315,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "camn",	0316,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "camg",	0317,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jump",	0320,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpl",	0321,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpe",	0322,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumple",	0323,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpa",	0324,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpge",	0325,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpn",	0326,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "jumpg",	0327,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "skip",	0330,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skipl",	0331,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skipe",	0332,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skiple",	0333,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skipa",	0334,	PDP10_A_E_UNUSED, PDP10_ALL, 0, 0 },
  { "skipge",	0335,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skipn",	0336,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "skipg",	0337,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aoj",	0340,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aojl",	0341,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aoje",	0342,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aojle",	0343,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aoja",	0344,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aojge",	0345,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aojn",	0346,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aojg",	0347,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "aos",	0350,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosl",	0351,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aose",	0352,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosle",	0353,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosa",	0354,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosge",	0355,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosn",	0356,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "aosg",	0357,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "soj",	0360,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sojl",	0361,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "soje",	0362,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sojle",	0363,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "soja",	0364,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sojge",	0365,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sojn",	0366,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sojg",	0367,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "sos",	0370,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosl",	0371,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sose",	0372,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosle",	0373,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosa",	0374,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosge",	0375,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosn",	0376,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "sosg",	0377,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "setz",	0400,	PDP10_E_UNUSED,	PDP10_ALL, 0, 0 },
  { "setzi",	0401,	PDP10_E_UNUSED,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "setzm",	0402,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "setzb",	0403,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "and",	0404,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andi",	0405,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "andm",	0406,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andb",	0407,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andca",	0410,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcai",	0411,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "andcam",	0412,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcab",	0413,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setm",	0414,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "xmovei",	0415,	PDP10_BASIC,	PDP10_KL10up, 0, HINT_IMMEDIATE }, /* setmi */
  { "setmm",	0416,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "setmb",	0417,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcm",	0420,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcmi",	0421,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "andcmm",	0422,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcmb",	0423,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "seta",	0424,	PDP10_E_UNUSED,	PDP10_ALL, 0, 0 },
  { "setai",	0425,	PDP10_E_UNUSED,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "setam",	0426,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setab",	0427,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "xor",	0430,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "xori",	0431,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "xorm",	0432,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "xorb",	0433,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "ior",	0434,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "iori",	0435,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "iorm",	0436,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "iorb",	0437,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcb",	0440,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcbi",	0441,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "andcbm",	0442,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "andcbb",	0443,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "eqv",	0444,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "eqvi",	0445,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "eqvm",	0446,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "eqvb",	0447,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setca",	0450,	PDP10_E_UNUSED,	PDP10_ALL, 0, 0 },
  { "setcai",	0451,	PDP10_E_UNUSED,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "setcam",	0452,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setcab",	0453,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orca",	0454,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcai",	0455,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "orcam",	0456,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcab",	0457,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setcm",	0460,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "setcmi",	0461,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "setcmm",	0462,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "setcmb",	0463,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcm",	0464,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcmi",	0465,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "orcmm",	0466,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcmb",	0467,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcb",	0470,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcbi",	0471,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "orcbm",	0472,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orcbb",	0473,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "seto",	0474,	PDP10_E_UNUSED,	PDP10_ALL, 0, 0 },
  { "setoi",	0475,	PDP10_E_UNUSED,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "setom",	0476,	PDP10_A_UNUSED,	PDP10_ALL, 0, 0 },
  { "setob",	0477,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hll",	0500,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "xhlli",	0501,	PDP10_BASIC,	PDP10_KL10up, 0, HINT_IMMEDIATE }, /* hlli */
  { "hllm",	0502,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlls",	0503,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrl",	0504,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrli",	0505,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrlm",	0506,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrls",	0507,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllz",	0510,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllzi",	0511,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hllzm",	0512,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllzs",	0513,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlz",	0514,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlzi",	0515,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrlzm",	0516,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlzs",	0517,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllo",	0520,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlloi",	0521,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hllom",	0522,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllos",	0523,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlo",	0524,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrloi",	0525,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrlom",	0526,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlos",	0527,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlle",	0530,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hllei",	0531,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hllem",	0532,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlles",	0533,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrle",	0534,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrlei",	0535,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrlem",	0536,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrles",	0537,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrr",	0540,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrri",	0541,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrrm",	0542,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrrs",	0543,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlr",	0544,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlri",	0545,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hlrm",	0546,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlrs",	0547,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrrz",	0550,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrrzi",	0551,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrrzm",	0552,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrrzs",	0553,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlrz",	0554,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlrzi",	0555,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hlrzm",	0556,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlrzs",	0557,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrro",	0560,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrroi",	0561,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrrom",	0562,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrros",	0563,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlro",	0564,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlroi",	0565,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hlrom",	0566,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlros",	0567,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrre",	0570,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrrei",	0571,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hrrem",	0572,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hrres",	0573,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlre",	0574,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlrei",	0575,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "hlrem",	0576,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlres",	0577,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "trn",	0600,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tln",	0601,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trne",	0602,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlne",	0603,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trna",	0604,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlna",	0605,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trnn",	0606,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlnn",	0607,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tdn",	0610,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsn",	0611,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdne",	0612,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsne",	0613,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdna",	0614,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsna",	0615,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdnn",	0616,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsnn",	0617,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "trz",	0620,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlz",	0621,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trze",	0622,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlze",	0623,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trza",	0624,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlza",	0625,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trzn",	0626,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlzn",	0627,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tdz",	0630,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsz",	0631,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdze",	0632,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsze",	0633,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdza",	0634,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsza",	0635,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdzn",	0636,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tszn",	0637,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "trc",	0640,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlc",	0641,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trce",	0642,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlce",	0643,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trca",	0644,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlca",	0645,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "trcn",	0646,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlcn",	0647,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tdc",	0650,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsc",	0651,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdce",	0652,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsce",	0653,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdca",	0654,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsca",	0655,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdcn",	0656,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tscn",	0657,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tro",	0660,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlo",	0661,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "troe",	0662,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tloe",	0663,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "troa",	0664,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tloa",	0665,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tron",	0666,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tlon",	0667,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "tdo",	0670,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tso",	0671,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdoe",	0672,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsoe",	0673,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdoa",	0674,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tsoa",	0675,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tdon",	0676,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "tson",	0677,	PDP10_BASIC,	PDP10_ALL, 0, 0 },

#if 1
  /*
   * KA10 ITS system instructions.
   */

  { "lpm",	010200,	PDP10_A_OPCODE,	PDP10_KA10_ITS, 0, 0 },
  { "spm",	010204,	PDP10_A_OPCODE,	PDP10_KA10_ITS, 0, 0 },
  { "lpmr",	010210,	PDP10_A_OPCODE,	PDP10_KA10_ITS, 0, 0 },
  { "lpmri",	010230,	PDP10_A_OPCODE,	PDP10_KA10_ITS, 0, HINT_IMMEDIATE },
  { "xctri",	010320,	PDP10_A_XCTRI,	PDP10_KA10_ITS, HINT_XCTR, 0 },
  { "xctr",	0103,	PDP10_BASIC,	PDP10_KA10_ITS, HINT_XCTR, 0 },
#endif

#if 1
  /*
   * KL10 ITS system instructions.
   */

  { "lpm",	010200,	PDP10_A_OPCODE,	PDP10_KL10_ITS, 0, 0 },
  { "spm",	010204,	PDP10_A_OPCODE,	PDP10_KL10_ITS, 0, 0 },
  { "lpmr",	010210,	PDP10_A_OPCODE,	PDP10_KL10_ITS, 0, 0 },
  { "lpmri",	010230,	PDP10_A_OPCODE,	PDP10_KL10_ITS, 0, HINT_IMMEDIATE },

  { "xctr",	0074,	PDP10_BASIC,	PDP10_KL10_ITS, HINT_XCTR, 0 },
  { "xctri",	0075,	PDP10_BASIC,	PDP10_KL10_ITS, HINT_XCTR, 0 },
  { "lpmr",	0076,	PDP10_BASIC,	PDP10_KL10_ITS, 0, 0 },
  { "spm",	0077,	PDP10_BASIC,	PDP10_KL10_ITS, 0, 0 },
#endif

  /*
   * KL10 I/O and system instructions.
   */

  { "aprid",	070000,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_KS10 |
					PDP10_XKL1, 0, 0 },
  { "wrfil",	070010,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "rdera",	070040,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_KS10 |
					PDP10_XKL1, 0, 0 },
  { "swpia",	070144,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "swpva",	070150,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "swpua",	070154,	PDP10_A_OPCODE | PDP10_E_UNUSED,
					PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "swpio",	070164,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "swpvo",	070170,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "swpuo",	070174,	PDP10_A_OPCODE,	PDP10_KL10any | PDP10_XKL1, 0, 0 },
  { "rdperf",	070200,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "rdtime",	070204,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "wrpae",	070210,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "rdmact",	070240,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },
  { "rdeact",	070244,	PDP10_A_OPCODE,	PDP10_KL10any, 0, 0 },

#if 1
  /*
   * KS10 ITS system instructions.
   */

  { "xctr",	0102,	PDP10_BASIC,	PDP10_KS10_ITS, HINT_XCTR, 0 },
  { "xctri",	0103,	PDP10_BASIC,	PDP10_KS10_ITS, HINT_XCTR, 0 },
  { "aprid",	070000,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
#if 0
  /* CONO and CONI appear to be preferred to these mnemonics in */
  /* ITS source code. */
  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdapr",	070024,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrpi",	070060,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdpi",	070064,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
#endif
  { "clrcsh",	070100,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdubr",	070104,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrubr",	070114,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrebr",	070120,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdebr",	070124,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "sdbr1",	070200,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "sdbr2",	070204,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "sdbr3",	070210,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "sdbr4",	070214,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdtim",	070220,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdint",	070224,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "rdhsb",	070230,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "spm",	070234,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "ldbr1",	070240,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "ldbr2",	070244,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "ldbr3",	070250,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "ldbr4",	070254,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrtim",	070260,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrint",	070264,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "wrhsb",	070270,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "lpmr",	070274,	PDP10_A_OPCODE,	PDP10_KS10_ITS, 0, 0 },
  { "umove",	0704,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "umovem",	0705,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iordi",	0710,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iordq",	0711,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iord",	0712,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowr",	0713,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowri",	0714,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowrq",	0715,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "bltbu",	0716,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "bltub",	0717,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iordbi",	0720,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iordbq",	0721,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iordb",	0722,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowrb",	0723,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowrbi",	0724,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },
  { "iowrbq",	0725,	PDP10_BASIC,	PDP10_KS10_ITS, 0, 0 },

#else
  /*
   * KS10 system instructions.
   */

  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdapr",	070024,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrpi",	070060,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdpi",	070064,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdubr",	070104,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrubr",	070114,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrebr",	070120,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "rdebr",	070124,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "rdspb",	070200,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdcsb",	070204,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdpur",	070210,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdcstm",	070214,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "rdtim",	070220,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "rdint",	070224,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "rdhsb",	070230,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "wrspb",	070240,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrcsb",	070244,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrpur",	070250,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrcstm",	070254,	PDP10_A_OPCODE,	PDP10_KS10 | PDP10_XKL1, 0, 0 },
  { "wrtim",	070260,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "wrint",	070264,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "wrhsb",	070270,	PDP10_A_OPCODE,	PDP10_KS10, 0, 0 },
  { "umove",	0704,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "umovem",	0705,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "tioe",	0710,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "tion",	0711,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "rdio",	0712,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "wrio",	0713,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "bsio",	0714,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "bcio",	0715,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "tioeb",	0720,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "tionb",	0721,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "rdiob",	0722,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "wriob",	0723,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "bsiob",	0724,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
  { "bciob",	0725,	PDP10_BASIC,	PDP10_KS10, 0, 0 },
#endif

  /*
   * XKL-1 system instructions.
   */

  { "rdadb",	070004,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "sysid",	070010,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wradb",	070014,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrapr",	070020,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "szapr",	070030,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "snapr",	070034,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wctrlf",	070040,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rctrlf",	070044,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "simird",	070050,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrkpa",	070054,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "szpi",	070070,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "snpi",	070074,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "apr0",	0700,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "clrpt",	070110,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrerr",	070120,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rderr",	070124,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrctx",	070130,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rdctx",	070134,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rddcsh",	070140,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "dwrcsh",	070160,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "swpio",	070164,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "swpvo",	070170,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "swpuo",	070174,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "apr1",	0701,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "rditm",	070220,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rdtime",	070224,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "drdptb",	070230,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrtime",	070234,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "writm",	070260,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "dwrptb",	070270,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "apr2",	0702,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "rdcty",	070304,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrcty",	070314,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "wrctys",	070320,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "rdctys",	070324,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "szcty",	070330,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "sncty",	070334,	PDP10_A_OPCODE,	PDP10_XKL1, 0, 0 },
  { "apr3",	0703,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "pmove",	0704,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "pmovem",	0705,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "nmove",	0706,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "nmovem",	0707,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "ldlpn",	0710,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "rdcfg",	0711,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "amove",	0714,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "amovem",	0715,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "umove",	0716,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },
  { "umovem",	0717,	PDP10_BASIC,	PDP10_XKL1, 0, 0 },

  /*
   * 166 / KA10 / KI10 / KL10 I/O instructions.
   *
   * These should come after the more specific instructions above.
   */

#define PDP10_not_KS10_or_XKL1 (PDP10_ALL & ~(PDP10_KS10 | \
				PDP10_XKL1))

  { "blki",	070000,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "datai",	070004,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "blko",	070010,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "datao",	070014,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "cono",	070020,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "coni",	070024,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "consz",	070030,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
  { "conso",	070034,	PDP10_IO,	PDP10_not_KS10_or_XKL1, 0, 0 },
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
  { "nop",	0255,	PDP10_BASIC,	PDP10_ALL, 0, HINT_NUMBER },
  { "pxct",	0256,	PDP10_BASIC,	PDP10_ALL, 0, 0 }, /* A_SPECIAL_CODE */
  { "setmi",	0415,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "or",	0434,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "ori",	0435,	PDP10_BASIC,	PDP10_ALL, 0, HINT_IMMEDIATE },
  { "orm",	0436,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "orb",	0437,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
  { "hlli",	0501,	PDP10_BASIC,	PDP10_ALL, 0, 0 },
};

const struct pdp10_instruction pdp10_extended_instruction[] =
{
  /* name,	opcode,	type,			models */
  { "cmpsl",	0001,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cmpse",	0002,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cmpsle",	0003,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "edit",	0004,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cmpsge",	0005,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cmpsn",	0006,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cmpsg",	0007,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "cvtdbo",	0010,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "cvtdbt",	0011,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "cvtbdo",	0012,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "cvtbdt",	0013,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "movso",	0014,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "movst",	0015,	PDP10_A_UNUSED,		PDP10_KL10up, 0, 0 },
  { "movslj",	0016,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "movsrj",	0017,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "xblt",	0020,	PDP10_A_E_UNUSED,	PDP10_KL10up, 0, 0 },
  { "gsngl",	0021,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gdble",	0022,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gdfix",	0023,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gdfixr",	0025,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gfix",	0024,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gfixr",	0026,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "dgfltr",	0027,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gfltr",	0030,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
  { "gfsc",	0031,	PDP10_A_UNUSED,		PDP10_KL10_271, 0, 0 },
};

const int pdp10_num_instructions = sizeof pdp10_instruction /
                                   sizeof pdp10_instruction[0];
const int pdp10_num_devices = sizeof pdp10_device / sizeof pdp10_device[0];
const int pdp10_num_aliases = sizeof pdp10_alias / sizeof pdp10_alias[0];
