#include "dis.h"
#include "memory.h"
#include "cpu/cpu.h"

typedef long long word_t;

int PC;

/* User variables. */

static word_t option = 0;

static void read_or_write (int set, word_t *value, int addr)
{
  if (set)
    *value = read_memory (addr);
  else
    write_memory (addr, *value);
}

static void its_open (void)
{
  word_t x;
  char s[7];

  fprintf (stderr, ".OPEN %o\n", MA);
  x = read_memory (MA);
  sixbit_to_ascii (x & 0777777, s);
  fprintf (stderr, "Mode %06llo, device %s\n", x >> 18, s);
  x = read_memory (MA + 1);
  sixbit_to_ascii (x, s);
  fprintf (stderr, "File name %s ", s);
  x = read_memory (MA + 2);
  sixbit_to_ascii (x, s);
  fprintf (stderr, "%s\n", s);

  PC++;
}

static void suset (void)
{
  word_t spec;
  int set, var, addr;

  fprintf (stderr, ".SUSET %o\n", MA);

  spec = read_memory (MA);
  set = spec >> 35;
  spec &= ~0400000000000LL;

  if (spec & 0200000000000LL)
    {
      fprintf (stderr, "Block mode\n");
      exit (0);
    }

  var = (spec >> 18) & 0177777;
  addr = spec & 0777777;

  switch (var)
    {
    case 0054:
      fprintf (stderr, "OPTION\n");
      read_or_write (set, &option, addr);
      return;
    default:
      fprintf (stderr, "Unknown variable %o\n", var);
      exit (0);
    }

}

static void lose (void)
{
  extern int cpu_model;
  printf ("ERROR; %o>>\n", PC-2);
  disassemble_word (memory, read_memory (PC-2), -1, 4);
  exit (1);
}

static void eval (void)
{
  word_t symbol;
  char s[7];

  fprintf (stderr, ".EVAL %o,\n", AC);
  symbol = FM[AC];

  switch (symbol)
    {
    case 0027277540200LL: // TOIP
    case 0027275713420LL: // TOBBP
    case 0027275724720LL: // TOBEP
    case 0022445216000LL: // NCT
      FM[AC] = 01234;
      break;
    default:
      squoze_to_ascii (symbol, s);
      fprintf (stderr, "SQUOZE %012llo /%s/\n", symbol, s);
      exit (0);
      return;
    }

  PC++;
}

static void logout (void)
{
  switch (AC)
    {
    case 1:
      printf (":KILL\n");
      exit (0);
    default:
      fprintf (stderr, ".LOGOUT %d,\n", AC);
      exit (0);
    }
}

static void oper (void)
{
  switch (MA)
    {
    case 0:
      fprintf (stderr, ".OPER ILLEGAL\n");
      exit (1);
    case 1:
      fprintf (stderr, ".OPER .ITYI\n");
      exit (0);
    case 033:
      return logout ();
    case 073:
      return eval ();
      /*
	.OPER 0		illegal
	.OPER 1		.ITYI
	.OPER 2		.LISTEN
	.OPER 3		.SLEEP
	.OPER 4		.SETMSK
	.OPER 5		.SETM2
	.OPER 6		.DEMON
	.OPER 7		.CLOSE
	.OPER 10	.UCLOSE
	.OPER 11	.ATTY
	.OPER 12	.DTTY
	.OPER 13	.IOPUSH
	.OPER 14	.IOPOP
	.OPER 15	.DCLOSE
	.OPER 16	.DSTOP
	.OPER 17	.RDTIME
	.OPER 20	.RDSW
	.OPER 21	.GUN
	.OPER 22	.UDISMT
	.OPER 23	.GETSYS
	.OPER 24	.IPDP
	.OPER 25	.GETLOC
	.OPER 26	.SETLOC
	.OPER 27	.DISOWN
	.OPER 30	.DWORD
	.OPER 31	.DSTEP
	.OPER 32	.GENSYM
	.OPER 33	.LOGOUT
	.OPER 34	.REALT
	.OPER 35	.WSNAME
	.OPER 36	.UPISET
	.OPER 37	.RESET
	.OPER 40	.ARMOVE
	.OPER 41	.DCONT
	.OPER 42	.CBLK
	.OPER 43	.ASSIGN
	.OPER 44	.DESIGN
	.OPER 45	.RTIME
	.OPER 46	.RDATE
	.OPER 47	.HANG
	.OPER 50	.EOFC
	.OPER 51	.IOTLSR
	.OPER 52	.RSYSI
	.OPER 53	.SUPSET
	.OPER 54	.PDTIME
	.OPER 55	.ARMRS
	.OPER 56	.UBLAT
	.OPER 57	.IOPDL
	.OPER 60	.ITYIC
	.OPER 61	.MASTER
	.OPER 62	.VSTST
	.OPER 63	.NETAC
	.OPER 64	.NETS
	.OPER 65	.REVIVE
	.OPER 66	.DIETIME
	.OPER 67	.SHUTDN
	.OPER 70	.ARMOFF
	.OPER 71	.NDIS
	.OPER 72	.FEED
	.OPER 73	.EVAL
	.OPER 74	.REDEF
	.OPER 75	.IFSET
	.OPER 76	.UTNAM
	.OPER 77	.UINIT
	.OPER 100	.RYEAR
	.OPER 101	.RLPDTM
	.OPER 102	.RDATIM
	.OPER 103	.RCHST
	.OPER 104	.RBTC
	.OPER 105	.DMPCH
	.OPER 106	.SWAP
	.OPER 107	.MTAPE
	.OPER 110	.GENNUM
	.OPER 111	.NETINT
       */
    default:
      fprintf (stderr, ".OPER %o\n", MA);
      exit (0);
    }
}

static void not_call (void)
{
  switch (AC)
    {
      /*
		.CALL 1,		.DISMISS
		.CALL 2,		.LOSE
		.CALL 3,		.TRANAD
		.CALL 4,		.VALUE
		.CALL 5,		.UTRAN
		.CALL 6,		.CORE
		.CALL 7,		.TRANDL
		.CALL 10,		.DSTART
		.CALL 11,		.FDELE
		.CALL 12,		.DSTRT
		.CALL 13,		.SUSET
		.CALL 14,		.LTPEN
		.CALL 15,		.VSCAN
		.CALL 16,		.POTSET
		.CALL 17,		unused
      */
    case 002:
      return lose ();
    case 013:
      return suset ();
    default:
      fprintf (stderr, ".CALL %o,\n", AC);
      exit (0);
    }
}

static void call_sstatu (void)
{
  fprintf (stderr, ".CALL SSTATU\n");
  PC++;
}

static void call_corblk (void)
{
  fprintf (stderr, ".CALL CORBLK\n");
  PC++;
}

static void call (void)
{
  word_t x;
  char s[7];

  if (AC != 0)
    return not_call ();

  x = read_memory (MA++);
  if (x != 0400000000000)
    {
      fprintf (stderr, "Illegal .CALL\n");
      exit (0);
    }

  x = read_memory (MA++);
  switch (x)
    {
    case 0435762425453LL:
      call_corblk ();
      break;
    case 0636364416465LL:
      call_sstatu ();
      break;
    default:
      sixbit_to_ascii (x, s);
      fprintf (stderr, ".CALL %s (%012llo)\n", s, x);
      exit (0);
    }
}

int its_muuo (int pc)
{
  PC = pc;
  switch (IR >> 27)
    {
    case 040:
      fprintf (stderr, ".IOT\n");
      //fprintf (stderr, ".");
      return PC;  //  exit (0);
    case 041:
      its_open ();
      return PC;
    case 042:
      oper ();
      return PC;
    case 043:
      call ();
      return PC;
    case 044:
      fprintf (stderr, ".USET\n");
      exit (0);
    case 045:
      fprintf (stderr, ".BREAK\n");
      exit (0);
    case 046:
      fprintf (stderr, ".STATUS\n");
      exit (0);
    case 047:
      fprintf (stderr, ".ACCESS\n");
      exit (0);
    default:
      fprintf (stderr, "Unknown MUUO\n");
      exit (0);
    }
  return PC;
}
