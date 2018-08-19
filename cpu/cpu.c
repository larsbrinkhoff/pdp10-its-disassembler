/* Just-in-time translating PDP-10 emulator.

   Entry points:

   pure_page (address) - Call this when a pure page has been loaded.

   unpure_page (address) - Call this when an unpure page has been loaded.

   unmapped_page (address) - Call this when there's an unmapped page.

   invalidate_word (address) - Call this when a word has been written.

   run (address) - Run the emulator.
*/

#include "memory.h"
#include "cpu/its.h"

typedef long long word_t;
void invalidate_word (int a);
static void execute (int *pc);

#define DEBUG(X) fprintf X
#define TODO(X) DEBUG((stderr, "TODO: %s\n", #X)); exit (1)

#define SIGN_EXTEND(X)                          \
  if (X & 0400000000000LL)                      \
    X |= -1LL << 36;

/* Machine state. */

int MA;
int AC;
int PC;
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

typedef void (*uop) (void);
#define UOPSIZE 3
static uop ops[UOPSIZE*MOBY];

static void uop_nop (void)
{
}

static void uop_read_immediate (void)
{
  AR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
}

static void uop_read_memory (void)
{
  AR = read_memory (MA);
  BR = 0;
  if (AR == 0777700000000LL)
    AR = -010000LL << 18;
  DEBUG((stderr, "AR %012llo\n", AR));
}

static void uop_read_ac (void)
{
  AR = FM[AC];
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
}

static void uop_read_ac_immediate (void)
{
  AR = FM[AC];
  BR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "BR %012llo\n", BR));
}

static void uop_read_both (void)
{
  AR = FM[AC];
  BR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "AC %o, BR %012llo\n", AC, BR));
}

static void uop_write_ac (void)
{
  DEBUG((stderr, "AC%o %012llo\n", AC, AR));
  FM[AC] = AR;
}

static void uop_write_ac1 (void)
{
  DEBUG((stderr, "AC%o %012llo\n", AC + 1, MQ));
  FM[AC+1] = MQ;
}

static void uop_write_acnz (void)
{
  if (AC != 0)
    FM[AC] = AR;
}

static void uop_write_mem (void)
{
  write_memory (MA, AR);
}

static void uop_write_both (void)
{
  FM[AC] = AR;
  write_memory (MA, AR);
}

static void uop_write_ac_mem (void)
{
  FM[AC] = AR;
  write_memory (MA, BR);
}

static void uop_write_same (void)
{
  if (AC != 0)
    FM[AC] = AR;
  write_memory (MA, AR);
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

static void uop_luuo (void)
{
  word_t x = IR & 0777740000000LL;
  write_memory (040, x + MA);
  // XCT 41; don't change PC.
  TODO(LUUO);
}

static void uop_muuo (void)
{
  its_muuo ();
}

static void uop_move (void)
{
  DEBUG((stderr, "MOVE\n"));
}

static void uop_movs (void)
{
  DEBUG((stderr, "MOVS\n"));
  AR = ((AR >> 18) & 0777777) | ((AR & 0777777) << 18);
  DEBUG((stderr, "AR %012llo\n", AR));
}

static void uop_movn (void)
{
  DEBUG((stderr, "MOVN\n"));
  AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
}

static void uop_movm (void)
{
  DEBUG((stderr, "MOVM\n"));
  if (AR & 0400000000000LL)
    AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
}

static void uop_ash (void)
{
  TODO(ASH);
}

static void uop_rot (void)
{
  TODO(ROT);
}

static void uop_lsh (void)
{
  TODO(LSH);
}

static void uop_jffo (void)
{
  TODO(JFFO);
}

static void uop_ashc (void)
{
  TODO(ASHC);
}

static void uop_rotc (void)
{
  TODO(ROTC);
}

static void uop_lshc (void)
{
  TODO(LSHC);
}

static void uop_exch (void)
{
  DEBUG((stderr, "EXCH\n"));
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  MQ = AR;
  AR = BR;
  BR = MQ;
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
}

static void uop_blt (void)
{
  TODO(BLT);
}

static void uop_aobjp (void)
{
  TODO(AOBJP);
}

static void uop_aobjn (void)
{
  word_t LH = AR & 0777777000000LL;
  DEBUG((stderr, "AOBJN\n"));
  LH += 01000000LL;
  AR = LH | ((AR + 1) & 0777777LL);
  if (LH == 01000000LL)
    LH = 0;
  else
    PC = MA;
}

static void uop_jrst (void)
{
  DEBUG((stderr, "JRST\n"));
  if (AC & 14)
    return uop_muuo ();
  if (AC & 2)
    flags = MB >> 18;
  PC = MA;
}

static void uop_jfcl (void)
{
  TODO(JFCL);
}

static void uop_xct (void)
{
  DEBUG((stderr, "XCT\n"));
  execute (&MA);
}

static void uop_pushj (void)
{
  DEBUG((stderr, "PUSHJ\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, (flags << 18) | PC);
  PC = MA;
}

static void uop_push (void)
{
  DEBUG((stderr, "PUSH\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, BR);
}

static void uop_popj (void)
{
  word_t x;
  DEBUG((stderr, "POPJ\n"));
  DEBUG((stderr, "AR %012llo\n", AR));
  x = read_memory (AR);
  PC = x & 0777777;
  flags = (x >> 18) & 0777777;
  DEBUG((stderr, "PC %06o\n", PC));
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
}

static void uop_pop (void)
{
  DEBUG((stderr, "POP\n"));
  MB = read_memory (AR);
  write_memory (MA, MB);
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
}

static void uop_jsr (void)
{
  DEBUG((stderr, "JSR\n"));
  write_memory (AR, (flags << 18) | PC);
  PC = AR + 1;
}

static void uop_jsp (void)
{
  DEBUG((stderr, "JSP\n"));
  AR = (flags << 18) | PC;
  PC = MA;
}

static void uop_jsa (void)
{
  TODO(JSA);
}

static void uop_jra (void)
{
  TODO(JRA);
}

static void uop_add (void)
{
  DEBUG((stderr, "ADD\n"));
  AR = AR + BR;
}

static void uop_sub (void)
{
  DEBUG((stderr, "SUB\n"));
  AR = AR - BR;
}

static void uop_setz (void)
{
  DEBUG((stderr, "SETZ\n"));
  AR = 0;
}

static void uop_skipl (void)
{
  DEBUG((stderr, "SKIPL: %012llo < %012llo\n", AR, BR));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR < BR)
    PC++;
}

static void uop_skipe (void)
{
  DEBUG((stderr, "SKIPE\n"));
  if (AR == BR)
    PC++;
}

static void uop_skiple (void)
{
  DEBUG((stderr, "SKIPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR <= BR)
    PC++;
}

static void uop_skipa (void)
{
  DEBUG((stderr, "SKIPA\n"));
  PC++;
}

static void uop_skipge (void)
{
  DEBUG((stderr, "SKIPGE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR >= BR)
    PC++;
}

static void uop_skipn (void)
{
  DEBUG((stderr, "SKIPN\n"));
  if (AR != BR)
    PC++;
}

static void uop_skipg (void)
{
  DEBUG((stderr, "SKIPG\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR > BR)
    PC++;
}

static void uop_jumpl (void)
{
  DEBUG((stderr, "JUMPL: %012llo < 0\n", AR));
  SIGN_EXTEND (AR);
  if (AR < 0)
    PC = MA;
}

static void uop_jumpe (void)
{
  DEBUG((stderr, "JUMPE\n"));
  if (AR == 0)
    PC = MA;
}

static void uop_jumple (void)
{
  DEBUG((stderr, "JUMPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR <= 0)
    PC = MA;
}

static void uop_jumpa (void)
{
  DEBUG((stderr, "JUMPA\n"));
  PC = MA;
}

static void uop_jumpge (void)
{
  DEBUG((stderr, "JUMPGE\n"));
  SIGN_EXTEND (AR);
  if (AR >= 0)
    PC = MA;
}

static void uop_jumpn (void)
{
  DEBUG((stderr, "JUMPN\n"));
  if (AR != 0)
    PC = MA;
}

static void uop_jumpg (void)
{
  DEBUG((stderr, "JUMPG\n"));
  SIGN_EXTEND (AR);
  if (AR > 0)
    PC = MA;
}

static void uop_tlo (void)
{
  DEBUG((stderr, "TLO\n"));
  AR |= BR;
}

static void uop_hll (void)
{
  DEBUG((stderr, "HLL\n"));
  AR = (BR & 0777777000000LL) | (AR & 0777777);
}

static void uop_hrl (void)
{
  DEBUG((stderr, "HRL\n"));
  AR = ((BR & 0777777LL) << 18) | (AR & 0777777);
}

static void uop_hlr (void)
{
  DEBUG((stderr, "HLR\n"));
  AR = (AR & 0777777000000LL) | ((BR >> 18) & 0777777);
}

static void uop_hrr (void)
{
  DEBUG((stderr, "HRR\n"));
  AR = (AR & 0777777000000LL) | (BR & 0777777);
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
  uop_setz, uop_setz, uop_setz, uop_setz, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
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

/* Decode one word. */
static void decode (void)
{
  word_t opcode = (IR >> 27) & 0777;
  uop *upc = ops + UOPSIZE*PC;
  *upc++ = read_operands[opcode];
  if (opcode == 0777)
    *upc++ = iot[(IR >> 23) & 3];
  else
    *upc++ = operate[opcode];
  *upc++ = write_back[opcode];
}

/* Retry executing the first uop after a decode operation. */
static void retry (void)
{
  ops[UOPSIZE*PC]();
}

/* Uop to decode a single word and execute it. */
static void decode_word (void)
{
  DEBUG((stderr, "Decode word\n"));
  decode ();
  retry ();
}

/* Uop to decode a page and then resume execution. */
static void decode_page (void)
{
  int save = PC;
  DEBUG((stderr, "Decode page\n"));
  PC &= 0776000;
  do {
    IR = read_memory (PC);
    decode ();
    PC++;
  } while ((PC & 01777) != 0);
  PC = save;
  retry ();
}

/* Invalidate a single word. */
void invalidate_word (int a)
{
  ops[UOPSIZE*a] = decode_word;
}

/* Fill a page with a uop. */
static void fill_page (int a, uop op)
{
  int i;
  uop *upc = &ops[UOPSIZE*(a & 0776000)];
  for (i = 0; i < 02000; i++) {
    *upc = op;
    upc += UOPSIZE;
  }
}

/* Fill an unpure page. */
void unpure_page (int a)
{
  fill_page (a, decode_word);
}

/* Fill an pure page. */
void pure_page (int a)
{
  fill_page (a, decode_page);
}

/* Uop for an unmapped word. */
static void unmapped_word (void)
{
  DEBUG((stderr, "Unmapped word.\n"));
  exit (1);
}

/* Fill an unmapped page. */
void unmapped_page (int a)
{
  fill_page (a, unmapped_word);
}

static void calculate_ea (void)
{
  MB = IR;
  int address, X, I;

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
}

/* Execute one instruction. */
static void execute (int *pc)
{
  uop *upc = ops + UOPSIZE*(*pc);
  DEBUG((stderr, "\nExecute %06o\n", *pc));
  IR = read_memory (*pc);
  AC = (IR >> 23) & 017;
  DEBUG((stderr, "IR %012llo\n", IR));
  calculate_ea ();
  DEBUG((stderr, "EA %06o\n", MA));
  (*upc++) ();
  (*pc)++; /* Needs to happen here. */
  (*upc++) ();
  (*upc++) ();
}

/* Run. */
void run (int start, struct pdp10_memory *m)
{
  memory = m;
  PC = start;
  for (;;)
    execute (&PC);
}

#if 0
int foo1 (int x)
{
  return x + 1;
  // 8d 47 01             	lea    0x1(%rdi),%eax
  // c3                   	retq   
}

int foo2 (int x)
{
  extern int bar (int);
  return bar(x) + 1;
  // 50                   	push   %rax
  // e8 de 05 00 00       	callq  409440 <bar>
  // ff c0                	inc    %eax
  // c3                   	retq   
}
#endif
