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

#include "opcode/pdp10.h"
#include "dis.h"
#include "timing.h"

#define OPCODE(word)	(int)(((word) >> 27) &    0777)
#define A(word)		(int)(((word) >> 23) &     017)
#define OPCODE_A(word)	(((OPCODE (word) << 4) | A (word)) << 2)
#define I(word)		(int)(((word) >> 22) &       1)
#define X(word)		(int)(((word) >> 18) &     017)
#define Y(word)		(int)( (word)        & 0777777)

#define max(a, b) ((a) > (b) ? (a) : (b))

static int
memory_read_access ()
{
  return 610;
}

#if 0
static int
memory_write_access ()
{
  return 200;
}
#endif

static int
wait_for_acknowledgement ()
{
  return 100; /* FIXME: this is a guess */
}

static int
instruction_execution (word_t instruction)
{
  int opcode = OPCODE (instruction);

  switch (opcode)
    {
    case 0001: /* LUUO 001 */	return 170 + 248;
    case 0002: /* LUUO 002 */	return 170 + 248;
    case 0003: /* LUUO 003 */	return 170 + 248;
    case 0004: /* LUUO 004 */	return 170 + 248;
    case 0005: /* LUUO 005 */	return 170 + 248;
    case 0006: /* LUUO 006 */	return 170 + 248;
    case 0007: /* LUUO 007 */	return 170 + 248;
    case 0010: /* LUUO 010 */	return 170 + 248;
    case 0011: /* LUUO 011 */	return 170 + 248;
    case 0012: /* LUUO 012 */	return 170 + 248;
    case 0013: /* LUUO 013 */	return 170 + 248;
    case 0014: /* LUUO 014 */	return 170 + 248;
    case 0015: /* LUUO 015 */	return 170 + 248;
    case 0016: /* LUUO 016 */	return 170 + 248;
    case 0017: /* LUUO 017 */	return 170 + 248;
    case 0020: /* LUUO 020 */	return 170 + 248;
    case 0021: /* LUUO 021 */	return 170 + 248;
    case 0022: /* LUUO 022 */	return 170 + 248;
    case 0023: /* LUUO 023 */	return 170 + 248;
    case 0024: /* LUUO 024 */	return 170 + 248;
    case 0025: /* LUUO 025 */	return 170 + 248;
    case 0026: /* LUUO 026 */	return 170 + 248;
    case 0027: /* LUUO 027 */	return 170 + 248;
    case 0030: /* LUUO 030 */	return 170 + 248;
    case 0031: /* LUUO 031 */	return 170 + 248;
    case 0032: /* LUUO 032 */	return 170 + 248;
    case 0033: /* LUUO 033 */	return 170 + 248;
    case 0034: /* LUUO 034 */	return 170 + 248;
    case 0035: /* LUUO 035 */	return 170 + 248;
    case 0036: /* LUUO 036 */	return 170 + 248;
    case 0037: /* LUUO 037 */	return 170 + 248;
    case 0040: /* MUUO 040 */	return 170 + 248;
    case 0041: /* MUUO 041 */	return 170 + 248;
    case 0042: /* MUUO 042 */	return 170 + 248;
    case 0043: /* MUUO 043 */	return 170 + 248;
    case 0044: /* MUUO 044 */	return 170 + 248;
    case 0045: /* MUUO 045 */	return 170 + 248;
    case 0046: /* MUUO 046 */	return 170 + 248;
    case 0047: /* MUUO 047 */	return 170 + 248;
    case 0050: /* MUUO 050 */	return 170 + 248;
    case 0051: /* MUUO 051 */	return 170 + 248;
    case 0052: /* MUUO 052 */	return 170 + 248;
    case 0053: /* MUUO 053 */	return 170 + 248;
    case 0054: /* MUUO 054 */	return 170 + 248;
    case 0055: /* MUUO 055 */	return 170 + 248;
    case 0056: /* MUUO 056 */	return 170 + 248;
    case 0057: /* MUUO 057 */	return 170 + 248;
    case 0060: /* MUUO 060 */	return 170 + 248;
    case 0061: /* MUUO 061 */	return 170 + 248;
    case 0062: /* MUUO 062 */	return 170 + 248;
    case 0063: /* MUUO 063 */	return 170 + 248;
    case 0064: /* MUUO 064 */	return 170 + 248;
    case 0065: /* MUUO 065 */	return 170 + 248;
    case 0066: /* MUUO 066 */	return 170 + 248;
    case 0067: /* MUUO 067 */	return 170 + 248;
    case 0070: /* MUUO 070 */	return 170 + 248;
    case 0071: /* MUUO 071 */	return 170 + 248;
    case 0072: /* MUUO 072 */	return 170 + 248;
    case 0073: /* MUUO 073 */	return 170 + 248;
    case 0074: /* MUUO 074 */	return 170 + 248;
    case 0075: /* MUUO 075 */	return 170 + 248;
    case 0076: /* MUUO 076 */	return 170 + 248;
    case 0077: /* MUUO 077 */	return 170 + 248;
    case 0122: /* FIX */	return 340 + 110 + 230 + 400 + 110 +
				       280 + 110;
    case 0124: /* DMOVEM */	return 220;
    case 0125: /* DMOVNM */	return 620;
    case 0126: /* FIXR */	return 340 + 110 + 230 + 400 + 110 +
				       280 + 110;
    case 0127: /* FLTR */	return 830 + 110 + 230 + 230;
    case 0130: /* UFA */	return 1070 + 60 + 110 + 230 + 170;
    case 0131: /* DFN */	return 560;
    case 0132: /* FSC */	return 560 + 170 + 230;
    case 0133: /* IBP */	return 510;
    case 0134: /* ILDB */	return 510 - 1000000; /* FIXME: go to C2 */
    case 0135: /* LDB */	return 270 - 1000000; /* FIXME: go to C2 */
    case 0136: /* IDPB */	return 510 - 1000000; /* FIXME: go to C2 */
    case 0137: /* DPB */	return 270 - 1000000; /* FIXME: go to C2 */
    case 0140: /* FAD */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0141: /* FADL */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230 + 120;
    case 0142: /* FADM */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0143: /* FADB */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0144: /* FADR */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0145: /* FADRI */	return 960 + 110 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0146: /* FADRM */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0147: /* FADRB */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0150: /* FSB */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0151: /* FSBL */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230 + 120;
    case 0152: /* FSBM */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0153: /* FSBB */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0154: /* FSBR */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0155: /* FSBRI */	return 960 + 60 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0156: /* FSBRM */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0157: /* FSBRB */	return 960 + 60 * 18 + 60 + 110 + 110 + 
				       230 + 110 + 230;
    case 0160: /* FMP */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0161: /* FMPL */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230 + 120;
    case 0162: /* FMPM */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0163: /* FMPB */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0164: /* FMPR */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0165: /* FMPRI */	return 2270 + 110 + 5 * 60 + 110 + 230 +
				       110 + 230;
    case 0166: /* FMPRM */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0167: /* FMPRB */	return 2270 + 14 * 60 + 110 + 230 +
				       110 + 230;
    case 0170: /* FDV */	return 5740 + 120 + 170 + 340 + 110 + 230;
    case 0171: /* FDVL */	return 6050 + 240 + 170 + 340 + 460;
    case 0172: /* FDVM */	return 5740 + 120 + 170 + 340 + 110 + 230;
    case 0173: /* FDVB */	return 5740 + 120 + 170 + 340 + 110 + 230;
    case 0174: /* FDVR */	return 6110 + 110 + 120 + 170 + 170 +
				       110 + 230;
    case 0175: /* FDVRI */	return 6110 + 110 + 120 + 170 + 170 +
				       110 + 230;
    case 0176: /* FDVRM */	return 6110 + 110 + 120 + 170 + 170 +
				       110 + 230;
    case 0177: /* FDVRB */	return 6110 + 110 + 120 + 170 + 170 +
				       110 + 230;
    case 0200: /* MOVE */	return 220;
    case 0201: /* MOVEI */	return 220;
    case 0202: /* MOVEM */	return 330;
    case 0203: /* MOVES */	return 220;
    case 0204: /* MOVS */	return 220;
    case 0205: /* MOVSI */	return 220;
    case 0206: /* MOVSM */	return 330;
    case 0207: /* MOVSS */	return 220;
    case 0210: /* MOVN */	return 280;
    case 0211: /* MOVNI */	return 280;
    case 0212: /* MOVNM */	return 390;
    case 0213: /* MOVNS */	return 280;
    case 0214: /* MOVM */	return 280;
    case 0215: /* MOVMI */	return 280;
    case 0216: /* MOVMM */	return 390;
    case 0217: /* MOVMS */	return 280;
    case 0220: /* IMUL */	return 3450 + 60 + 60;
    case 0221: /* IMULI */	return 2910 + 60;
    case 0222: /* IMULM */	return 3450 + 60 + 60;
    case 0223: /* IMULB */	return 3450 + 60 + 60;
    case 0224: /* MUL */	return 3450 + 60;
    case 0225: /* MULI */	return 2910;
    case 0226: /* MULM */	return 3450 + 60;
    case 0227: /* MULB */	return 3450 + 60;
    case 0230: /* IDIV */	return 60 + 120 + 6280 + 180 + 560 + 60 + 170;
    case 0231: /* IDIVI */	return 60 + 120 + 6280 + 180 + 560 + 60 + 170;
    case 0232: /* IDIVM */	return 60 + 120 + 6280 + 180 + 560 + 60 + 170;
    case 0233: /* IDIVB */	return 60 + 120 + 6280 + 180 + 560 + 60 + 170;
    case 0234: /* DIV */	return 6280 + 180 + 560 + 60 + 170;
    case 0235: /* DIVI */	return 6280 + 180 + 560 + 60 + 170;
    case 0236: /* DIVM */	return 6280 + 180 + 560 + 60 + 170;
    case 0237: /* DIVB */	return 6280 + 180 + 560 + 60 + 170;
    case 0240: /* ASH */	return 800 + 110 + 110 * 36;
    case 0241: /* ROT */	return 800 + 110 + 110 * 36;
    case 0242: /* LSH */	return 800 + 110 + 110 * 36;
    case 0243: /* JFFO */	return 560 + 110 * 9; /* FIXME: average */
    case 0244: /* ASHC */	return 800 + 110 + 110 * 72;
    case 0245: /* ROTC */	return 800 + 110 + 110 * 72;
    case 0246: /* LSHC */	return 800 + 110 + 110 * 72;
    case 0250: /* EXCH */	return 220;
    case 0251: /* BLT */	return 0; /* FIXME */
    case 0252: /* AOBJP */	return 340;
    case 0253: /* AOBJN */	return 340;
    case 0254: /* JRST */
      switch (A (instruction))
	{
	case 000: /* JRST */	return 110;
	case 001: /* PORTAL */	return 110;
	case 002: /* JRSTF */	return 220;
	default:		return -1000000;
	}
    case 0255: /* JFCL */	return 110;
    case 0256: /* XCT */	return 110;
    case 0257: /* MAP */	return 220;
    case 0260: /* PUSHJ */	return 170 + 248;
    case 0261: /* PUSH */	return 170 + 248;
    case 0262: /* POP */	return 418;
    case 0263: /* POPJ */	return 280;
    case 0264: /* JSR */	return 220;
    case 0265: /* JSP */	return 220;
    case 0266: /* JSA */	return 330;
    case 0267: /* JRA */	return 220;
    case 0270: /* ADD */	return 280;
    case 0271: /* ADDI */	return 280;
    case 0272: /* ADDM */	return 280;
    case 0273: /* ADDB */	return 280;
    case 0274: /* SUB */	return 280;
    case 0275: /* SUBI */	return 280;
    case 0276: /* SUBM */	return 280;
    case 0277: /* SUBB */	return 280;
    case 0300: /* CAI */	return 390;
    case 0301: /* CAIL */	return 390;
    case 0302: /* CAIE */	return 390;
    case 0303: /* CAILE */	return 390;
    case 0304: /* CAIA */	return 390;
    case 0305: /* CAIGE */	return 390;
    case 0306: /* CAIN */	return 390;
    case 0307: /* CAIG */	return 390;
    case 0310: /* CAM */	return 390;
    case 0311: /* CAML */	return 390;
    case 0312: /* CAME */	return 390;
    case 0313: /* CAMLE */	return 390;
    case 0314: /* CAMA */	return 390;
    case 0315: /* CAMGE */	return 390;
    case 0316: /* CAMN */	return 390;
    case 0317: /* CAMG */	return 390;
    case 0320: /* JUMP */	return 330;
    case 0321: /* JUMPL */	return 330;
    case 0322: /* JUMPE */	return 330;
    case 0323: /* JUMPLE */	return 330;
    case 0324: /* JUMPA */	return 330;
    case 0325: /* JUMPGE */	return 330;
    case 0326: /* JUMPN */	return 330;
    case 0327: /* JUMPG */	return 330;
    case 0330: /* SKIP */	return 330;
    case 0331: /* SKIPL */	return 330;
    case 0332: /* SKIPE */	return 330;
    case 0333: /* SKIPLE */	return 330;
    case 0334: /* SKIPA */	return 330;
    case 0335: /* SKIPGE */	return 330;
    case 0336: /* SKIPN */	return 330;
    case 0337: /* SKIPG */	return 330;
    case 0340: /* AOJ */	return 390;
    case 0341: /* AOJL */	return 390;
    case 0342: /* AOJE */	return 390;
    case 0343: /* AOJLE */	return 390;
    case 0344: /* AOJA */	return 390;
    case 0345: /* AOJGE */	return 390;
    case 0346: /* AOJN */	return 390;
    case 0347: /* AOJG */	return 390;
    case 0350: /* AOS */	return 390;
    case 0351: /* AOSL */	return 390;
    case 0352: /* AOSE */	return 390;
    case 0353: /* AOSLE */	return 390;
    case 0354: /* AOSA */	return 390;
    case 0355: /* AOSGE */	return 390;
    case 0356: /* AOSN */	return 390;
    case 0357: /* AOSG */	return 390;
    case 0360: /* SOJ */	return 390;
    case 0361: /* SOJL */	return 390;
    case 0362: /* SOJE */	return 390;
    case 0363: /* SOJLE */	return 390;
    case 0364: /* SOJA */	return 390;
    case 0365: /* SOJGE */	return 390;
    case 0366: /* SOJN */	return 390;
    case 0367: /* SOJG */	return 390;
    case 0370: /* SOS */	return 390;
    case 0371: /* SOSL */	return 390;
    case 0372: /* SOSE */	return 390;
    case 0373: /* SOSLE */	return 390;
    case 0374: /* SOSA */	return 390;
    case 0375: /* SOSGE */	return 390;
    case 0376: /* SOSN */	return 390;
    case 0377: /* SOSG */	return 390;
    case 0400: /* SETZ */	return 220;
    case 0401: /* SETZI */	return 220;
    case 0402: /* SETZM */	return 220;
    case 0403: /* SETZB */	return 220;
    case 0404: /* AND */	return 220;
    case 0405: /* ANDI */	return 220;
    case 0406: /* ANDM */	return 220;
    case 0407: /* ANDB */	return 220;
    case 0410: /* ANDCA */	return 220;
    case 0411: /* ANDCAI */	return 220;
    case 0412: /* ANDCAM */	return 220;
    case 0413: /* ANDCAB */	return 220;
    case 0414: /* SETM */	return 220;
    case 0415: /* SETMI */	return 220;
    case 0416: /* SETMM */	return 220;
    case 0417: /* SETMB */	return 220;
    case 0420: /* ANDCM */	return 220;
    case 0421: /* ANDCMI */	return 220;
    case 0422: /* ANDCMM */	return 220;
    case 0423: /* ANDCMB */	return 220;
    case 0424: /* SETA */	return 220;
    case 0425: /* SETAI */	return 220;
    case 0426: /* SETAM */	return 220;
    case 0427: /* SETAB */	return 220;
    case 0430: /* XOR */	return 220;
    case 0431: /* XORI */	return 220;
    case 0432: /* XORM */	return 220;
    case 0433: /* XORB */	return 220;
    case 0434: /* OR */		return 330;
    case 0435: /* ORI */	return 330;
    case 0436: /* ORM */	return 330;
    case 0437: /* ORB */	return 330;
    case 0440: /* ANDCB */	return 220;
    case 0441: /* ANDCBI */	return 220;
    case 0442: /* ANDCBM */	return 220;
    case 0443: /* ANDCBB */	return 220;
    case 0444: /* EQV */	return 220;
    case 0445: /* EQVI */	return 220;
    case 0446: /* EQVM */	return 220;
    case 0447: /* EQVB */	return 220;
    case 0450: /* SETCA */	return 220;
    case 0451: /* SETCAI */	return 220;
    case 0452: /* SETCAM */	return 220;
    case 0453: /* SETCAB */	return 220;
    case 0454: /* ORCA */	return 330;
    case 0455: /* ORCAI */	return 330;
    case 0456: /* ORCAM */	return 330;
    case 0457: /* ORCAB */	return 330;
    case 0460: /* SETCM */	return 220;
    case 0461: /* SETCMI */	return 220;
    case 0462: /* SETCMM */	return 220;
    case 0463: /* SETCMB */	return 220;
    case 0464: /* ORCM */	return 330;
    case 0465: /* ORCMI */	return 330;
    case 0466: /* ORCMM */	return 330;
    case 0467: /* ORCMB */	return 330;
    case 0470: /* ORCB */	return 330;
    case 0471: /* ORCBI */	return 330;
    case 0472: /* ORCBM */	return 330;
    case 0473: /* ORCBB */	return 330;
    case 0474: /* SETO */	return 220;
    case 0475: /* SETOI */	return 220;
    case 0476: /* SETOM */	return 220;
    case 0477: /* SETOB */	return 220;
    case 0500: /* HLL */	return 220;
    case 0501: /* HLLI */	return 220;
    case 0502: /* HLLM */	return 330;
    case 0503: /* HLLS */	return 220;
    case 0504: /* HRL */	return 220;
    case 0505: /* HRLI */	return 220;
    case 0506: /* HRLM */	return 330;
    case 0507: /* HRLS */	return 220;
    case 0510: /* HLLZ */	return 220;
    case 0511: /* HLLZI */	return 220;
    case 0512: /* HLLZM */	return 330;
    case 0513: /* HLLZS */	return 220;
    case 0514: /* HRLZ */	return 220;
    case 0515: /* HRLZI */	return 220;
    case 0516: /* HRLZM */	return 330;
    case 0517: /* HRLZS */	return 220;
    case 0520: /* HLLO */	return 220;
    case 0521: /* HLLOI */	return 220;
    case 0522: /* HLLOM */	return 330;
    case 0523: /* HLLOS */	return 220;
    case 0524: /* HRLO */	return 220;
    case 0525: /* HRLOI */	return 220;
    case 0526: /* HRLOM */	return 330;
    case 0527: /* HRLOS */	return 220;
    case 0530: /* HLLE */	return 220;
    case 0531: /* HLLEI */	return 220;
    case 0532: /* HLLEM */	return 330;
    case 0533: /* HLLES */	return 220;
    case 0534: /* HRLE */	return 220;
    case 0535: /* HRLEI */	return 220;
    case 0536: /* HRLEM */	return 330;
    case 0537: /* HRLES */	return 220;
    case 0540: /* HRR */	return 220;
    case 0541: /* HRRI */	return 220;
    case 0542: /* HRRM */	return 330;
    case 0543: /* HRRS */	return 220;
    case 0544: /* HLR */	return 220;
    case 0545: /* HLRI */	return 220;
    case 0546: /* HLRM */	return 330;
    case 0547: /* HLRS */	return 220;
    case 0550: /* HRRZ */	return 220;
    case 0551: /* HRRZI */	return 220;
    case 0552: /* HRRZM */	return 330;
    case 0553: /* HRRZS */	return 220;
    case 0554: /* HLRZ */	return 220;
    case 0555: /* HLRZI */	return 220;
    case 0556: /* HLRZM */	return 330;
    case 0557: /* HLRZS */	return 220;
    case 0560: /* HRRO */	return 220;
    case 0561: /* HRROI */	return 220;
    case 0562: /* HRROM */	return 330;
    case 0563: /* HRROS */	return 220;
    case 0564: /* HLRO */	return 220;
    case 0565: /* HLROI */	return 220;
    case 0566: /* HLROM */	return 330;
    case 0567: /* HLROS */	return 220;
    case 0570: /* HRRE */	return 220;
    case 0571: /* HRREI */	return 220;
    case 0572: /* HRREM */	return 330;
    case 0573: /* HRRES */	return 220;
    case 0574: /* HLRE */	return 220;
    case 0575: /* HLREI */	return 220;
    case 0576: /* HLREM */	return 330;
    case 0577: /* HLRES */	return 220;
    case 0600: /* TRN */	return 110;
    case 0601: /* TLN */	return 220;
    case 0602: /* TRNE */	return 280;
    case 0603: /* TLNE */	return 390;
    case 0604: /* TRNA */	return 220;
    case 0605: /* TLNA */	return 330;
    case 0606: /* TRNN */	return 280;
    case 0607: /* TLNN */	return 390;
    case 0610: /* TDN */	return 110;
    case 0611: /* TSN */	return 220;
    case 0612: /* TDNE */	return 280;
    case 0613: /* TSNE */	return 390;
    case 0614: /* TDNA */	return 220;
    case 0615: /* TSNA */	return 330;
    case 0616: /* TDNN */	return 280;
    case 0617: /* TSNN */	return 390;
    case 0620: /* TRZ */	return 220;
    case 0621: /* TLZ */	return 330;
    case 0622: /* TRZE */	return 390;
    case 0623: /* TLZE */	return 440;
    case 0624: /* TRZA */	return 330;
    case 0625: /* TLZA */	return 440;
    case 0626: /* TRZN */	return 390;
    case 0627: /* TLZN */	return 440;
    case 0630: /* TDZ */	return 220;
    case 0631: /* TSZ */	return 330;
    case 0632: /* TDZE */	return 390;
    case 0633: /* TSZE */	return 500;
    case 0634: /* TDZA */	return 330;
    case 0635: /* TSZA */	return 440;
    case 0636: /* TDZN */	return 390;
    case 0637: /* TSZN */	return 500;
    case 0640: /* TRC */	return 220;
    case 0641: /* TLC */	return 330;
    case 0642: /* TRCE */	return 390;
    case 0643: /* TLCE */	return 440;
    case 0644: /* TRCA */	return 330;
    case 0645: /* TLCA */	return 440;
    case 0646: /* TRCN */	return 390;
    case 0647: /* TLCN */	return 440;
    case 0650: /* TDC */	return 220;
    case 0651: /* TSC */	return 330;
    case 0652: /* TDCE */	return 390;
    case 0653: /* TSCE */	return 440;
    case 0654: /* TDCA */	return 330;
    case 0655: /* TSCA */	return 440;
    case 0656: /* TDCN */	return 390;
    case 0657: /* TSCN */	return 440;
    case 0660: /* TRO */	return 330;
    case 0661: /* TLO */	return 440;
    case 0662: /* TROE */	return 390;
    case 0663: /* TLOE */	return 440;
    case 0664: /* TROA */	return 330;
    case 0665: /* TLOA */	return 440;
    case 0666: /* TRON */	return 390;
    case 0667: /* TLON */	return 440;
    case 0670: /* TDO */	return 330;
    case 0671: /* TSO */	return 440;
    case 0672: /* TDOE */	return 390;
    case 0673: /* TSOE */	return 500;
    case 0674: /* TDOA */	return 330;
    case 0675: /* TSOA */	return 440;
    case 0676: /* TDON */	return 390;
    case 0677: /* TSON */	return 500;
    }

  return -1000000;
}

int
timing_ki10 (word_t instruction)
{
  int opcode = OPCODE (instruction);
  int index_last = 0;
  int nanos = 0;
  int core = 1;

#if 0 /* Instruction fetch overlapped with previous instruction. */
  nanos += 175;
  nanos += memory_read_access ();
  nanos += 100;
#endif

  nanos += 28;

  if (X (instruction))
    nanos += 170;
  else
    nanos += 110;

  if (I (instruction))
    {
      nanos += 175;
      nanos += memory_read_access ();
      nanos += 100;
      nanos += 28;
      nanos += 110;
    }

  if (opcode == 0)
    return -1;
  if ((opcode & 0700) == 0700) /* IO instructions */
    return -1;
  if (opcode == 0251) /* BLT */
    return -1;

  /* FIXME: is E a accumulator or core address?  Set "core" accordingly. */

  switch (opcode)
    {
    case 0000:
      return -1;
    case 0267: /* JRA */
    case 0262: /* POP */
    case 0263: /* POPJ */
      if (index_last)
	nanos += 230;
      else
	nanos += 110;
      if (opcode == 0267) /* JRA */
	nanos += 220;
      nanos += 175;
      nanos += memory_read_access ();
      nanos += 28;
      if (opcode == 0262) /* POP */
	{
	  nanos += 110;
	  nanos += 125;
	  if (core)
	    nanos += 50 + memory_read_access ();
	  else
	    nanos += 93;
	}
      break;
    case 0251: /* BLT */
      {
	int i, n = 10;

	nanos += 790;

	for (i = 0; i < n; i++)
	  {
	    nanos += 175;
	    nanos += 291;
	    if (1 /* FIXME: destination core */)
	      {
		/* FIXME */
		nanos += 126;
		nanos += 60;
	      }
	    else
	      nanos += 110;
	    nanos += 230;
	  }

	nanos += 330;
	nanos += 87;
      }
      return nanos;
    case 0110: /* DFAD */
    case 0111: /* DFSB */
    case 0112: /* DFMP */
    case 0113: /* DFDV */
    case 0120: /* DMOVE */
    case 0121: /* DMOVN */
      nanos += 175;
      
      {
	int nanos1 = 0;
	int nanos2 = 0;

	if (1 /* FIXME: core */)
	  nanos1 += 179;
	nanos1 += 28;
	switch (opcode)
	  {
	  case 0120: /* DMOVE */
	  case 0121: /* DMOVN */
	    nanos1 += 110;
	    break;
	  case 0110: /* DFAD */
	  case 0111: /* DFSB */
	    nanos1 += 450;
	    break;
	  case 0112: /* DFMP */
	    nanos1 += 450;
	    break;
	  case 0113: /* DFDV */
	    nanos1 += 450;
	    nanos1 += 40; /* FIXME: assume 50% negative dividends */
	    break;
	  }

	nanos2 += 223;
#if 0
	if (!1 /* FIXME: core */)
	  /* FIXME: sync with nanos2 */;
#endif
	nanos2 += 50;
	nanos2 += 28;

	nanos = max (nanos1, nanos2);
      }

      switch (opcode)
	{
	case 0111: /* DFSB */
	  nanos += 191; /* FIXME... */
	  /* FALLTHROUGH */
	case 0110: /* DFAD */
	  nanos += 382; /* FIXME... */
	  break;
	case 0112: /* DFMP */
	  nanos += 4682; /* FIXME... */
	  break;
	case 0113: /* DFDV */
	  nanos += 12734; /* FIXME... */
	  break;
	case 0120: /* DMOVE */
	  nanos += 417;
	  break;
	case 0121: /* DMOVN */
	  nanos += 757;
	  if (0 /* FIXME: zero low word */)
	    nanos += 60;
	  break;
	}

      return nanos;
    default:

      if (memory_read (instruction) ||
	  memory_read_modify_write (instruction))
	{
	  nanos += 175;
	  nanos += memory_read_access ();
	  nanos += 28;
	}
      else
	{
	  if (memory_write (instruction))
	    {
	      nanos += 125;
	      nanos += 28;
	    }
	  if (index_last)
	    nanos += 110;
	}

      break;
    }

  nanos += instruction_execution (instruction);

  if (memory_write (instruction))
    {
      if (core)
	{
	  nanos += 126;
	  nanos += wait_for_acknowledgement ();
	  nanos += 60;
	  if (accumulator_write (instruction))
	    nanos += 280;
	  else
	    {
	      nanos += 170;
	      /* FIXME */
	    }
	}
      else
	{
	  nanos += 390;
	  if (accumulator_write (instruction))
	    nanos += 390;
	  else
	    {
	      nanos += 170;
	      /* FIXME */
	    }
	}
    }
  else
    {
      if (accumulator_write (instruction))
	nanos += 220;
    }

  nanos += 82;

  return nanos;
}
