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
  return 600;
}

static int
memory_write_access ()
{
  return 200;
}

static int
instruction_execution (int opcode)
{
  if ((opcode & 0700) == 0700)
    return 270;

  switch (opcode)
    {
    case 0130: /* UFA */	return 4330; /* FIXME: average */
    case 0131: /* DFN */	return 800;
    case 0132: /* FSC */	return 1520 + 250 * 5; /* FIXME: average */
    case 0133: /* IBP */	return 380 + 260 / 5; /* FIXME: average */
    case 0134: /* ILDB */	return 0;
    case 0135: /* LDB */	return 0;
    case 0136: /* IDPB */	return 0;
    case 0137: /* DPB */	return 0;
    case 0140: /* FAD */	return 4330; /* FIXME: average */
    case 0141: /* FADL */	return 5020;
    case 0142: /* FADM */	return 4330;
    case 0143: /* FADB */	return 4330;
    case 0144: /* FADR */	return 4810; /* FIXME: average */
    case 0145: /* FADRI */	return 4810;
    case 0146: /* FADRM */	return 4810;
    case 0147: /* FADRB */	return 4810;
    case 0150: /* FSB */	return 4510; /* FIXME: average */
    case 0151: /* FSBL */	return 5200;
    case 0152: /* FSBM */	return 4510;
    case 0153: /* FSBB */	return 4510;
    case 0154: /* FSBR */	return 4990; /* FIXME: average */
    case 0155: /* FSBRI */	return 4990;
    case 0156: /* FSBRM */	return 4990;
    case 0157: /* FSBRB */	return 4990;
    case 0160: /* FMP */	return 8210; /* FIXME: average */
    case 0161: /* FMPL */	return 8900;
    case 0162: /* FMPM */	return 8210;
    case 0163: /* FMPB */	return 8210;
    case 0164: /* FMPR */	return 8690; /* FIXME: average */
    case 0165: /* FMPRI */	return 8690;
    case 0166: /* FMPRM */	return 8690;
    case 0167: /* FMPRB */	return 8690;
    case 0170: /* FDV */	return 12000;
    case 0171: /* FDVL */	return 13280; /* FIXME: fast_registers */
    case 0172: /* FDVM */	return 12000;
    case 0173: /* FDVB */	return 12000;
    case 0174: /* FDVR */	return 12000;
    case 0175: /* FDVRI */	return 12000;
    case 0176: /* FDVRM */	return 12000;
    case 0177: /* FDVRB */	return 12000;
    case 0200: /* MOVE */	return 270;
    case 0201: /* MOVEI */	return 270;
    case 0202: /* MOVEM */	return 270;
    case 0203: /* MOVES */	return 270;
    case 0204: /* MOVS */	return 270;
    case 0205: /* MOVSI */	return 270;
    case 0206: /* MOVSM */	return 270;
    case 0207: /* MOVSS */	return 270;
    case 0210: /* MOVN */	return 450;
    case 0211: /* MOVNI */	return 450;
    case 0212: /* MOVNM */	return 450;
    case 0213: /* MOVNS */	return 450;
    case 0214: /* MOVM */	return 450;
    case 0215: /* MOVMI */	return 450;
    case 0216: /* MOVMM */	return 450;
    case 0217: /* MOVMS */	return 450;
    case 0220: /* IMUL */	return 7510; /* FIXME: average */
    case 0221: /* IMULI */	return 7510;
    case 0222: /* IMULM */	return 7510;
    case 0223: /* IMULB */	return 7510;
    case 0224: /* MUL */	return 8360; /* FIXME: average */
    case 0225: /* MULI */	return 8360;
    case 0226: /* MULM */	return 8360;
    case 0227: /* MULB */	return 8360;
    case 0230: /* IDIV */	return 13780;
    case 0231: /* IDIVI */	return 13780;
    case 0232: /* IDIVM */	return 13780;
    case 0233: /* IDIVB */	return 13780;
    case 0234: /* DIV */	return 13780;
    case 0235: /* DIVI */	return 13780;
    case 0236: /* DIVM */	return 13780;
    case 0237: /* DIVB */	return 13780;
    case 0240: /* ASH */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0241: /* ROT */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0242: /* LSH */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0243: /* JFFO */	return 800 + 190 * 9; /* FIXME: average */
    case 0244: /* ASHC */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0245: /* ROTC */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0246: /* LSHC */	return (390 + 230)/2 + 150 * 4; /* average */
    case 0250: /* EXCH */	return 270;
    case 0251: /* BLT */	return 0; /* FIXME */
    case 0252: /* AOBJP */	return 450;
    case 0253: /* AOBJN */	return 450;
    case 0254: /* JRST */	return 270;
    case 0255: /* JFCL */	return 270;
    case 0256: /* XCT */	return 270;
    case 0257: /* MAP */	return 0;
    case 0260: /* PUSHJ */	return 800;
    case 0261: /* PUSH */	return 800;
    case 0262: /* POP */	return 800;
    case 0263: /* POPJ */	return 800;
    case 0264: /* JSR */	return 620;
    case 0265: /* JSP */	return 270;
    case 0266: /* JSA */	return 620;
    case 0267: /* JRA */	return 620;
    case 0270: /* ADD */	return 450;
    case 0271: /* ADDI */	return 450;
    case 0272: /* ADDM */	return 450;
    case 0273: /* ADDB */	return 450;
    case 0274: /* SUB */	return 450;
    case 0275: /* SUBI */	return 450;
    case 0276: /* SUBM */	return 450;
    case 0277: /* SUBB */	return 450;
    case 0300: /* CAI */	return 450;
    case 0301: /* CAIL */	return 450;
    case 0302: /* CAIE */	return 450;
    case 0303: /* CAILE */	return 450;
    case 0304: /* CAIA */	return 450;
    case 0305: /* CAIGE */	return 450;
    case 0306: /* CAIN */	return 450;
    case 0307: /* CAIG */	return 450;
    case 0310: /* CAM */	return 450;
    case 0311: /* CAML */	return 450;
    case 0312: /* CAME */	return 450;
    case 0313: /* CAMLE */	return 450;
    case 0314: /* CAMA */	return 450;
    case 0315: /* CAMGE */	return 450;
    case 0316: /* CAMN */	return 450;
    case 0317: /* CAMG */	return 450;
    case 0320: /* JUMP */	return 450;
    case 0321: /* JUMPL */	return 450;
    case 0322: /* JUMPE */	return 450;
    case 0323: /* JUMPLE */	return 450;
    case 0324: /* JUMPA */	return 450;
    case 0325: /* JUMPGE */	return 450;
    case 0326: /* JUMPN */	return 450;
    case 0327: /* JUMPG */	return 450;
    case 0330: /* SKIP */	return 450;
    case 0331: /* SKIPL */	return 450;
    case 0332: /* SKIPE */	return 450;
    case 0333: /* SKIPLE */	return 450;
    case 0334: /* SKIPA */	return 450;
    case 0335: /* SKIPGE */	return 450;
    case 0336: /* SKIPN */	return 450;
    case 0337: /* SKIPG */	return 450;
    case 0340: /* AOJ */	return 450;
    case 0341: /* AOJL */	return 450;
    case 0342: /* AOJE */	return 450;
    case 0343: /* AOJLE */	return 450;
    case 0344: /* AOJA */	return 450;
    case 0345: /* AOJGE */	return 450;
    case 0346: /* AOJN */	return 450;
    case 0347: /* AOJG */	return 450;
    case 0350: /* AOS */	return 450;
    case 0351: /* AOSL */	return 450;
    case 0352: /* AOSE */	return 450;
    case 0353: /* AOSLE */	return 450;
    case 0354: /* AOSA */	return 450;
    case 0355: /* AOSGE */	return 450;
    case 0356: /* AOSN */	return 450;
    case 0357: /* AOSG */	return 450;
    case 0360: /* SOJ */	return 450;
    case 0361: /* SOJL */	return 450;
    case 0362: /* SOJE */	return 450;
    case 0363: /* SOJLE */	return 450;
    case 0364: /* SOJA */	return 450;
    case 0365: /* SOJGE */	return 450;
    case 0366: /* SOJN */	return 450;
    case 0367: /* SOJG */	return 450;
    case 0370: /* SOS */	return 450;
    case 0371: /* SOSL */	return 450;
    case 0372: /* SOSE */	return 450;
    case 0373: /* SOSLE */	return 450;
    case 0374: /* SOSA */	return 450;
    case 0375: /* SOSGE */	return 450;
    case 0376: /* SOSN */	return 450;
    case 0377: /* SOSG */	return 450;
    case 0400: /* SETZ */	return 270;
    case 0401: /* SETZI */	return 270;
    case 0402: /* SETZM */	return 270;
    case 0403: /* SETZB */	return 270;
    case 0404: /* AND */	return 270;
    case 0405: /* ANDI */	return 270;
    case 0406: /* ANDM */	return 270;
    case 0407: /* ANDB */	return 270;
    case 0410: /* ANDCA */	return 620;
    case 0411: /* ANDCAI */	return 620;
    case 0412: /* ANDCAM */	return 620;
    case 0413: /* ANDCAB */	return 620;
    case 0414: /* SETM */	return 270;
    case 0415: /* SETMI */	return 270;
    case 0416: /* SETMM */	return 270;
    case 0417: /* SETMB */	return 270;
    case 0420: /* ANDCM */	return 270;
    case 0421: /* ANDCMI */	return 270;
    case 0422: /* ANDCMM */	return 270;
    case 0423: /* ANDCMB */	return 270;
    case 0424: /* SETA */	return 270;
    case 0425: /* SETAI */	return 270;
    case 0426: /* SETAM */	return 270;
    case 0427: /* SETAB */	return 270;
    case 0430: /* XOR */	return 270;
    case 0431: /* XORI */	return 270;
    case 0432: /* XORM */	return 270;
    case 0433: /* XORB */	return 270;
    case 0434: /* OR */		return 270;
    case 0435: /* ORI */	return 270;
    case 0436: /* ORM */	return 270;
    case 0437: /* ORB */	return 270;
    case 0440: /* ANDCB */	return 620;
    case 0441: /* ANDCBI */	return 620;
    case 0442: /* ANDCBM */	return 620;
    case 0443: /* ANDCBB */	return 620;
    case 0444: /* EQV */	return 270;
    case 0445: /* EQVI */	return 270;
    case 0446: /* EQVM */	return 270;
    case 0447: /* EQVB */	return 270;
    case 0450: /* SETCA */	return 270;
    case 0451: /* SETCAI */	return 270;
    case 0452: /* SETCAM */	return 270;
    case 0453: /* SETCAB */	return 270;
    case 0454: /* ORCA */	return 620;
    case 0455: /* ORCAI */	return 620;
    case 0456: /* ORCAM */	return 620;
    case 0457: /* ORCAB */	return 620;
    case 0460: /* SETCM */	return 270;
    case 0461: /* SETCMI */	return 270;
    case 0462: /* SETCMM */	return 270;
    case 0463: /* SETCMB */	return 270;
    case 0464: /* ORCM */	return 270;
    case 0465: /* ORCMI */	return 270;
    case 0466: /* ORCMM */	return 270;
    case 0467: /* ORCMB */	return 270;
    case 0470: /* ORCB */	return 620;
    case 0471: /* ORCBI */	return 620;
    case 0472: /* ORCBM */	return 620;
    case 0473: /* ORCBB */	return 620;
    case 0474: /* SETO */	return 270;
    case 0475: /* SETOI */	return 270;
    case 0476: /* SETOM */	return 270;
    case 0477: /* SETOB */	return 270;
    case 0500: /* HLL */	return 270;
    case 0501: /* HLLI */	return 270;
    case 0502: /* HLLM */	return 270;
    case 0503: /* HLLS */	return 270;
    case 0504: /* HRL */	return 620;
    case 0505: /* HRLI */	return 620;
    case 0506: /* HRLM */	return 270;
    case 0507: /* HRLS */	return 270;
    case 0510: /* HLLZ */	return 270;
    case 0511: /* HLLZI */	return 270;
    case 0512: /* HLLZM */	return 270;
    case 0513: /* HLLZS */	return 270;
    case 0514: /* HRLZ */	return 270;
    case 0515: /* HRLZI */	return 270;
    case 0516: /* HRLZM */	return 270;
    case 0517: /* HRLZS */	return 270;
    case 0520: /* HLLO */	return 270;
    case 0521: /* HLLOI */	return 270;
    case 0522: /* HLLOM */	return 270;
    case 0523: /* HLLOS */	return 270;
    case 0524: /* HRLO */	return 270;
    case 0525: /* HRLOI */	return 270;
    case 0526: /* HRLOM */	return 270;
    case 0527: /* HRLOS */	return 270;
    case 0530: /* HLLE */	return 270;
    case 0531: /* HLLEI */	return 270;
    case 0532: /* HLLEM */	return 270;
    case 0533: /* HLLES */	return 270;
    case 0534: /* HRLE */	return 270;
    case 0535: /* HRLEI */	return 270;
    case 0536: /* HRLEM */	return 270;
    case 0537: /* HRLES */	return 270;
    case 0540: /* HRR */	return 270;
    case 0541: /* HRRI */	return 270;
    case 0542: /* HRRM */	return 270;
    case 0543: /* HRRS */	return 270;
    case 0544: /* HLR */	return 620;
    case 0545: /* HLRI */	return 620;
    case 0546: /* HLRM */	return 270;
    case 0547: /* HLRS */	return 270;
    case 0550: /* HRRZ */	return 270;
    case 0551: /* HRRZI */	return 270;
    case 0552: /* HRRZM */	return 270;
    case 0553: /* HRRZS */	return 270;
    case 0554: /* HLRZ */	return 270;
    case 0555: /* HLRZI */	return 270;
    case 0556: /* HLRZM */	return 270;
    case 0557: /* HLRZS */	return 270;
    case 0560: /* HRRO */	return 270;
    case 0561: /* HRROI */	return 270;
    case 0562: /* HRROM */	return 270;
    case 0563: /* HRROS */	return 270;
    case 0564: /* HLRO */	return 270;
    case 0565: /* HLROI */	return 270;
    case 0566: /* HLROM */	return 270;
    case 0567: /* HLROS */	return 270;
    case 0570: /* HRRE */	return 270;
    case 0571: /* HRREI */	return 270;
    case 0572: /* HRREM */	return 270;
    case 0573: /* HRRES */	return 270;
    case 0574: /* HLRE */	return 270;
    case 0575: /* HLREI */	return 270;
    case 0576: /* HLREM */	return 270;
    case 0577: /* HLRES */	return 270;
    case 0600: /* TRN */	return 620;
    case 0601: /* TLN */	return 620;
    case 0602: /* TRNE */	return 620;
    case 0603: /* TLNE */	return 620;
    case 0604: /* TRNA */	return 620;
    case 0605: /* TLNA */	return 620;
    case 0606: /* TRNN */	return 620;
    case 0607: /* TLNN */	return 620;
    case 0610: /* TDN */	return 620;
    case 0611: /* TSN */	return 620;
    case 0612: /* TDNE */	return 620;
    case 0613: /* TSNE */	return 620;
    case 0614: /* TDNA */	return 620;
    case 0615: /* TSNA */	return 620;
    case 0616: /* TDNN */	return 620;
    case 0617: /* TSNN */	return 620;
    case 0620: /* TRZ */	return 620;
    case 0621: /* TLZ */	return 620;
    case 0622: /* TRZE */	return 620;
    case 0623: /* TLZE */	return 620;
    case 0624: /* TRZA */	return 620;
    case 0625: /* TLZA */	return 620;
    case 0626: /* TRZN */	return 620;
    case 0627: /* TLZN */	return 620;
    case 0630: /* TDZ */	return 620;
    case 0631: /* TSZ */	return 620;
    case 0632: /* TDZE */	return 620;
    case 0633: /* TSZE */	return 620;
    case 0634: /* TDZA */	return 620;
    case 0635: /* TSZA */	return 620;
    case 0636: /* TDZN */	return 620;
    case 0637: /* TSZN */	return 620;
    case 0640: /* TRC */	return 620;
    case 0641: /* TLC */	return 620;
    case 0642: /* TRCE */	return 620;
    case 0643: /* TLCE */	return 620;
    case 0644: /* TRCA */	return 620;
    case 0645: /* TLCA */	return 620;
    case 0646: /* TRCN */	return 620;
    case 0647: /* TLCN */	return 620;
    case 0650: /* TDC */	return 620;
    case 0651: /* TSC */	return 620;
    case 0652: /* TDCE */	return 620;
    case 0653: /* TSCE */	return 620;
    case 0654: /* TDCA */	return 620;
    case 0655: /* TSCA */	return 620;
    case 0656: /* TDCN */	return 620;
    case 0657: /* TSCN */	return 620;
    case 0660: /* TRO */	return 620;
    case 0661: /* TLO */	return 620;
    case 0662: /* TROE */	return 620;
    case 0663: /* TLOE */	return 620;
    case 0664: /* TROA */	return 620;
    case 0665: /* TLOA */	return 620;
    case 0666: /* TRON */	return 620;
    case 0667: /* TLON */	return 620;
    case 0670: /* TDO */	return 620;
    case 0671: /* TSO */	return 620;
    case 0672: /* TDOE */	return 620;
    case 0673: /* TSOE */	return 620;
    case 0674: /* TDOA */	return 620;
    case 0675: /* TSOA */	return 620;
    case 0676: /* TDON */	return 620;
    case 0677: /* TSON */	return 620;
    }

  return -1000000;
}

int
timing_ka10 (word_t instruction)
{
  int opcode = OPCODE (instruction);
  int fast_registers = 1;
  int user_mode = 1;
  int nanos = 0;

#if 0 /* Instruction fetch overlapped with previous instruction. */
  nanos += 170;
  if (user_mode)
    nanos += 110;
  nanos += memory_read_access ();
  nanos += 170;
#endif

  if (X (instruction))
    {
      if (!fast_registers)
	{
	  nanos += 140;
	  nanos += memory_read_access ();
	  nanos += 40;
	}
      nanos += 280;
    }

  nanos += 90;

  if (I (instruction))
    {
      nanos += 170;
      if (user_mode)
	nanos += 110;
      nanos += memory_read_access ();
      nanos += 170;
      /* FIXME: this is simplified: no indexing, no indirect bit, fast regs */
      nanos += 90;
    }

  nanos += 20;

  if (opcode == 0)
    return -1;
  if ((opcode & 0700) == 0700) /* IO instructions */
    return -1;
  if (opcode == 0251) /* BLT */
    return -1;

  if (memory_read (instruction) ||
      memory_read_modify_write (instruction))
    {
      if (memory_read (instruction))
	nanos += 170;
      else
	nanos += 260;
      if (user_mode)
	nanos += 110;
      nanos += memory_read_access ();
      nanos += 160;
    }
  else if (floating_point_immediate (instruction))
    {
      nanos += 120;
    }
  else /* other immediates or no memory operand */
    {
      nanos += 30;
    }

  if (accumulator_read (instruction))
    {
      nanos += 140;
      if (!fast_registers)
	{
	  nanos += memory_read_access();
	  nanos += 130;
	}
      switch (opcode)
	{
	case 0170: /* FDVL */
	case 0234: /* DIV */
	case 0244: /* ASHC */
	case 0245: /* ROTC */
	case 0246: /* LSHC */
	  if (fast_registers)
	    nanos += 220;
	  else
	    {
	      nanos += 250;
	      nanos += memory_read_access ();
	      nanos += 130;
	    }
	  break;
	case 0267: /* JRA */
	  nanos += 140;
	  /* FALLTHROUGH */
	case 0262: /* POP */
	case 0263: /* POPJ */
	  if (user_mode)
	    nanos += 110;
	  nanos += 250;
	  nanos += memory_read_access ();
	  nanos += 130;
	}
    }

  nanos += 30;

  nanos += instruction_execution (opcode);

  nanos += 30;

  if (accumulator_write (instruction))
    {
      if (!fast_registers)
	{
	  nanos += 170;
	  if (user_mode)
	    nanos += 110;
	  nanos += memory_write_access ();
	  nanos += 130;
	}
    }

  if (memory_read_modify_write (instruction))
    nanos += 260;
  else if (memory_write (instruction))
    {
      nanos += 170;
      if (user_mode)
	nanos += 110;
      nanos += memory_write_access ();
      nanos += 130;
    }

  return nanos;
}
