/* Copyright (C) 2013, 2021 Lars Brinkhoff <lars@nocrew.org>

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

#define _GNU_SOURCE /* for strcasestr */

#include <stdio.h>
#include <string.h>

#include "opcode/pdp10.h"
#include "dis.h"
#include "memory.h"
#include "symbols.h"
#include "timing.h"

#define OPCODE(word)	(int)(((word) >> 27) &    0777)
#define A(word)		(int)(((word) >> 23) &     017)
#define OPCODE_A(word)	(((OPCODE (word) << 4) | A (word)) << 2)
#define DEVICE(word)	((OPCODE_A (word) & 07740) >> 3)
#define I(word)		(int)(((word) >> 22) &       1)
#define X(word)		(int)(((word) >> 18) &     017)
#define Y(word)		(int)( (word)        & 0777777)
#define E(word)		((I (word) << 22) | (X (word) << 18) | Y (word))

#define POPJ_17 ((word_t)(0263740000000LL))
#define SETZ	((word_t)(0400000000000LL))
#define ITS_OPER 042
#define ITS_CALL 04300
#define CALLI    047
#define JSYS     0104

struct its_oper
{
  const char *name;
  int opcode;
  int hint;
};

static struct its_oper opers[] =
{
  { ".ityi",	0000001, HINT_ACCUMULATOR },
  { ".listen",	0000002, HINT_ACCUMULATOR },
  { ".sleep",	0000003, HINT_ACCUMULATOR },
  { ".setmsk",	0000004, HINT_ACCUMULATOR },
  { ".setm2",	0000005, HINT_ACCUMULATOR },
#if 1
  { ".demon",	0000006, HINT_ACCUMULATOR },
#else
  { ".login",	0000006, HINT_ACCUMULATOR },
#endif
  { ".close",	0000007, HINT_CHANNEL },
  { ".uclose",	0000010, HINT_CHANNEL },
  { ".atty",	0000011, HINT_CHANNEL },
  { ".dtty",	0000012, HINT_ACCUMULATOR },
  { ".iopush",	0000013, HINT_ACCUMULATOR },
  { ".iopop",	0000014, HINT_CHANNEL },
  { ".dclose",	0000015, HINT_ACCUMULATOR },
  { ".dstop",	0000016, HINT_ACCUMULATOR },
  { ".rdtime",	0000017, HINT_ACCUMULATOR },
  { ".rdsw",	0000020, HINT_ACCUMULATOR },
  { ".gun",	0000021, HINT_ACCUMULATOR },
  { ".udismt",	0000022, HINT_ACCUMULATOR },
  { ".getsys",	0000023, HINT_ACCUMULATOR },
#if 1
  { ".ipdp",	0000024, HINT_CHANNEL },
#else
  { ".rd500",	0000024, HINT_ACCUMULATOR },
#endif
  { ".getloc",	0000025, HINT_ACCUMULATOR },
  { ".setloc",	0000026, HINT_ACCUMULATOR },
  { ".disown",	0000027, HINT_CHANNEL },
#if 1
  { ".dword",	0000030, HINT_ACCUMULATOR },
  { ".dstep",	0000031, HINT_ACCUMULATOR },
#else
  { ".rd760",	0000030, HINT_ACCUMULATOR }, /* Also .mspace. */
  { ".wr760",	0000031, HINT_ACCUMULATOR },
#endif
  { ".gensym",	0000032, HINT_ACCUMULATOR },
  { ".logout",	0000033, HINT_NUMBER },
#if 1
  { ".realt",	0000034, HINT_ACCUMULATOR },
#else
  { ".gsnam",	0000034, HINT_ACCUMULATOR },
#endif
  { ".wsname",	0000035, HINT_ACCUMULATOR },
  { ".upiset",	0000036, HINT_ACCUMULATOR },
  { ".reset",	0000037, HINT_CHANNEL },
  { ".armove",	0000040, HINT_ACCUMULATOR },
#if 1
  { ".dcontin",	0000041, HINT_ACCUMULATOR },
  { ".cblk",	0000042, HINT_ACCUMULATOR },
#else
  { ".wmar",	0000041, HINT_ACCUMULATOR },
  { ".rrtim",	0000042, HINT_ACCUMULATOR },
#endif
  { ".assign",	0000043, HINT_ACCUMULATOR },
  { ".design",	0000044, HINT_ACCUMULATOR },
  { ".rtime",	0000045, HINT_ACCUMULATOR },
  { ".rdate",	0000046, HINT_ACCUMULATOR },
#if 1
  { ".hang",	0000047, HINT_ACCUMULATOR },
#else
  { ".rd710",	0000047, HINT_ACCUMULATOR },
#endif
  { ".eofc",	0000050, HINT_ACCUMULATOR },
  { ".iotlsr",	0000051, HINT_ACCUMULATOR },
  { ".rsysi",	0000052, HINT_ACCUMULATOR },
  { ".supset",	0000053, HINT_ACCUMULATOR },
#if 1
  { ".pdtime",	0000054, HINT_ACCUMULATOR },
  { ".armrs",	0000055, HINT_ACCUMULATOR },
#else
  { ".orgi",	0000054, HINT_ACCUMULATOR },
  { ".orgo",	0000055, HINT_ACCUMULATOR },
#endif
  { ".ublat",	0000056, HINT_ACCUMULATOR },
  { ".iopdl",	0000057, HINT_ACCUMULATOR },
  { ".ityic",	0000060, HINT_ACCUMULATOR },
  { ".master",	0000061, HINT_ACCUMULATOR },
  { ".vstst",	0000062, HINT_ACCUMULATOR },
#if 1
  { ".netac",	0000063, HINT_CHANNEL },
  { ".nets",	0000064, HINT_CHANNEL },
  { ".revive",	0000065, HINT_ACCUMULATOR },
#else
  { ".dial",	0000063, HINT_ACCUMULATOR },
  { ".dialw",	0000064, HINT_NUMBER },
  { ".hangu",	0000065, HINT_NUMBER },
#endif
  { ".dietim",	0000066, HINT_ACCUMULATOR },
  { ".shutdn",	0000067, HINT_ACCUMULATOR },
  { ".armoff",	0000070, HINT_ACCUMULATOR },
  { ".ndis",	0000071, HINT_ACCUMULATOR },
  { ".feed",	0000072, HINT_CHANNEL },
  { ".eval",	0000073, HINT_ACCUMULATOR },
  { ".redef",	0000074, HINT_ACCUMULATOR },
  { ".ifset",	0000075, HINT_ACCUMULATOR },
  { ".utnam",	0000076, HINT_ACCUMULATOR },
  { ".uinit",	0000077, HINT_ACCUMULATOR },
  { ".ryear",	0000100, HINT_ACCUMULATOR },
  { ".rlpdtm",	0000101, HINT_ACCUMULATOR },
  { ".rdatim",	0000102, HINT_ACCUMULATOR },
  { ".rchst",	0000103, HINT_ACCUMULATOR },
  { ".rbtc",	0000104, HINT_ACCUMULATOR },
  { ".dmpch",	0000105, HINT_ACCUMULATOR },
  { ".swap",	0000106, HINT_ACCUMULATOR },
  { ".mtape",	0000107, HINT_ACCUMULATOR },
  { ".gennum",	0000110, HINT_ACCUMULATOR },
  { ".netint",	0000111, HINT_CHANNEL },
};

static struct its_oper waits_callis[] =
{
  { "reset",	0000000, 0 },
  { "ddtin",	0000001, 0 },
  { "setddt",	0000002, 0 },
  { "ddtout",	0000003, 0 },
  { "devchr",	0000004, 0 },
//{ "ddtgt",	0000005, 0 }, /* Illegail in WAITS. */
  { "getchr",	0000006, 0 },
//{ "ddtrl",	0000007, 0 }, /* Illegail in WAITS. */
  { "wait",	0000010, 0 },
  { "core",	0000011, 0 },
  { "exit",	0000012, 0 },
  { "utpclr",	0000013, 0 },
  { "date",	0000014, 0 },
  { "login",	0000015, 0 },
  { "aprenb",	0000016, 0 },
  { "logout",	0000017, 0 },
  { "switch",	0000020, 0 },
  { "reassi",	0000021, 0 },
  { "timer",	0000022, 0 },
  { "mstime",	0000023, 0 },
  { "getppn",	0000024, 0 },
//{ "trpset",	0000025, 0 }, /* Illegail in WAITS. */
//{ "trpjen",	0000026, 0 }, /* Illegail in WAITS. */
  { "runtim",	0000027, 0 },
  { "pjob",	0000030, 0 },
  { "sleep",	0000031, 0 },
  { "setpov",	0000032, 0 },
  { "peek",	0000033, 0 },
  { "getln",	0000034, 0 },
  { "run",	0000035, 0 },
  { "setuwp",	0000036, 0 },
  { "remap",	0000037, 0 },
//{ "getseg",	0000040, 0 }, /* Illegail in WAITS. */
//{ "gettab",	0000041, 0 }, /* Illegail in WAITS. */
//{ "spy",	0000042, 0 }, /* Illegail in WAITS. */
  { "setnam",	0000043, 0 },
  { "tmpcor",	0000044, 0 },

  { "spwbut",	0400000, 0 },
  { "ctlv",	0400001, 0 },
  { "setnam",	0400002, 0 },
  { "spcwgo",	0400003, 0 },
  { "swap",	0400004, 0 },
  { "eiotm",	0400005, 0 },
  { "liotm",	0400006, 0 },
  { "pname",	0400007, 0 },
  { ".syml",	0400010, 0 },
  { "showit",	0400011, 0 },
  { "freeze",	0400012, 0 },
  { "jbtsts",	0400013, 0 },
  { "ttyios",	0400014, 0 },
  { "core2",	0400015, 0 },
  { "attseg",	0400016, 0 },
  { "detseg",	0400017, 0 },
  { "setpro",	0400020, 0 },
  { "segnum",	0400021, 0 },
  { "segsiz",	0400022, 0 },
  { "linkup",	0400023, 0 },
  { "dismis",	0400024, 0 },
  { "inteng",	0400025, 0 },
  { "intorm",	0400026, 0 },
  { "intacm",	0400027, 0 },
  { "intens",	0400030, 0 },
  { "intiip",	0400031, 0 },
  { "intirq",	0400032, 0 },
  { "intgen",	0400033, 0 },
  { "uwait",	0400034, 0 },
  { "debrea",	0400035, 0 },
  { "setnm2",	0400036, 0 },
  { "segnam",	0400037, 0 },
  { "iwait",	0400040, 0 },
  { "uskip",	0400041, 0 },
  { "buflen",	0400042, 0 },
  { "namein",	0400043, 0 },
  { "slevel",	0400044, 0 },
  { "ienbw",	0400045, 0 },
//{ "runmsk",	0400046, 0 }, /* Illegail in WAITS. */
  { "ttymes",	0400047, 0 },
  { "jobrd",	0400050, 0 },
  { "devuse",	0400051, 0 },
  { "setpr2",	0400052, 0 },
  { "getpr2",	0400053, 0 },
  { "rlevel",	0400054, 0 },
  { "ufbphy",	0400055, 0 },
  { "ufbskp",	0400056, 0 },
  { "fbwait",	0400057, 0 },
  { "ufberr",	0400060, 0 },
  { "wakeme",	0400061, 0 },
  { "getnam",	0400062, 0 },
  { "sneakw",	0400063, 0 },
  { "sneak",	0400064, 0 },
//{ "gdptim",	0400065, 0 }, /* Illegail in WAITS. */
  { "setprv",	0400066, 0 },
  { "ddchan",	0400067, 0 },
  { "vdsmap",	0400070, 0 },
  { "dskppn",	0400071, 0 },
//{ "dsktim",	0400072, 0 }, /* In early Stanford monitor. */
  { "gethi",	0400072, 0 },
  { "setcrd",	0400073, 0 },
  { "callit",	0400074, 0 },
  { "xgpuuo",	0400075, 0 },
  { "lock",	0400076, 0 },
  { "unlock",	0400077, 0 },
  { "dayvnt",	0400100, 0 },
  { "acctim",	0400101, 0 },
  { "unpure",	0400102, 0 },
  { "tmpcrd",	0400103, 0 },
  { "devnum",	0400104, 0 },
  { "actchr",	0400105, 0 },
  { "uuosim",	0400106, 0 },
  { "ppspy",	0400107, 0 },
  { "adsmap",	0400110, 0 },
  { "beep",	0400111, 0 },
  { "who",	0400112, 0 },
  { "ttyjob",	0400113, 0 },
  { "nulmes",	0400114, 0 },
  { "getprv",	0400115, 0 },
  { "ttyskp",	0400116, 0 },
  { "dial",	0400117, 0 },
//{ "lthuuo",	0400120, 0 }, /* Illegail in WAITS. */
  { "ttyset",	0400121, 0 },
  { "mtruuo",	0400122, 0 },
  { "rdline",	0400123, 0 },
};

static struct its_oper jsyses[] =
{
  { "reset",	0000000, 0 },
  { "login",    01, 0 },
  { "crjob",    02, 0 },
  { "lgout",    03, 0 },
  { "cacct",    04, 0 },
  { "efact",    05, 0 },
  { "smon",    06, 0 },
  { "tmon",    07, 0 },
  { "getab",    010, 0 },
  { "erstr",    011, 0 },
  { "geter",    012, 0 },
  { "gjinf",    013, 0 },
  { "time",    014, 0 },
  { "runtm",    015, 0 },
  { "sysgt",    016, 0 },
  { "gnjfn",    017, 0 },
  { "gtjfn",    020, 0 },
  { "openf",    021, 0 },
  { "closf",    022, 0 },
  { "rljfn",    023, 0 },
  { "gtsts",    024, 0 },
  { "ststs",    025, 0 },
  { "delf",    026, 0 },
  { "sfptr",    027, 0 },
  { "jfns",    030, 0 },
  { "ffffp",    031, 0 },
  { "rddir",    032, 0 },
  { "cprtf",    033, 0 },
  { "clzff",    034, 0 },
  { "rnamf",    035, 0 },
  { "sizef",    036, 0 },
  { "gactf",    037, 0 },
  { "stdir",    040, 0 },
  { "dirst",    041, 0 },
  { "bkjfn",    042, 0 },
  { "rfptr",    043, 0 },
  { "cndir",    044, 0 },
  { "rfbsz",    045, 0 },
  { "sfbsz",    046, 0 },
  { "swjfn",    047, 0 },
  { "bin",    050, 0 },
  { "bout",    051, 0 },
  { "sin",    052, 0 },
  { "sout",    053, 0 },
  { "rin",    054, 0 },
  { "rout",    055, 0 },
  { "pmap",    056, 0 },
  { "rpacs",    057, 0 },
  { "spacs",    060, 0 },
  { "rmap",    061, 0 },
  { "sactf",    062, 0 },
  { "gtfdb",    063, 0 },
  { "chfdb",    064, 0 },
  { "dumpi",    065, 0 },
  { "dumpo",    066, 0 },
  { "deldf",    067, 0 },
  { "asnd",    070, 0 },
  { "reld",    071, 0 },
  { "csyno",    072, 0 },
  { "pbin",    073, 0 },
  { "pbout",    074, 0 },
  { "psin",    075, 0 },
  { "psout",    076, 0 },
  { "mtopr",    077, 0 },
  { "cfibf",    0100, 0 },
  { "cfobf",    0101, 0 },
  { "sibe",    0102, 0 },
  { "sobe",    0103, 0 },
  { "dobe",    0104, 0 },
  { "gtabs",    0105, 0 },
  { "stabs",    0106, 0 },
  { "rfmod",    0107, 0 },
  { "sfmod",    0110, 0 },
  { "rfpos",    0111, 0 },
  { "rfcoc",    0112, 0 },
  { "sfcoc",    0113, 0 },
  { "sti",    0114, 0 },
  { "dtach",    0115, 0 },
  { "atach",    0116, 0 },
  { "dvchr",    0117, 0 },
  { "stdev",    0120, 0 },
  { "devst",    0121, 0 },
  { "mount",    0122, 0 },
  { "dsmnt",    0123, 0 },
  { "inidr",    0124, 0 },
  { "sir",    0125, 0 },
  { "eir",    0126, 0 },
  { "skpir",    0127, 0 },
  { "dir",    0130, 0 },
  { "aic",    0131, 0 },
  { "iic",    0132, 0 },
  { "dic",    0133, 0 },
  { "rcm",    0134, 0 },
  { "rwm",    0135, 0 },
  { "debrk",    0136, 0 },
  { "ati",    0137, 0 },
  { "dti",    0140, 0 },
  { "cis",    0141, 0 },
  { "sircm",    0142, 0 },
  { "rircm",    0143, 0 },
  { "rir",    0144, 0 },
  { "gdsts",    0145, 0 },
  { "sdsts",    0146, 0 },
  { "reset",    0147, 0 },
  { "rpcap",    0150, 0 },
  { "epcap",    0151, 0 },
  { "cfork",    0152, 0 },
  { "kfork",    0153, 0 },
  { "ffork",    0154, 0 },
  { "rfork",    0155, 0 },
  { "rfsts",    0156, 0 },
  { "sfork",    0157, 0 },
  { "sfacs",    0160, 0 },
  { "rfacs",    0161, 0 },
  { "hfork",    0162, 0 },
  { "wfork",    0163, 0 },
  { "gfrkh",    0164, 0 },
  { "rfrkh",    0165, 0 },
  { "gfrks",    0166, 0 },
  { "disms",    0167, 0 },
  { "haltf",    0170, 0 },
  { "gtrpw",    0171, 0 },
  { "gtrpi",    0172, 0 },
  { "rtiw",    0173, 0 },
  { "stiw",    0174, 0 },
  { "sobf",    0175, 0 },
  { "rwset",    0176, 0 },
  { "getnm",    0177, 0 },
  { "get",    0200, 0 },
  { "sfrkv",    0201, 0 },
  { "save",    0202, 0 },
  { "ssave",    0203, 0 },
  { "sevec",    0204, 0 },
  { "gevec",    0205, 0 },
  { "gpjfn",    0206, 0 },
  { "spjfn",    0207, 0 },
  { "setnm",    0210, 0 },
  { "ffufp",    0211, 0 },
  { "dibe",    0212, 0 },
  { "fdfre",    0213, 0 },
  { "gdskc",    0214, 0 },
  { "lites",    0215, 0 },
  { "tlink",    0216, 0 },
  { "stpar",    0217, 0 },
  { "odtim",    0220, 0 },
  { "idtim",    0221, 0 },
  { "odcnv",    0222, 0 },
  { "idcnv",    0223, 0 },
  { "nout",    0224, 0 },
  { "nin",    0225, 0 },
  { "stad",    0226, 0 },
  { "gtad",    0227, 0 },
  { "odtnc",    0230, 0 },
  { "idtnc",    0231, 0 },
  { "flin",    0232, 0 },
  { "flout",    0233, 0 },
  { "dfin",    0234, 0 },
  { "dfout",    0235, 0 },
  { "crdir",    0240, 0 },
  { "gtdir",    0241, 0 },
  { "dskop",    0242, 0 },
  { "spriw",    0243, 0 },
  { "dskas",    0244, 0 },
  { "sjpri",    0245, 0 },
  { "asndp",    0260, 0 },
  { "reldp",    0261, 0 },
  { "asndc",    0262, 0 },
  { "reldc",    0263, 0 },
  { "strdp",    0264, 0 },
  { "stpdp",    0265, 0 },
  { "stsdp",    0266, 0 },
  { "rdsdp",    0267, 0 },
  { "watdp",    0270, 0 },
  { "atpty",    0274, 0 },
  { "cvskt",    0275, 0 },
  { "cvhst",    0276, 0 },
  { "flhst",    0277, 0 },
  { "gcvec",    0300, 0 },
  { "scvec",    0301, 0 },
  { "sttyp",    0302, 0 },
  { "gttyp",    0303, 0 },
  { "bpt",    0304, 0 },
  { "gtdal",    0305, 0 },
  { "wait",    0306, 0 },
  { "hsys",    0307, 0 },
  { "usrio",    0310, 0 },
  { "peek",    0311, 0 },
  { "msfrk",    0312, 0 },
  { "esout",    0313, 0 },
  { "splfk",    0314, 0 },
  { "adviz",    0315, 0 },
  { "jobtm",    0316, 0 },
  { "delnf",    0317, 0 },
  { "swtch",    0320, 0 },
  { "oprfn",    0326, 0 },

  { "sndim",    0750, 0 },
  { "rcvim",    0751, 0 },
  { "asnsq",    0752, 0 },
  { "relsq",    0753, 0 },
  { "dbgim",    0766, 0 },
  { "mrpac",    0772, 0 },
  { "ttmsg",    0775, 0 },
  { "exec",     0777, 0 },

  /* AMES */
  { "aspty",    0360, 0 },
  { "repty",    0361, 0 },
  { "psti",    0362, 0 },
  { "psto",    0363, 0 },
  { "sibf",    0364, 0 },
  { "sruba",    0365, 0 },

  /* SUMEX */
  { "sfpos",    0526, 0 },
  { "vkeep",    0674, 0 },
  { "prge",    0675, 0 },

  /* IMSSS */
  { "pbtin",    0600, 0 },
  { "ttcvt",    0601, 0 },
  { "kidno",    0602, 0 },
  { "logsv",    0603, 0 },
  { "datsv",    0604, 0 },
  { "scedr",    0605, 0 },
  { "sceds",    0606, 0 },
  { "syslk",    0610, 0 },
  { "klgot",    0613, 0 },
  { "klgin",    0623, 0 },

  /* SUMEX and IMSSS */
  { "cntsz",    0607, 0 },
  { "pstin",    0611, 0 },
  { "rand",     0612, 0 },
  { "ptinf",    0614, 0 },
  { "gtinf",    0615, 0 },
  { "delch",    0625, 0 },
  { "sjpct",    0626, 0 },
  { "rjpct",    0627, 0 },
  { "iit",      0630, 0 },
  { "stcha",    0633, 0 },
  { "gtblt",    0634, 0 },

  /* Tymnet */
  { "tymbw",    0651, 0 },
  { "tymbr",    0652, 0 },
  { "tymli",    0653, 0 },
  { "tymop",    0654, 0 },
};

static int
spaces (int n)
{
  int i;

  for (i = 0; i < n; i++)
    fputc (' ', output_file);

  return i < 0 ? 0 : i;
}

static const struct pdp10_instruction *
lookup (word_t word, int cpu_model)
{
  int i, opcode;

  for (i = 0; i < pdp10_num_instructions; i++)
    {
      if (pdp10_instruction[i].type & PDP10_A_OPCODE)
	opcode = OPCODE_A (word);
      else if (pdp10_instruction[i].type & PDP10_A_XCTRI)
	opcode = OPCODE_A (word) & 077720; /* Just keep the XCTRI bit. */
      else if (pdp10_instruction[i].type & PDP10_IO)
	opcode = OPCODE_A (word) & 070034;
      else
	opcode = OPCODE (word);
      if (opcode == pdp10_instruction[i].opcode &&
	  (pdp10_instruction[i].model & cpu_model))
	return &pdp10_instruction[i];
    }

  return NULL;
}

static const struct pdp10_device *
lookup_device (int number, int cpu_model)
{
  int i;

  for (i = 0; i < pdp10_num_devices; i++)
    {
      if (number == pdp10_device[i].number &&
	  (pdp10_instruction[i].model & cpu_model))
	return &pdp10_device[i];
    }

  return NULL;
}

static const struct its_oper *
lookup_oper (word_t word, struct its_oper *table, int size)
{
  int i, e;

  e = E (word);

  for (i = 0; i < size; i++)
    {
      if (e == table[i].opcode)
	return &table[i];
    }

  return NULL;
}

int
calc_e (struct pdp10_memory *memory, word_t word)
{
  int e;

  for (;;)
    {
      if (X (word) != 0)
	return -1;

      e = Y (word);
      if (!I (word))
	break;

      word = get_word_at (memory, e);
    }
  
  return e;
}

void
dis (struct pdp10_memory *memory, int cpu_model)
{
  word_t mask = 0777777777777LL;
  word_t word;

  set_address (memory, -1);
  while ((word = get_next_word (memory)) != -1)
    {
      if (word & START_TAPE)
	fprintf (output_file, "Logical end of tape.\n");
      else if (word & START_FILE)
	fprintf (output_file, "Start of file.\n");
      else if (word & START_RECORD)
	fprintf (output_file, "Start of record.\n");
      disassemble_word (memory, word & mask, get_address (memory), cpu_model);
    }
}

int
print_val (const char *format, int field, int hint)
{
  const char *p;
  const struct symbol *sym;
  int n = 0;

  for (p = format; *p; p++)
    {
      if (*p == '%')
	{
	  p++; /* assume %o */

	  sym = get_symbol_by_value (field, hint);
	  if (sym == NULL)
	    n += fprintf (output_file, "%o", field);
	  else
	    n += fprintf (output_file, "%s", sym->name);
	}
      else
	{
	  n += fprintf (output_file, "%c", *p);
	}
    }

  return n;
}

static double immediate_float (word_t x)
{
  int sign = x >> 17;
  int exponent, fraction;
  double exp = 1.0, y = 2.0;
  int i;

  if (sign)
    {
      x = -(x << 18);
      x = (x >> 18) & 0777777LL;
    }

  exponent = (x >> 9) & 0377;
  fraction = x & 0777;

  exponent -= 0211;
  if (exponent < 0)
    {
      exponent = -exponent;
      y = 0.5;
    }

  for (i = 0; i < exponent; i++)
    exp *= y;

  return (sign ? -1.0 : 1.0) * (double)fraction * exp;
}

void
disassemble_word (struct pdp10_memory *memory, word_t word,
		  int address, int cpu_model)
{
  const struct symbol *sym;
  const struct pdp10_instruction *op;
  int printable, hint;
  char ch[5];
  int i, n;

  sym = get_symbol_by_value (address, HINT_ADDRESS);
  if (sym != NULL)
    fprintf (output_file, "%s:\n", sym->name);

  if (address == -1)
    fprintf (output_file, "         ");
  else
    fprintf (output_file, "%06o:  ", address);

  fprintf (output_file, "%012llo  ", word);

  n = 0;

  op = lookup (word, cpu_model);
  if (op == NULL)
    {
      if ((word >> 18) == 0 && Y (word) != 0)
	{
	  /* If no opcode found and left half is 0, print as symbol. */
	  sym = get_symbol_by_value (Y (word), HINT_ADDRESS);
	  if (sym != NULL)
	    n += fprintf (output_file, "%s", sym->name);
	}
      else if ((word >> 18) != 0 && Y (word))
	{
	  /* No opcode found and right half is 0, print as symbol. */
	  sym = get_symbol_by_value (word >> 18, HINT_ADDRESS);
	  if (sym != NULL)
	    n += fprintf (output_file, "%s,,", sym->name);
	}
    }
  else if ((cpu_model & PDP10_ITS) && OPCODE (word) == ITS_OPER)
    {
      const struct its_oper *oper;
      oper = lookup_oper (word, opers, sizeof opers / sizeof opers[0]);
      if (oper)
	{
	  n += fprintf (output_file, "%-8s ", oper->name);
	  if (oper->hint == HINT_CHANNEL || A (word) != 0)
	    n += print_val ("%o,", A (word), oper->hint);
	}
    }
  else if ((cpu_model & PDP10_SAIL) && OPCODE (word) == CALLI)
    {
      const struct its_oper *calli;
      calli = lookup_oper (word, waits_callis,
			   sizeof waits_callis / sizeof waits_callis[0]);
      if (calli)
	{
	  n += fprintf (output_file, "%-8s ", calli->name);
	  if (calli->hint == HINT_CHANNEL || A (word) != 0)
	    n += print_val ("%o,", A (word), calli->hint);
	}
    }
  else if ((cpu_model & PDP10_T20) && OPCODE (word) == JSYS)
    {
      const struct its_oper *oper;
      oper = lookup_oper (word, jsyses, sizeof jsyses / sizeof jsyses[0]);
      if (oper)
	{
	  n += fprintf (output_file, "%-8s ", oper->name);
	  if (oper->hint == HINT_CHANNEL || A (word) != 0)
	    n += print_val ("%o,", A (word), oper->hint);
	}
    }
#if 1
  else if ((cpu_model & PDP10_ITS) && OPCODE_A (word) == ITS_CALL)
    {
      int address;
      
      n += fprintf (output_file, "%-8s ", ".call");

      address = calc_e (memory, word);
      if (address == -1 ||
	  get_word_at (memory, address) != SETZ)
	{
	  if (I (word))
	    n += fprintf (output_file, "@");
	  n += print_val ("%o", Y (word), HINT_ADDRESS);
	  if (X (word))
	    n += print_val ("(%o)", X (word), HINT_ACCUMULATOR);
	}
      else
	{
	  char name[7];
	  int i;
	  word_t w;

	  n = 0;
	  fprintf (output_file, "[setz\n");
	  w = get_word_at (memory, address + 1);
	  sixbit_to_ascii (w, name);
	  fprintf (output_file, "                                 SIXBIT/%s/\n", name);
	  i = 2;
	  while (((w = get_word_at (memory, address + i)) & SIGNBIT) == 0)
	    {
	      fprintf (output_file, "                                 %012llo\n", w);
	      i++;
	    }
	  fprintf (output_file, "                                 %012llo]\n", w);
	  fprintf (output_file, "                       ");
	}
    }
#endif
  else
    {
      n += fprintf (output_file, "%-8s ", op->name);

      if (op->type & PDP10_IO)
	{
	  const struct pdp10_device *dev;
	  dev = lookup_device (DEVICE (word), cpu_model);
	  if (dev != NULL)
	    n += fprintf (output_file, "%s, ", dev->name);
	  else
	    n += print_val ("%o, ", DEVICE (word), HINT_DEVICE);
	}
      else if (op->type & PDP10_A_XCTRI)
	{
	  hint = op->ac_hint ? op->ac_hint : HINT_ACCUMULATOR;
	  /* Mask off XCTRI bit in AC field. */
	  n += print_val ("%o, ", A (word) & ~4, hint);
	}
      else if (!(op->type & PDP10_A_OPCODE))
	{
	  if (A (word) != 0 || !(op->type & PDP10_A_UNUSED))
	    {
	      hint = op->ac_hint ? op->ac_hint : HINT_ACCUMULATOR;
	      n += print_val ("%o, ", A (word), hint);
	    }
	}

      if (E (word) != 0 || !(op->type & PDP10_E_UNUSED))
	{
	  if (I (word))
	    n += fprintf (output_file, "@");

	  if (op->addr_hint == HINT_FLOAT && X (word) == 0)
	    {
	      const struct symbol *sym = get_symbol_by_value (Y (word), hint);
	      if (sym == NULL)
		n += fprintf (output_file, "(%f)", immediate_float (Y (word)));
	      else
		n += fprintf (output_file, "%s", sym->name);
	    }
	  else if (Y (word) != 0 && X (word) != 0)
	    n += print_val ("%o", Y (word), HINT_OFFSET);
	  else if (op->addr_hint != 0 && X (word) == 0)
	    n += print_val ("%o", Y (word), op->addr_hint);
	  else if (Y (word) != 0 ||
		   (I (word) != 0 && X (word) == 0) ||
		   (!(op->type & PDP10_E_UNUSED) && X (word) == 0))
	    n += print_val ("%o", Y (word), HINT_ADDRESS);

	  if (X (word))
	    n += print_val ("(%o)", X (word), HINT_ACCUMULATOR);
	}
    }

  spaces (25 - n);

#if 0
  /* Print instruction execution time. */
  {
    int t = instruction_time (word, PDP10_KI10);
    if (t > 0)
      fprintf (output_file, ";%5dns", t);
    else
      spaces (8);
  }
#endif

#if 1
  /* Print word as six SIXBIT characters. */
  fprintf (output_file, ";\"");
  for (i = 0; i < 6; i++)
    {
      int c = (int)(word >> (6 * (5 - i))) & 077;
      fputc (c + ' ', output_file);
    }
  fputc ('"', output_file);
#endif

#if 1
  /* Print word as five ASCII characters, if it seems likely to be a
   * printable string. */
  printable = 1;
  for (i = 0; i < 5; i++)
    {
      ch[i] = (word >> (((4 - i) * 7) + 1)) & 0177;
      switch (ch[i])
	{
	case '\0':
	case '\t':
	case '\n':
	case '\f':
	case '\r':
	case '\033':
	  break;
	default:
	  if (ch[i] < 040 || ch[i] > 0176)
	    printable = 0;
	  break;
	}
    }

  if ((word & 1) == 1 ||
      word == 0 ||
      word == POPJ_17)
    printable = 0;

  {
    int seen_nonzero = 0;
    for (i = 4; i >= 0; i--)
      {
	if (ch[i] == 0)
	  {
	    if (seen_nonzero)
	      printable = 0;
	  }
	else
	  seen_nonzero = 1;
      }
  }

  if (printable)
    {
      fprintf (output_file, " \"");
      for (i = 0; i < 5; i++)
	{
	  switch (ch[i])
	    {
	    case '\0':
	      fprintf (output_file, "\\0");
	      break;
	    case '\t':
	      fprintf (output_file, "\\t");
	      break;
	    case '\n':
	      fprintf (output_file, "\\n");
	      break;
	    case '\f':
	      fprintf (output_file, "\\f");
	      break;
	    case '\r':
	      fprintf (output_file, "\\r");
	      break;
	    case '\\':
	      fprintf (output_file, "\\\\");
	      break;
	    case '\"':
	      fprintf (output_file, "\\\"");
	      break;
	    default:
	      if (ch[i] < 040 || ch[i] > 0176)
		fprintf (output_file, "\\%03o", ch[i]);
	      else
		fputc (ch[i], output_file);
	      break;
	    }
	}
      fprintf (output_file, "\"");
    }
#endif

#if 0
  /* Print word as six SQUOZE characters. */
  squoze_to_ascii (word, ch);
  fprintf (output_file, " \"%s\"", ch);
#endif

  fprintf (output_file, "\n");
}

void
usage_machine (void)
{
  fprintf (stderr, "Valid machines are: 166, KA10, KI10, KL10, and KS10.\n");
  fprintf (stderr, "For KA/KL/KS, append ITS to get the MIT instructions.\n");
  fprintf (stderr, "Append T20 to get the JSYS instructions.\n");
}

int
parse_machine (const char *string, int *machine)
{
  /* Do some lax matching to find CPU and variants. */

  if (strcasestr (string, "166"))
    *machine = PDP6_166;
  else if (strcasestr (string, "pdp6"))
    *machine = PDP6_166;
  else if (strcasestr (string, "KA"))
    *machine = PDP10_KA10;
  else if (strcasestr (string, "KI"))
    *machine = PDP10_KI10;
  else if (strcasestr (string, "KL"))
    *machine = PDP10_KL10 | PDP10_KL10_271;
  else if (strcasestr (string, "KS"))
    *machine = PDP10_KS10;
  else
    return -1;

  if (strcasestr (string, "ITS"))
    {
      if (*machine & PDP10_KA10)
	*machine = PDP10_KA10_ITS;
      if (*machine & PDP10_KL10)
	*machine = PDP10_KL10_ITS;
      if (*machine & PDP10_KS10)
	*machine = PDP10_KS10_ITS;
    }

  if (strcasestr (string, "SAIL"))
    {
      if (*machine & PDP10_KA10)
	*machine = PDP10_KA10_SAIL;
      if (*machine & PDP10_KL10)
	*machine = PDP10_KL10_SAIL;
    }

  if (strcasestr (string, "T20"))
    {
      *machine |= PDP10_T20;
    }

  return 0;
}
