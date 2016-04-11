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
#include "timing.h"

#define OPCODE(word)	(int)(((word) >> 27) &    0777)
#define A(word)		(int)(((word) >> 23) &     017)
#define OPCODE_A(word)	(((OPCODE (word) << 4) | A (word)) << 2)
#define I(word)		(int)(((word) >> 22) &       1)
#define X(word)		(int)(((word) >> 18) &     017)
#define Y(word)		(int)( (word)        & 0777777)

#define NONE	0x00
#define READ	0x01
#define MODIFY	0x02
#define WRITE	0x04
#define RACC	0x08
#define WACC	0x10
#define RWACC	(RACC | WACC)

int
instruction_time (word_t instruction, int cpu_model)
{
  switch (cpu_model)
    {
    case PDP10_KA10: return timing_ka10 (instruction);
    case PDP10_KI10: return timing_ki10 (instruction);
    default:         return -1;
    }
}

static int
memory_op (word_t instruction)
{
  int opcode = OPCODE (instruction);

  switch (opcode)
    {
    case 0100: /* UJEN */	return NONE; /* ??? */
    case 0102: /* GFAD */	return READ			| RWACC;
    case 0103: /* GFSB */	return READ			| RWACC;
    case 0104: /* JSYS */	return NONE;
    case 0105: /* ADJSP */	return NONE			| RWACC;
    case 0106: /* GFMP */	return READ			| RWACC;
    case 0107: /* GFDV */	return READ			| RWACC;
    case 0110: /* DFAD */	return READ			| RWACC;
    case 0111: /* DFSB */	return READ			| RWACC;
    case 0112: /* DFMP */	return READ			| RWACC;
    case 0113: /* DFDV */	return READ			| RWACC;
    case 0114: /* DADD */	return READ			| RWACC;
    case 0115: /* DSUB */	return READ			| RWACC;
    case 0116: /* DMUL */	return READ			| RWACC;
    case 0117: /* DDIV */	return READ			| RWACC;
    case 0120: /* DMOVE */	return READ;
    case 0121: /* DMOVN */	return READ;
    case 0122: /* FIX */	return READ;
    case 0123: /* EXTEND */	return NONE;
    case 0124: /* DMOVEM */	return WRITE			| RACC;
    case 0125: /* DMOVNM */	return WRITE			| RACC;
    case 0126: /* FIXR */	return READ;
    case 0127: /* FLTR */	return READ;
    case 0130: /* UFA */	return READ;
    case 0131: /* DFN */	return READ | MODIFY | WRITE;
    case 0132: /* FSC */	return READ;
    case 0133: /* IBP */	return READ | MODIFY | WRITE;
    case 0134: /* ILDB */	return READ | MODIFY | WRITE;
    case 0135: /* LDB */	return READ;
    case 0136: /* IDPB */	return READ | MODIFY | WRITE;
    case 0137: /* DPB */	return READ | WRITE;
    case 0140: /* FAD */	return READ			| RWACC;
    case 0141: /* FADL */	return READ			| RWACC;
    case 0142: /* FADM */	return READ | WRITE		| RWACC;
    case 0143: /* FADB */	return READ | WRITE;
    case 0144: /* FADR */	return READ			| RWACC;
    case 0145: /* FADRI */	return NONE			| RWACC;
    case 0146: /* FADRM */	return READ | WRITE;
    case 0147: /* FADRB */	return READ | WRITE;
    case 0150: /* FSB */	return READ			| RWACC;
    case 0151: /* FSBL */	return READ			| RWACC;
    case 0152: /* FSBM */	return READ | WRITE;
    case 0153: /* FSBB */	return READ | WRITE;
    case 0154: /* FSBR */	return READ			| RWACC;
    case 0155: /* FSBRI */	return NONE			| RWACC;
    case 0156: /* FSBRM */	return READ | WRITE;
    case 0157: /* FSBRB */	return READ | WRITE;
    case 0160: /* FMP */	return READ			| RWACC;
    case 0161: /* FMPL */	return READ			| RWACC;
    case 0162: /* FMPM */	return READ | WRITE;
    case 0163: /* FMPB */	return READ | WRITE;
    case 0164: /* FMPR */	return READ			| RWACC;
    case 0165: /* FMPRI */	return NONE			| RWACC;
    case 0166: /* FMPRM */	return READ | WRITE;
    case 0167: /* FMPRB */	return READ | WRITE;
    case 0170: /* FDV */	return READ			| RWACC;
    case 0171: /* FDVL */	return READ			| RWACC;
    case 0172: /* FDVM */	return READ | WRITE;
    case 0173: /* FDVB */	return READ | WRITE;
    case 0174: /* FDVR */	return READ			| RWACC;
    case 0175: /* FDVRI */	return NONE			| RWACC;
    case 0176: /* FDVRM */	return READ | WRITE;
    case 0177: /* FDVRB */	return READ | WRITE;
    case 0200: /* MOVE */	return READ;
    case 0201: /* MOVEI */	return NONE;
    case 0202: /* MOVEM */	return WRITE			| RACC;
    case 0203: /* MOVES */	return READ | WRITE		| RWACC;
    case 0204: /* MOVS */	return READ;
    case 0205: /* MOVSI */	return NONE;
    case 0206: /* MOVSM */	return WRITE			| RACC;
    case 0207: /* MOVSS */	return READ | WRITE		| RWACC;
    case 0210: /* MOVN */	return READ;
    case 0211: /* MOVNI */	return NONE;
    case 0212: /* MOVNM */	return WRITE			| RACC;
    case 0213: /* MOVNS */	return READ | WRITE		| RWACC;
    case 0214: /* MOVM */	return READ;
    case 0215: /* MOVMI */	return NONE;
    case 0216: /* MOVMM */	return WRITE			| RACC;
    case 0217: /* MOVMS */	return READ | WRITE		| RWACC;
    case 0220: /* IMUL */	return READ			| RWACC;
    case 0221: /* IMULI */	return NONE			| RWACC;
    case 0222: /* IMULM */	return READ | WRITE		| RACC;
    case 0223: /* IMULB */	return READ | WRITE		| RWACC;
    case 0224: /* MUL */	return READ			| RWACC;
    case 0225: /* MULI */	return NONE			| RWACC;
    case 0226: /* MULM */	return READ | WRITE		| RACC;
    case 0227: /* MULB */	return READ | WRITE		| RWACC;
    case 0230: /* IDIV */	return READ			| RWACC;
    case 0231: /* IDIVI */	return NONE			| RWACC;
    case 0232: /* IDIVM */	return READ | WRITE		| RACC;
    case 0233: /* IDIVB */	return READ | WRITE		| RWACC;
    case 0234: /* DIV */	return READ			| RWACC;
    case 0235: /* DIVI */	return NONE			| RWACC;
    case 0236: /* DIVM */	return READ | WRITE		| RACC;
    case 0237: /* DIVB */	return READ | WRITE		| RWACC;
    case 0240: /* ASH */	return NONE			| RWACC;
    case 0241: /* ROT */	return NONE			| RWACC;
    case 0242: /* LSH */	return NONE			| RWACC;
    case 0243: /* JFFO */	return NONE			| RWACC;
    case 0244: /* ASHC */	return NONE			| RWACC;
    case 0245: /* ROTC */	return NONE			| RWACC;
    case 0246: /* LSHC */	return NONE			| RWACC;
    case 0250: /* EXCH */	return READ | MODIFY | WRITE	| RWACC;
    case 0251: /* BLT */	return READ | WRITE		| RWACC;
    case 0252: /* AOBJP */	return NONE			| RWACC;
    case 0253: /* AOBJN */	return NONE			| RWACC;
    case 0254: /* JRST */	return NONE;
    case 0255: /* JFCL */	return NONE;
    case 0256: /* XCT */	return NONE;
    case 0257: /* MAP */	return NONE; /* ??? */
    case 0260: /* PUSHJ */	return WRITE			| RWACC;
    case 0261: /* PUSH */	return WRITE			| RWACC;
    case 0262: /* POP */	return READ | WRITE		| RWACC;
    case 0263: /* POPJ */	return READ			| RWACC;
    case 0264: /* JSR */	return WRITE;
    case 0265: /* JSP */	return NONE			| WACC;
    case 0266: /* JSA */	return WRITE			| RACC;
    case 0267: /* JRA */	return READ			| WACC;
    case 0270: /* ADD */	return WRITE			| RWACC;
    case 0271: /* ADDI */	return NONE			| RWACC;
    case 0272: /* ADDM */	return READ | MODIFY | WRITE	| RWACC;
    case 0273: /* ADDB */	return READ | MODIFY | WRITE	| RWACC;
    case 0274: /* SUB */	return WRITE			| RWACC;
    case 0275: /* SUBI */	return NONE			| RWACC;
    case 0276: /* SUBM */	return READ | MODIFY | WRITE	| RWACC;
    case 0277: /* SUBB */	return READ | MODIFY | WRITE	| RWACC;
    case 0300: /* CAI */	return NONE			| RACC;
    case 0301: /* CAIL */	return NONE			| RACC;
    case 0302: /* CAIE */	return NONE			| RACC;
    case 0303: /* CAILE */	return NONE			| RACC;
    case 0304: /* CAIA */	return NONE			| RACC;
    case 0305: /* CAIGE */	return NONE			| RACC;
    case 0306: /* CAIN */	return NONE			| RACC;
    case 0307: /* CAIG */	return NONE			| RACC;
    case 0310: /* CAM */	return READ			| RACC;
    case 0311: /* CAML */	return READ			| RACC;
    case 0312: /* CAME */	return READ			| RACC;
    case 0313: /* CAMLE */	return READ			| RACC;
    case 0314: /* CAMA */	return READ			| RACC;
    case 0315: /* CAMGE */	return READ			| RACC;
    case 0316: /* CAMN */	return READ			| RACC;
    case 0317: /* CAMG */	return READ			| RACC;
    case 0320: /* JUMP */	return NONE			| RACC;
    case 0321: /* JUMPL */	return NONE			| RACC;
    case 0322: /* JUMPE */	return NONE			| RACC;
    case 0323: /* JUMPLE */	return NONE			| RACC;
    case 0324: /* JUMPA */	return NONE			| RACC;
    case 0325: /* JUMPGE */	return NONE			| RACC;
    case 0326: /* JUMPN */	return NONE			| RACC;
    case 0327: /* JUMPG */	return NONE			| RACC;
    case 0330: /* SKIP */	return READ			| RWACC;
    case 0331: /* SKIPL */	return READ			| RWACC;
    case 0332: /* SKIPE */	return READ			| RWACC;
    case 0333: /* SKIPLE */	return READ			| RWACC;
    case 0334: /* SKIPA */	return READ			| RWACC;
    case 0335: /* SKIPGE */	return READ			| RWACC;
    case 0336: /* SKIPN */	return READ			| RWACC;
    case 0337: /* SKIPG */	return READ			| RWACC;
    case 0340: /* AOJ */	return NONE			| RWACC;
    case 0341: /* AOJL */	return NONE			| RWACC;
    case 0342: /* AOJE */	return NONE			| RWACC;
    case 0343: /* AOJLE */	return NONE			| RWACC;
    case 0344: /* AOJA */	return NONE			| RWACC;
    case 0345: /* AOJGE */	return NONE			| RWACC;
    case 0346: /* AOJN */	return NONE			| RWACC;
    case 0347: /* AOJG */	return NONE			| RWACC;
    case 0350: /* AOS */	return READ | MODIFY | WRITE	| RWACC;
    case 0351: /* AOSL */	return READ | MODIFY | WRITE	| RWACC;
    case 0352: /* AOSE */	return READ | MODIFY | WRITE	| RWACC;
    case 0353: /* AOSLE */	return READ | MODIFY | WRITE	| RWACC;
    case 0354: /* AOSA */	return READ | MODIFY | WRITE	| RWACC;
    case 0355: /* AOSGE */	return READ | MODIFY | WRITE	| RWACC;
    case 0356: /* AOSN */	return READ | MODIFY | WRITE	| RWACC;
    case 0357: /* AOSG */	return READ | MODIFY | WRITE	| RWACC;
    case 0360: /* SOJ */	return NONE			| RWACC;
    case 0361: /* SOJL */	return NONE			| RWACC;
    case 0362: /* SOJE */	return NONE			| RWACC;
    case 0363: /* SOJLE */	return NONE			| RWACC;
    case 0364: /* SOJA */	return NONE			| RWACC;
    case 0365: /* SOJGE */	return NONE			| RWACC;
    case 0366: /* SOJN */	return NONE			| RWACC;
    case 0367: /* SOJG */	return NONE			| RWACC;
    case 0370: /* SOS */	return READ | MODIFY | WRITE	| RWACC;
    case 0371: /* SOSL */	return READ | MODIFY | WRITE	| RWACC;
    case 0372: /* SOSE */	return READ | MODIFY | WRITE	| RWACC;
    case 0373: /* SOSLE */	return READ | MODIFY | WRITE	| RWACC;
    case 0374: /* SOSA */	return READ | MODIFY | WRITE	| RWACC;
    case 0375: /* SOSGE */	return READ | MODIFY | WRITE	| RWACC;
    case 0376: /* SOSN */	return READ | MODIFY | WRITE	| RWACC;
    case 0377: /* SOSG */	return READ | MODIFY | WRITE	| RWACC;
    case 0400: /* SETZ */	return NONE			| WACC;
    case 0401: /* SETZI */	return NONE			| WACC;
    case 0402: /* SETZM */	return WRITE;
    case 0403: /* SETZB */	return WRITE			| WACC;
    case 0404: /* AND */	return READ			| RWACC;
    case 0405: /* ANDI */	return NONE			| RWACC;
    case 0406: /* ANDM */	return READ | MODIFY | WRITE	| RACC;
    case 0407: /* ANDB */	return READ | MODIFY | WRITE	| RWACC;
    case 0410: /* ANDCA */	return NONE			| RWACC;
    case 0411: /* ANDCAI */	return NONE			| RWACC;
    case 0412: /* ANDCAM */	return READ | MODIFY | WRITE	| RACC;
    case 0413: /* ANDCAB */	return READ | MODIFY | WRITE	| RWACC;
    case 0414: /* SETM */	return READ			| WACC;
    case 0415: /* SETMI */	return NONE			| WACC;
    case 0416: /* SETMM */	return READ | MODIFY | WRITE;
    case 0417: /* SETMB */	return READ | MODIFY | WRITE	| WACC;
    case 0420: /* ANDCM */	return READ			| WACC;
    case 0421: /* ANDCMI */	return NONE			| WACC;
    case 0422: /* ANDCMM */	return READ | MODIFY | WRITE;
    case 0423: /* ANDCMB */	return READ | MODIFY | WRITE	| WACC;
    case 0424: /* SETA */	return NONE			| RWACC;
    case 0425: /* SETAI */	return NONE			| RWACC;
    case 0426: /* SETAM */	return WRITE			| RACC;
    case 0427: /* SETAB */	return WRITE			| RWACC;
    case 0430: /* XOR */	return READ			| RWACC;
    case 0431: /* XORI */	return NONE			| RWACC;
    case 0432: /* XORM */	return READ | MODIFY | WRITE	| RACC;
    case 0433: /* XORB */	return READ | MODIFY | WRITE	| RWACC;
    case 0434: /* OR */		return READ			| RWACC;
    case 0435: /* ORI */	return NONE			| RWACC;
    case 0436: /* ORM */	return READ | MODIFY | WRITE	| RACC;
    case 0437: /* ORB */	return READ | MODIFY | WRITE	| RWACC;
    case 0440: /* ANDCB */	return READ | MODIFY | WRITE	| RWACC;
    case 0441: /* ANDCBI */	return NONE			| RWACC;
    case 0442: /* ANDCBM */	return READ | MODIFY | WRITE	| RACC;
    case 0443: /* ANDCBB */	return READ | MODIFY | WRITE	| RWACC;
    case 0444: /* EQV */	return READ			| RWACC;
    case 0445: /* EQVI */	return NONE			| RWACC;
    case 0446: /* EQVM */	return READ | MODIFY | WRITE	| RACC;
    case 0447: /* EQVB */	return READ | MODIFY | WRITE	| RWACC;
    case 0450: /* SETCA */	return NONE			| RWACC;
    case 0451: /* SETCAI */	return NONE			| RWACC;
    case 0452: /* SETCAM */	return WRITE			| RACC;
    case 0453: /* SETCAB */	return WRITE			| RWACC;
    case 0454: /* ORCA */	return NONE			| RWACC;
    case 0455: /* ORCAI */	return NONE			| RWACC;
    case 0456: /* ORCAM */	return READ | MODIFY | WRITE	| RACC;
    case 0457: /* ORCAB */	return READ | MODIFY | WRITE	| RWACC;
    case 0460: /* SETCM */	return READ			| WACC;
    case 0461: /* SETCMI */	return NONE			| WACC;
    case 0462: /* SETCMM */	return READ | MODIFY | WRITE;
    case 0463: /* SETCMB */	return READ | MODIFY | WRITE	| WACC;
    case 0464: /* ORCM */	return READ			| WACC;
    case 0465: /* ORCMI */	return NONE			| WACC;
    case 0466: /* ORCMM */	return READ | MODIFY | WRITE;
    case 0467: /* ORCMB */	return READ | MODIFY | WRITE	| WACC;
    case 0470: /* ORCB */	return READ | MODIFY | WRITE	| RWACC;
    case 0471: /* ORCBI */	return NONE			| RWACC;
    case 0472: /* ORCBM */	return READ | MODIFY | WRITE	| RACC;
    case 0473: /* ORCBB */	return READ | MODIFY | WRITE	| RWACC;
    case 0474: /* SETO */	return NONE			| WACC;
    case 0475: /* SETOI */	return NONE			| WACC;
    case 0476: /* SETOM */	return WRITE;
    case 0477: /* SETOB */	return WRITE			| WACC;
    case 0500: /* HLL */	return READ			| WACC;
    case 0501: /* HLLI */	return NONE			| WACC;
    case 0502: /* HLLM */	return READ | MODIFY | WRITE	| WACC;
    case 0503: /* HLLS */	return READ | MODIFY | WRITE	| WACC;
    case 0504: /* HRL */	return READ			| WACC;
    case 0505: /* HRLI */	return NONE			| WACC;
    case 0506: /* HRLM */	return READ | MODIFY | WRITE	| WACC;
    case 0507: /* HRLS */	return READ | MODIFY | WRITE	| WACC;
    case 0510: /* HLLZ */	return READ			| WACC;
    case 0511: /* HLLZI */	return NONE			| WACC;
    case 0512: /* HLLZM */	return WRITE			| WACC;
    case 0513: /* HLLZS */	return READ | MODIFY | WRITE	| WACC;
    case 0514: /* HRLZ */	return READ			| WACC;
    case 0515: /* HRLZI */	return NONE			| WACC;
    case 0516: /* HRLZM */	return WRITE			| WACC;
    case 0517: /* HRLZS */	return READ | MODIFY | WRITE	| WACC;
    case 0520: /* HLLO */	return READ			| WACC;
    case 0521: /* HLLOI */	return NONE			| WACC;
    case 0522: /* HLLOM */	return WRITE			| WACC;
    case 0523: /* HLLOS */	return READ | MODIFY | WRITE	| WACC;
    case 0524: /* HRLO */	return READ			| WACC;
    case 0525: /* HRLOI */	return NONE			| WACC;
    case 0526: /* HRLOM */	return WRITE			| WACC;
    case 0527: /* HRLOS */	return READ | MODIFY | WRITE	| WACC;
    case 0530: /* HLLE */	return READ			| WACC;
    case 0531: /* HLLEI */	return NONE			| WACC;
    case 0532: /* HLLEM */	return WRITE			| WACC;
    case 0533: /* HLLES */	return READ | MODIFY | WRITE	| WACC;
    case 0534: /* HRLE */	return READ			| WACC;
    case 0535: /* HRLEI */	return NONE			| WACC;
    case 0536: /* HRLEM */	return WRITE			| WACC;
    case 0537: /* HRLES */	return READ | MODIFY | WRITE	| WACC;
    case 0540: /* HRR */	return READ			| WACC;
    case 0541: /* HRRI */	return NONE			| WACC;
    case 0542: /* HRRM */	return READ | MODIFY | WRITE	| WACC;
    case 0543: /* HRRS */	return READ | MODIFY | WRITE	| WACC;
    case 0544: /* HLR */	return READ			| WACC;
    case 0545: /* HLRI */	return NONE			| WACC;
    case 0546: /* HLRM */	return READ | MODIFY | WRITE	| WACC;
    case 0547: /* HLRS */	return READ | MODIFY | WRITE	| WACC;
    case 0550: /* HRRZ */	return READ			| WACC;
    case 0551: /* HRRZI */	return NONE			| WACC;
    case 0552: /* HRRZM */	return WRITE			| WACC;
    case 0553: /* HRRZS */	return READ | MODIFY | WRITE	| WACC;
    case 0554: /* HLRZ */	return READ			| WACC;
    case 0555: /* HLRZI */	return NONE			| WACC;
    case 0556: /* HLRZM */	return WRITE			| WACC;
    case 0557: /* HLRZS */	return READ | MODIFY | WRITE	| WACC;
    case 0560: /* HRRO */	return READ			| WACC;
    case 0561: /* HRROI */	return NONE			| WACC;
    case 0562: /* HRROM */	return WRITE			| WACC;
    case 0563: /* HRROS */	return READ | MODIFY | WRITE	| WACC;
    case 0564: /* HLRO */	return READ			| WACC;
    case 0565: /* HLROI */	return NONE			| WACC;
    case 0566: /* HLROM */	return WRITE			| WACC;
    case 0567: /* HLROS */	return READ | MODIFY | WRITE	| WACC;
    case 0570: /* HRRE */	return READ			| WACC;
    case 0571: /* HRREI */	return NONE			| WACC;
    case 0572: /* HRREM */	return WRITE			| WACC;
    case 0573: /* HRRES */	return READ | MODIFY | WRITE	| WACC;
    case 0574: /* HLRE */	return READ			| WACC;
    case 0575: /* HLREI */	return NONE			| WACC;
    case 0576: /* HLREM */	return WRITE			| WACC;
    case 0577: /* HLRES */	return READ | MODIFY | WRITE	| WACC;
    case 0600: /* TRN */	return NONE;
    case 0601: /* TLN */	return NONE;
    case 0602: /* TRNE */	return NONE;
    case 0603: /* TLNE */	return NONE;
    case 0604: /* TRNA */	return NONE;
    case 0605: /* TLNA */	return NONE;
    case 0606: /* TRNN */	return NONE;
    case 0607: /* TLNN */	return NONE;
    case 0610: /* TDN */	return READ;
    case 0611: /* TSN */	return READ;
    case 0612: /* TDNE */	return READ;
    case 0613: /* TSNE */	return READ;
    case 0614: /* TDNA */	return READ;
    case 0615: /* TSNA */	return READ;
    case 0616: /* TDNN */	return READ;
    case 0617: /* TSNN */	return READ;
    case 0620: /* TRZ */	return NONE			| WACC;
    case 0621: /* TLZ */	return NONE			| WACC;
    case 0622: /* TRZE */	return NONE			| WACC;
    case 0623: /* TLZE */	return NONE			| WACC;
    case 0624: /* TRZA */	return NONE			| WACC;
    case 0625: /* TLZA */	return NONE			| WACC;
    case 0626: /* TRZN */	return NONE			| WACC;
    case 0627: /* TLZN */	return NONE			| WACC;
    case 0630: /* TDZ */	return READ			| WACC;
    case 0631: /* TSZ */	return READ			| WACC;
    case 0632: /* TDZE */	return READ			| WACC;
    case 0633: /* TSZE */	return READ			| WACC;
    case 0634: /* TDZA */	return READ			| WACC;
    case 0635: /* TSZA */	return READ			| WACC;
    case 0636: /* TDZN */	return READ			| WACC;
    case 0637: /* TSZN */	return READ			| WACC;
    case 0640: /* TRC */	return NONE			| WACC;
    case 0641: /* TLC */	return NONE			| WACC;
    case 0642: /* TRCE */	return NONE			| WACC;
    case 0643: /* TLCE */	return NONE			| WACC;
    case 0644: /* TRCA */	return NONE			| WACC;
    case 0645: /* TLCA */	return NONE			| WACC;
    case 0646: /* TRCN */	return NONE			| WACC;
    case 0647: /* TLCN */	return NONE			| WACC;
    case 0650: /* TDC */	return READ			| WACC;
    case 0651: /* TSC */	return READ			| WACC;
    case 0652: /* TDCE */	return READ			| WACC;
    case 0653: /* TSCE */	return READ			| WACC;
    case 0654: /* TDCA */	return READ			| WACC;
    case 0655: /* TSCA */	return READ			| WACC;
    case 0656: /* TDCN */	return READ			| WACC;
    case 0657: /* TSCN */	return READ			| WACC;
    case 0660: /* TRO */	return NONE			| WACC;
    case 0661: /* TLO */	return NONE			| WACC;
    case 0662: /* TROE */	return NONE			| WACC;
    case 0663: /* TLOE */	return NONE			| WACC;
    case 0664: /* TROA */	return NONE			| WACC;
    case 0665: /* TLOA */	return NONE			| WACC;
    case 0666: /* TRON */	return NONE			| WACC;
    case 0667: /* TLON */	return NONE			| WACC;
    case 0670: /* TDO */	return READ			| WACC;
    case 0671: /* TSO */	return READ			| WACC;
    case 0672: /* TDOE */	return READ			| WACC;
    case 0673: /* TSOE */	return READ			| WACC;
    case 0674: /* TDOA */	return READ			| WACC;
    case 0675: /* TSOA */	return READ			| WACC;
    case 0676: /* TDON */	return READ			| WACC;
    case 0677: /* TSON */	return READ			| WACC;
    }

  return -1000000;
}

int
memory_read (word_t instruction)
{
  int op = memory_op (instruction) & (READ | MODIFY | WRITE);
  return op == READ || op == (READ | WRITE);
}

int
memory_read_modify_write (word_t instruction)
{
  int op = memory_op (instruction) & (READ | MODIFY | WRITE);
  return op == (READ | MODIFY | WRITE);
}

int
memory_write (word_t instruction)
{
  int op = memory_op (instruction) & (READ | MODIFY | WRITE);
  return op == WRITE || op == (READ | WRITE);
}

int
floating_point_immediate (word_t instruction)
{
  int opcode = OPCODE (instruction);

  switch (opcode)
    {
      /* FIXME: KA10 only */
    case 0145: /* FADRI */
    case 0155: /* FSBRI */
    case 0165: /* FMPRI */
    case 0175: /* FDVRI */
      return 1;
    default:
      return 0;
    }
}

int accumulator_read (word_t instruction)
{
  return (memory_op (instruction) & RACC) != 0;
}

int accumulator_write (word_t instruction)
{
  return (memory_op (instruction) & WACC) != 0;
}
