/* Just-in-time translating PDP-10 emulator.

   Entry points:

   pure_page (address) - Call this when a pure page has been loaded.

   unpure_page (address) - Call this when an unpure page has been loaded.

   unmapped_page (address) - Call this when there's an unmapped page.

   invalidate_word (address) - Call this when a word has been written.

   run (address) - Run the emulator.
*/

#include <unistd.h>
#include <sys/mman.h>
#include "memory.h"
#include "cpu/its.h"

#define THREADED 0

#if defined(__GNUC__) && defined (__i386__)
#define REGPARM __attribute__((regparm(1)))
#else
#define REGPARM
#endif

typedef long long word_t;
static int REGPARM calculate_ea (int);
void invalidate_word (int a);
#if THREADED
static int execute (int);
#endif

#define DEBUG(X) fprintf X
#define TODO(X) DEBUG((stderr, "TODO: %s\n", #X)); exit (1)

#define SIGN_EXTEND(X)                          \
  if (X & 0400000000000LL)                      \
    X |= -1LL << 36;

/* Machine state. */

int MA;
int AC;
//int PC;
int flags;
word_t IR;
word_t AR;
word_t BR;
word_t MQ;
word_t MB;
word_t FM[16];
struct pdp10_memory *memory;

word_t read_memory (int address)
{
  word_t data;

  address &= 0777777;
  if (address < 16)
    return FM[address];

  data = get_word_at (memory, address);
  DEBUG((stderr, "READ %06o %012llo\n", address, data));
  return data;
}

void write_memory (int address, word_t data)
{
  address &= 0777777;
  DEBUG((stderr, "WRITE %06o %012llo\n", address, data));
  invalidate_word (address);
  if (address < 16)
    FM[address] = data;
  else
    set_word_at (memory, address, data);
}

/* Every word is decoded into three uops.  The first reads the
   operands, the second is the operation, and the third writes back
   the result. */

typedef int REGPARM (*uop) (int);

#if THREADED
#define UOPSIZE 3
static uop ops[UOPSIZE*MOBY];
typedef uop *upc_t;
#else
#define UOPSIZE 4*5
static unsigned char ops[UOPSIZE*MOBY];
typedef unsigned char *upc_t;
#endif


static int REGPARM uop_nop (int PC)
{
  return PC;
}

static int REGPARM uop_read_immediate (int PC)
{
  AR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
  PC++;
  return PC;
}

static int REGPARM uop_read_memory (int PC)
{
  AR = read_memory (MA);
  BR = 0;
  if (AR == 0777700000000LL)
    AR = -010000LL << 18;
  DEBUG((stderr, "AR %012llo\n", AR));
  PC++;
  return PC;
}

static int REGPARM uop_read_ac (int PC)
{
  AR = FM[AC];
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
  PC++;
  return PC;
}

static int REGPARM uop_read_ac_immediate (int PC)
{
  AR = FM[AC];
  BR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "BR %012llo\n", BR));
  PC++;
  return PC;
}

static int REGPARM uop_read_both (int PC)
{
  AR = FM[AC];
  BR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "AC %o, BR %012llo\n", AC, BR));
  PC++;
  return PC;
}

static int REGPARM uop_write_ac (int PC)
{
  DEBUG((stderr, "AC%o %012llo\n", AC, AR));
  FM[AC] = AR;
  return PC;
}

static int REGPARM uop_write_ac1 (int PC)
{
  DEBUG((stderr, "AC%o %012llo\n", AC + 1, MQ));
  FM[AC+1] = MQ;
  return PC;
}

static int REGPARM uop_write_acnz (int PC)
{
  if (AC != 0)
    FM[AC] = AR;
  return PC;
}

static int REGPARM uop_write_mem (int PC)
{
  write_memory (MA, AR);
  return PC;
}

static int REGPARM uop_write_both (int PC)
{
  FM[AC] = AR;
  write_memory (MA, AR);
  return PC;
}

static int REGPARM uop_write_ac_mem (int PC)
{
  FM[AC] = AR;
  write_memory (MA, BR);
  return PC;
}

static int REGPARM uop_write_same (int PC)
{
  if (AC != 0)
    FM[AC] = AR;
  write_memory (MA, AR);
  return PC;
}

/* Table to decode an opcode into a read uop. */
#define RDNO uop_read_immediate
#define RDMA uop_read_memory
#define RDAA uop_read_ac
#define RDAB uop_read_ac_immediate
#define RDAM 0
#define RDA2 0
#define RDB1 uop_read_both
#define RDB2 uop_read_both
#define RDB3 0
#define RDB4 0

static uop read_operands[] = {
  /* 000-077 */
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  /* 100-177 */
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, // DFAD etc 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, // DMOVE etc
  RDB1, RDB2, RDAA, RDNO, RDNO, RDNO, RDNO, RDNO, // UFA etc
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FAD
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FSB
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FMP
  RDB1, RDB4, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FDV
  /* 200-277 */
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // MOVE, MOVS
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // MOVN, MOVM
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // IMUL, MUL
  RDB2, RDAA, RDB2, RDB2, RDB3, RDA2, RDB3, RDB3, // IDIV, DIV
  RDAA, RDAA, RDAA, RDAA, RDA2, RDA2, RDA2, RDNO, // ASH etc
  RDB2, RDAA, RDAA, RDAA, RDNO, RDNO, RDNO, RDNO, // EXCH etc
  RDAA, RDB2, RDAA, RDAA, RDNO, RDNO, RDAB, RDNO, // PUSHJ etc
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // ADD, SUB
  /* 300-377 */
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // CAI
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // CAM
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, // JUMP
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, // SKIP
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, // AOJ
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, // AOS
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, // SOJ
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, // SOS
  /* 400-477 */
  RDNO, RDNO, RDNO, RDNO, RDB1, RDAB, RDB1, RDB1, // SETZ, AND
  RDB1, RDAB, RDB1, RDB1, RDMA, RDNO, RDNO, RDMA, // ANDCA, SETM
  RDB1, RDAB, RDB1, RDB1, RDAB, RDAB, RDAB, RDAB, // ANDCM, SETA
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // XOR, IOR
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // ANDCB, EQV
  RDAB, RDAB, RDAB, RDAB, RDB1, RDAB, RDB1, RDB1, // SETCA, ORCA
  RDMA, RDNO, RDMA, RDMA, RDB1, RDAB, RDB1, RDB1, // SETCM, ORCM
  RDB1, RDAB, RDB1, RDB1, RDNO, RDNO, RDNO, RDNO, // ORCB, SETO
  /* 500-577 */
  RDB1, RDAB, RDB2, RDMA, RDB1, RDAB, RDB2, RDMA, // HLL, HRL
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLZ, HRLZ
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLO, HRLO
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLE, HRLE
  RDB1, RDAB, RDB2, RDMA, RDB1, RDAB, RDB2, RDMA, // HLL, HRL
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLZ, HRLZ
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLO, HRLO
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, // HLLE, HRLE
  /* 600-677 */
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // TxN
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // TxN
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // TxZ
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // TxZ
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // TxC
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // TxC
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // TxO
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // TxO
  /* 700-777 */
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
};

/* Table to decode an opcode into a write back uop. */
#define WRNO uop_nop
#define WRM1 uop_write_mem
#define WRA1 uop_write_ac
#define WRA2 uop_write_ac1
#define WRA0 uop_write_acnz
#define WRA3 0
#define WRB1 uop_write_both
#define WRB2 uop_write_same
#define WRB3 0
#define WRB4 uop_write_ac_mem

static uop write_back[] = {
  /* 000-077 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  /* 100-177 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRA1, WRA1, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  /* 200-277 */
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // MOVE, MOVS
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // MOVN, MOVM
  WRA1, WRA1, WRM1, WRB1, WRA3, WRA3, WRM1, WRB3, 
  WRA3, WRA3, WRM1, WRB3, WRA3, WRA3, WRM1, WRB3, 
  WRA1, WRA1, WRA1, WRNO, WRA3, WRA3, WRA3, WRNO, 
  WRB4, WRNO, WRA1, WRA1, WRNO, WRNO, WRNO, WRNO, // EXCH etc
  WRA1, WRA1, WRA1, WRA1, WRNO, WRA1, WRM1, WRNO, // PUSHJ etc
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  /* 300-377 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // CAI
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // CAM
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // JUMP
  WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, // SKIP
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, WRB2, 
  /* 400-477 */
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRNO, WRA1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  /* 500-577 */
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  /* 600-677 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, 
  /* 700-777 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
};

static int REGPARM uop_luuo (int PC)
{
  word_t x = IR & 0777740000000LL;
  write_memory (040, x + MA);
  // XCT 41; don't change PC.
  TODO(LUUO);
  return PC;
}

static int REGPARM uop_muuo (int PC)
{
  PC = its_muuo (PC);
  return PC;
}

static int REGPARM uop_move (int PC)
{
  DEBUG((stderr, "MOVE\n"));
  // TODO: flags
  return PC;
}

static int REGPARM uop_movs (int PC)
{
  DEBUG((stderr, "MOVS\n"));
  AR = ((AR >> 18) & 0777777) | ((AR & 0777777) << 18);
  DEBUG((stderr, "AR %012llo\n", AR));
  return PC;
}

static int REGPARM uop_movn (int PC)
{
  DEBUG((stderr, "MOVN\n"));
  AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
  return PC;
}

static int REGPARM uop_movm (int PC)
{
  DEBUG((stderr, "MOVM\n"));
  if (AR & 0400000000000LL)
    AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
  return PC;
}

static int REGPARM uop_ash (int PC)
{
  DEBUG((stderr, "ASH\n"));
  return PC;
}

static int REGPARM uop_rot (int PC)
{
  TODO(ROT);
  return PC;
}

static int REGPARM uop_lsh (int PC)
{
  DEBUG((stderr, "LSH\n"));
  if (MA & 0400000000000LL)
    AR >>= MA;
  else
    AR <<= MA;
  return PC;
}

static int REGPARM uop_jffo (int PC)
{
  TODO(JFFO);
  return PC;
}

static int REGPARM uop_ashc (int PC)
{
  TODO(ASHC);
  return PC;
}

static int REGPARM uop_rotc (int PC)
{
  TODO(ROTC);
  return PC;
}

static int REGPARM uop_lshc (int PC)
{
  TODO(LSHC);
  return PC;
}

static int REGPARM uop_exch (int PC)
{
  DEBUG((stderr, "EXCH\n"));
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  MQ = AR;
  AR = BR;
  BR = MQ;
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  return PC;
}

static int REGPARM uop_blt (int PC)
{
  TODO(BLT);
  return PC;
}

static int REGPARM uop_aobjp (int PC)
{
  TODO(AOBJP);
  return PC;
}

static int REGPARM uop_aobjn (int PC)
{
  word_t LH = AR & 0777777000000LL;
  DEBUG((stderr, "AOBJN\n"));
  LH += 01000000LL;
  AR = LH | ((AR + 1) & 0777777LL);
  if (LH == 01000000LL)
    LH = 0;
  else
    PC = MA;
  return PC;
}

static int REGPARM uop_jrst (int PC)
{
  DEBUG((stderr, "JRST\n"));
  if (AC & 14)
    return uop_muuo (PC);
  if (AC & 2)
    flags = MB >> 18;
  PC = MA;
  return PC;
}

static int REGPARM uop_jfcl (int PC)
{
  TODO(JFCL);
  return PC;
}

static int REGPARM uop_xct (int PC)
{
  TODO(XCT);
  // Don't set PC to target instruction.
  // Or do, and work around it.
  return PC;
}

static int REGPARM uop_pushj (int PC)
{
  DEBUG((stderr, "PUSHJ\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, (flags << 18) | PC);
  PC = MA;
  return PC;
}

static int REGPARM uop_push (int PC)
{
  DEBUG((stderr, "PUSH\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, BR);
  return PC;
}

static int REGPARM uop_popj (int PC)
{
  word_t x;
  DEBUG((stderr, "POPJ\n"));
  DEBUG((stderr, "AR %012llo\n", AR));
  x = read_memory (AR);
  PC = x & 0777777;
  flags = (x >> 18) & 0777777;
  DEBUG((stderr, "PC %06o\n", PC));
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
  return PC;
}

static int REGPARM uop_pop (int PC)
{
  DEBUG((stderr, "POP\n"));
  MB = read_memory (AR);
  write_memory (MA, MB);
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
  return PC;
}

static int REGPARM uop_jsr (int PC)
{
  DEBUG((stderr, "JSR\n"));
  write_memory (AR, (flags << 18) | PC);
  PC = AR + 1;
  return PC;
}

static int REGPARM uop_jsp (int PC)
{
  DEBUG((stderr, "JSP\n"));
  AR = (flags << 18) | PC;
  PC = MA;
  return PC;
}

static int REGPARM uop_jsa (int PC)
{
  TODO(JSA);
  return PC;
}

static int REGPARM uop_jra (int PC)
{
  TODO(JRA);
  return PC;
}

static int REGPARM uop_add (int PC)
{
  DEBUG((stderr, "ADD\n"));
  AR = AR + BR;
  return PC;
}

static int REGPARM uop_sub (int PC)
{
  DEBUG((stderr, "SUB\n"));
  AR = AR - BR;
  return PC;
}

static int REGPARM uop_setz (int PC)
{
  DEBUG((stderr, "SETZ\n"));
  AR = 0;
  return PC;
}

static int REGPARM uop_and (int PC)
{
  DEBUG((stderr, "AND\n"));
  AR &= BR;
  return PC;
}

static int REGPARM uop_andca (int PC)
{
  DEBUG((stderr, "ANDCA\n"));
  AR = BR & (~AR & 0777777777777LL);
  return PC;
}

static int REGPARM uop_setm (int PC)
{
  DEBUG((stderr, "SETM\n"));
  AR = BR;
  return PC;
}

static int REGPARM uop_andcm (int PC)
{
  DEBUG((stderr, "ANDCM\n"));
  AR &= ~BR & 0777777777777LL;
  return PC;
}

static int REGPARM uop_seta (int PC)
{
  DEBUG((stderr, "SETA\n"));
  return PC;
}

static int REGPARM uop_xor (int PC)
{
  DEBUG((stderr, "XOR\n"));
  AR ^= BR;
  return PC;
}

static int REGPARM uop_ior (int PC)
{
  DEBUG((stderr, "IOR\n"));
  AR |= BR;
  return PC;
}

static int REGPARM uop_andcb (int PC)
{
  DEBUG((stderr, "ANDCB\n"));
  AR = (~AR & 0777777777777LL) & (~BR & 0777777777777LL);
  return PC;
}

static int REGPARM uop_eqv (int PC)
{
  DEBUG((stderr, "EQV\n"));
  AR = ~(AR ^ BR) & 0777777777777LL;
  return PC;
}

static int REGPARM uop_setca (int PC)
{
  DEBUG((stderr, "SETCA\n"));
  AR = ~AR & 0777777777777LL;
  return PC;
}

static int REGPARM uop_orca (int PC)
{
  DEBUG((stderr, "ORCA\n"));
  AR = BR | (~AR & 0777777777777LL);
  return PC;
}

static int REGPARM uop_setcm (int PC)
{
  DEBUG((stderr, "SETCM\n"));
  AR = ~BR & 0777777777777LL;
  return PC;
}

static int REGPARM uop_orcm (int PC)
{
  DEBUG((stderr, "ORCM\n"));
  AR |= ~BR & 0777777777777LL;
  return PC;
}

static int REGPARM uop_orcb (int PC)
{
  DEBUG((stderr, "ORCB\n"));
  AR = (~AR & 0777777777777LL) | (~BR & 0777777777777LL);
  return PC;
}

static int REGPARM uop_seto (int PC)
{
  DEBUG((stderr, "SETO\n"));
  AR = 0777777777777LL;
  return PC;
}

static int REGPARM uop_skipl (int PC)
{
  DEBUG((stderr, "SKIPL: %012llo < %012llo\n", AR, BR));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR < BR)
    PC++;
  return PC;
}

static int REGPARM uop_skipe (int PC)
{
  DEBUG((stderr, "SKIPE\n"));
  if (AR == BR)
    PC++;
  return PC;
}

static int REGPARM uop_skiple (int PC)
{
  DEBUG((stderr, "SKIPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR <= BR)
    PC++;
  return PC;
}

static int REGPARM uop_skipa (int PC)
{
  DEBUG((stderr, "SKIPA\n"));
  PC++;
  return PC;
}

static int REGPARM uop_skipge (int PC)
{
  DEBUG((stderr, "SKIPGE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR >= BR)
    PC++;
  return PC;
}

static int REGPARM uop_skipn (int PC)
{
  DEBUG((stderr, "SKIPN\n"));
  if (AR != BR)
    PC++;
  return PC;
}

static int REGPARM uop_skipg (int PC)
{
  DEBUG((stderr, "SKIPG\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR > BR)
    PC++;
  return PC;
}

static int REGPARM uop_jumpl (int PC)
{
  DEBUG((stderr, "JUMPL: %012llo < 0\n", AR));
  SIGN_EXTEND (AR);
  if (AR < 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_jumpe (int PC)
{
  DEBUG((stderr, "JUMPE\n"));
  if (AR == 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_jumple (int PC)
{
  DEBUG((stderr, "JUMPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR <= 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_jumpa (int PC)
{
  DEBUG((stderr, "JUMPA\n"));
  PC = MA;
  return PC;
}

static int REGPARM uop_jumpge (int PC)
{
  DEBUG((stderr, "JUMPGE\n"));
  SIGN_EXTEND (AR);
  if (AR >= 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_jumpn (int PC)
{
  DEBUG((stderr, "JUMPN\n"));
  if (AR != 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_jumpg (int PC)
{
  DEBUG((stderr, "JUMPG\n"));
  SIGN_EXTEND (AR);
  if (AR > 0)
    PC = MA;
  return PC;
}

static int REGPARM uop_tlo (int PC)
{
  DEBUG((stderr, "TLO\n"));
  AR |= BR;
  return PC;
}

static int REGPARM uop_hll (int PC)
{
  DEBUG((stderr, "HLL\n"));
  AR = (BR & 0777777000000LL) | (AR & 0777777);
  return PC;
}

static int REGPARM uop_hrl (int PC)
{
  DEBUG((stderr, "HRL\n"));
  AR = ((BR & 0777777LL) << 18) | (AR & 0777777);
  return PC;
}

static int REGPARM uop_hlr (int PC)
{
  DEBUG((stderr, "HLR\n"));
  AR = (AR & 0777777000000LL) | ((BR >> 18) & 0777777);
  return PC;
}

static int REGPARM uop_hrr (int PC)
{
  DEBUG((stderr, "HRR\n"));
  AR = (AR & 0777777000000LL) | (BR & 0777777);
  return PC;
}

/* Table to decode an opcode into an operation uop. */
static uop operate[] = {
  /* 000-077 */
  uop_muuo, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  uop_muuo, uop_muuo, uop_muuo, uop_muuo, uop_muuo, uop_muuo, uop_muuo, uop_muuo, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 100-177 */
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 200-277 */
  uop_move, uop_move, uop_move, uop_move, uop_movs, uop_movs, uop_movs, uop_movs,
  uop_movn, uop_movn, uop_movn, uop_movn, uop_movm, uop_movm, uop_movm, uop_movm,
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  uop_ash, uop_rot, uop_lsh, uop_jffo, uop_ashc, uop_rotc, uop_lshc, 0, 
  uop_exch, uop_blt, uop_aobjp, uop_aobjn, uop_jrst, uop_jfcl, uop_xct, 0, 
  uop_pushj, uop_push, uop_pop, uop_popj, uop_jsr, uop_jsp, uop_jsa, uop_jra,
  uop_add, uop_add, uop_add, uop_add, uop_sub, uop_sub, uop_sub, uop_sub,
  /* 300-377 */
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg,
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg,
  uop_nop, uop_jumpl, uop_jumpe, uop_jumple, uop_jumpa, uop_jumpge, uop_jumpn, uop_jumpg,
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // SKIP
  0, 0, 0, 0, 0, 0, 0, 0, // AOJ
  0, 0, 0, 0, 0, 0, 0, 0, // AOS
  0, 0, 0, 0, 0, 0, 0, 0, // SOJ 
  0, 0, 0, 0, 0, 0, 0, 0, // SOS
  /* 400-477 */
  uop_setz, uop_setz, uop_setz, uop_setz, uop_and, uop_and, uop_and, uop_and, 
  uop_andca, uop_andca, uop_andca, uop_andca, uop_setm, uop_setm, uop_setm, uop_setm, 
  uop_andcm, uop_andcm, uop_andcm, uop_andcm, uop_seta, uop_seta, uop_seta, uop_seta, 
  uop_xor, uop_xor, uop_xor, uop_xor, uop_ior, uop_ior, uop_ior, uop_ior, 
  uop_andcb, uop_andcb, uop_andcb, uop_andcb, uop_eqv, uop_eqv, uop_eqv, uop_eqv, 
  uop_setca, uop_setca, uop_setca, uop_setca, uop_orca, uop_orca, uop_orca, uop_orca, 
  uop_setcm, uop_setcm, uop_setcm, uop_setcm, uop_orcm, uop_orcm, uop_orcm, uop_orcm, 
  uop_orcb, uop_orcb, uop_orcb, uop_orcb, uop_seto, uop_seto, uop_seto, uop_seto, 
  /* 500-577 */

  /* HLL,  HLLI,  HLLM,  HLLS       HRL,  HRLI,  HRLM,  HRLS */
  /* HLLZ, HLLZI, HLLZM, HLLZS      HRLZ, HRLZI, HRLZM, HRLZS */
  /* HLLO, HLLOI, HLLOM, HLLOS      HRLO, HRLOI, HRLOM, HRLOS */
  /* HLLE, HLLEI, HLLEM, HLLES      HRLE, HRLEI, HRLEM, HRLES */

  /* HRR,  HRRI,  HRRM,  HRRS       HLR,  HLRI,  HLRM,  HLRS */
  /* HRRZ, HRRZI, HRRZM, HRRZS      HLRZ, HLRZI, HLRZM, HLRZS */
  /* HRRO, HRROI, HRROM, HRROS      HLRO, HLROI, HLROM, HLROS */
  /* HRRE, HRREI, HRREM, HLLES      HLRE, HLREI, HLREM, HLRES */

  uop_hll, uop_hll, uop_hll, uop_hll, uop_hrl, uop_hrl, uop_hrl, uop_hll,
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  uop_hrr, uop_hrr, uop_hrr, uop_hrr, uop_hlr, uop_hlr, uop_hlr, uop_hrr,
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 600-677 */
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, uop_tlo, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 700-777 */
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
};

/* Table to decode an IO instruction into a uop. */
#define CONI 0
#define CONO 0
#define DATAI 0
#define DATAO 0
static uop iot[] = {
  CONI, CONO, DATAI, DATAO
};

#if !THREADED
static void write_call (upc_t *upc, uop op)
{
  size_t a = (size_t)op;
  a -= (size_t)*upc;
  a -= 5;
  **upc = 0xE8; (*upc)++;
  **upc = a; (*upc)++;
  **upc = a >> 8; (*upc)++;
  **upc = a >> 16; (*upc)++;
  **upc = a >> 24; (*upc)++;
}

static void write_jump (upc_t *upc, uop op)
{
  size_t a = (size_t)op;
  a -= (size_t)*upc;
  a -= 5;
  **upc = 0xE9; (*upc)++;
  **upc = a; (*upc)++;
  **upc = a >> 8; (*upc)++;
  **upc = a >> 16; (*upc)++;
  **upc = a >> 24; (*upc)++;
}

static void write_set_pc (upc_t *upc, int x)
{
  **upc = 0xB8; (*upc)++;
  **upc = x; (*upc)++;
  **upc = x >> 8; (*upc)++;
  **upc = x >> 16; (*upc)++;
  **upc = x >> 24; (*upc)++;
}

static void write_nop (upc_t *upc)
{
  // Recommended five byte NOP.
  **upc = 0x0F; (*upc)++;
  **upc = 0x1F; (*upc)++;
  **upc = 0x44; (*upc)++;
  **upc = 0x00; (*upc)++;
  **upc = 0x00; (*upc)++;

  // 90
  // 66 90
  // 0F 1F 00
  // 0F 1F 40 00
  // 0F 1F 44 00 00
  // 66 0F 1F 44 00 00
  // 0F 1F 80 00 00 00 00
  // 0F 1F 84 00 00 00 00 00
  // 66 0F 1F 84 00 00 00 00 00
}
#endif

static void write_uop (upc_t *upc, uop op)
{
#if THREADED
  **upc = op;
  (*upc)++;
#else
  if (op == uop_nop)
    write_nop (upc);
  else
    write_call (upc, op);
#endif
}

/* Decode one word. */
static void decode (int PC)
{
  upc_t upc = ops + UOPSIZE*PC;
  word_t opcode;
  IR = read_memory (PC);

  opcode = (IR >> 27) & 0777;
#if !THREADED
  if (1 && (IR & 0777777000000) == 0254000000000LL) {
    write_set_pc (&upc, IR & 0777777);
    write_jump (&upc, (uop)(ops + UOPSIZE*(IR & 0777777)));
    write_nop (&upc);
    write_nop (&upc);
    return;
  }
  write_uop (&upc, calculate_ea);
#endif
  write_uop (&upc, read_operands[opcode]);
  if (opcode == 0777)
    write_uop (&upc, iot[(IR >> 23) & 3]);
  else
    write_uop (&upc, operate[opcode]);
  write_uop (&upc, write_back[opcode]);
}

/* Retry executing the first uop after a decode operation. */
static int retry (int PC)
{
#if THREADED
  PC = ops[UOPSIZE*PC](PC);
#else
  upc_t upc = ops + UOPSIZE*PC;
  uop op = (uop)upc;
  PC = op (PC);
#endif
  return PC;
}

/* Uop to decode a single word and execute it. */
static int REGPARM decode_word (int PC)
{
  DEBUG((stderr, "Decode word\n"));
  decode (PC);
  PC = retry (PC);
  return PC;
}

/* Uop to decode a page and then resume execution. */
static int REGPARM decode_page (int PC)
{
  int save = PC;
  DEBUG((stderr, "Decode page\n"));
  PC &= 0776000;
  do {
    IR = read_memory (PC);
    decode (PC);
    PC++;
  } while ((PC & 01777) != 0);
  PC = save;
  PC = retry (PC);
  return PC;
}

/* Invalidate a single word. */
void invalidate_word (int a)
{
  upc_t upc = ops + UOPSIZE*a;
  write_uop (&upc, decode_word);
}

/* Fill a page with a uop. */
static void fill_page (int a, uop op)
{
  int i;
  upc_t upc = &ops[UOPSIZE*(a & 0776000)];
  for (i = 0; i < 02000; i++) {
    write_uop (&upc, op);
    write_uop (&upc, uop_nop);
    write_uop (&upc, uop_nop);
#if !THREADED
    write_uop (&upc, uop_nop);
#endif
  }
}

/* Fill an unpure page. */
void unpure_page (int a)
{
  DEBUG((stderr, "Unpure page: %06o.\n", a));
  fill_page (a, decode_word);
}

/* Fill an pure page. */
void pure_page (int a)
{
  fill_page (a, decode_page);
}

/* Uop for an unmapped word. */
static int REGPARM unmapped_word (int PC)
{
  DEBUG((stderr, "Unmapped word.\n"));
  exit (1);
  return PC;
}

/* Fill an unmapped page. */
void unmapped_page (int a)
{
  fill_page (a, unmapped_word);
}

static int REGPARM calculate_ea (int PC)
{
  int address, X, I;

  MB = IR = read_memory (PC);
  AC = (IR >> 23) & 017;
  DEBUG((stderr, "IR %012llo\n", IR));

  do {
    address = MB & 0777777;
    X = (MB >> 18) & 017;
    I = (MB >> 22) & 01;
    if (X)
      address += FM[X];
    if (I)
      MB = read_memory (address);
  } while (I);

  MA = address;
  DEBUG((stderr, "EA %06o\n", MA));
  return PC;
}

static void rwx (void)
{
  size_t length = sizeof ops;
  size_t start = (size_t)ops;
  long page_size = sysconf (_SC_PAGESIZE);
  start &= -page_size;
  length += page_size - 1;
  start &= -page_size;
  if (mprotect ((void *)start, length,
                PROT_READ | PROT_WRITE | PROT_EXEC))
    perror ("mprotect");
}

#if THREADED
/* Execute one instruction. */
static int execute (int PC)
{
  upc_t upc = ops + UOPSIZE*PC;
  DEBUG((stderr, "\nExecute %06o\n", PC));
  PC = calculate_ea (PC);
  PC = (*upc++) (PC);
  PC = (*upc++) (PC);
  PC = (*upc++) (PC);
  return PC;
}
#endif

/* Run. */
void run (int start, struct pdp10_memory *m)
{
  int PC = start;
  memory = m;
#if THREADED
  for (;;)
    PC = execute (PC);
#else
  rwx ();
  {
    upc_t upc = ops + UOPSIZE*PC;
    uop op = (uop)upc;
    PC = op (PC);
  }
#endif
}
