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

  if (address < 16)
    return FM[address];

  data = get_word_at (memory, address);
  if (data & 0400000000000LL)
    data |= -1LL << 36;
  return data;
}

void write_memory (int address, word_t data)
{
  fprintf (stderr, "CORE %06o %012llo\n", address, data);
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
static uop ops[3*MOBY];

static void uop_nop (void)
{
}

static void uop_read_immediate (void)
{
  AR = MA;
  fprintf (stderr, "AR %012llo\n", AR);
}

static void uop_read_memory (void)
{
  AR = read_memory (MA);
  fprintf (stderr, "AR %012llo\n", AR);
}

static void uop_read_ac (void)
{
  AR = FM[AC];
  fprintf (stderr, "AC %o, AR %012llo\n", AC, AR);
}

static void uop_read_ac_immediate (void)
{
  AR = FM[AC];
  BR = MA;
  fprintf (stderr, "AR %012llo\n", AR);
  fprintf (stderr, "BR %012llo\n", BR);
}

static void uop_read_both (void)
{
  AR = FM[AC];
  BR = read_memory (MA);
  fprintf (stderr, "AR %012llo\n", AR);
  fprintf (stderr, "AC %o, BR %012llo\n", AC, BR);
}

static void uop_write_ac (void)
{
  fprintf (stderr, "AC%o %012llo\n", AC, AR);
  FM[AC] = AR;
}

static void uop_write_ac1 (void)
{
  fprintf (stderr, "AC%o %012llo\n", AC + 1, MQ);
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
#define RDB2 0
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
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDB1, RDB2, RDAA, RDNO, RDNO, RDNO, RDNO, RDNO, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDB4, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  /* 200-277 */
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDB2, RDAA, RDB2, RDB2, RDB3, RDA2, RDB3, RDB3, 
  RDAA, RDAA, RDAA, RDAA, RDA2, RDA2, RDA2, RDNO, 
  RDB2, RDAA, RDAA, RDAA, RDNO, RDNO, RDNO, RDNO, 
  RDAA, RDB2, RDAA, RDAA, RDNO, RDNO, RDAB, RDNO, 
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  /* 300-377 */
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, 
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, 
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, 
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, 
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, 
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, 
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, 
  /* 400-477 */
  RDNO, RDNO, RDNO, RDNO, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDAB, RDB1, RDB1, RDMA, RDNO, RDNO, RDMA, 
  RDB1, RDAB, RDB1, RDB1, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, 
  RDAB, RDAB, RDAB, RDAB, RDB1, RDAB, RDB1, RDB1, 
  RDMA, RDNO, RDMA, RDMA, RDB1, RDAB, RDB1, RDB1, 
  RDB1, RDAB, RDB1, RDB1, RDNO, RDNO, RDNO, RDNO, 
  /* 500-577 */
  RDB1, RDAB, RDB2, RDMA, RDB1, RDAB, RDB2, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDB1, RDAB, RDB2, RDMA, RDB1, RDAB, RDB2, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  RDMA, RDNO, RDAA, RDMA, RDMA, RDNO, RDAA, RDMA, 
  /* 600-677 */
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, 
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, 
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, 
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, 
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, 
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
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, 
  WRA1, WRA1, WRM1, WRB1, WRA3, WRA3, WRM1, WRB3, 
  WRA3, WRA3, WRM1, WRB3, WRA3, WRA3, WRM1, WRB3, 
  WRA1, WRA1, WRA1, WRNO, WRA3, WRA3, WRA3, WRNO, 
  WRNO, WRNO, WRA1, WRA1, WRNO, WRNO, WRNO, WRNO, 
  WRA1, WRA1, WRA1, WRA1, WRNO, WRA1, WRM1, WRNO, 
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, 
  /* 300-377 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, 
  WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, 
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

static void uop_move (void)
{
  fprintf (stderr, "MOVE\n");
}

static void uop_exch (void)
{
  fprintf (stderr, "EXCH\n");
  exit (0);
}

static void uop_blt (void)
{
  fprintf (stderr, "BLT\n");
  exit (0);
}

static void uop_aobjp (void)
{
  fprintf (stderr, "AOBJP\n");
  exit (0);
}

static void uop_aobjn (void)
{
  fprintf (stderr, "AOBJN\n");
  exit (0);
}

static void uop_jrst (void)
{
  fprintf (stderr, "JRST\n");
  PC = MA;
}

static void uop_jfcl (void)
{
  fprintf (stderr, "JFCL\n");
  exit (0);
}

static void uop_xct (void)
{
  fprintf (stderr, "XCT\n");
  exit (0);
}

static void uop_pushj (void)
{
  fprintf (stderr, "PUSHJ\n");
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR & 0777777, PC);
  PC = MA;
}

static void uop_popj (void)
{
  fprintf (stderr, "POPJ\n");
  PC = read_memory (AR & 0777777);
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
}

static void uop_setz (void)
{
  fprintf (stderr, "SETZ\n");
  AR = 0;
}

static void uop_skipl (void)
{
  fprintf (stderr, "SKIPL: %012llo < %012llo\n", AR, BR);
  if (AR < BR)
    PC++;
}

static void uop_skipe (void)
{
  fprintf (stderr, "SkipE\n");
  if (AR == BR)
    PC++;
}

static void uop_skiple (void)
{
  fprintf (stderr, "SkipLE\n");
  if (AR <= BR)
    PC++;
}

static void uop_skipa (void)
{
  fprintf (stderr, "SkipN\n");
  PC++;
}

static void uop_skipge (void)
{
  fprintf (stderr, "SkipN\n");
  if (AR >= BR)
    PC++;
}

static void uop_skipn (void)
{
  fprintf (stderr, "SkipN\n");
  if (AR != BR)
    PC++;
}

static void uop_skipg (void)
{
  fprintf (stderr, "SkipN\n");
  if (AR > BR)
    PC++;
}

static void uop_jumpl (void)
{
  fprintf (stderr, "JumpL\n");
  if (AR < BR)
    PC = MA;
}

static void uop_jumpe (void)
{
  fprintf (stderr, "JumpE\n");
  if (AR == BR)
    PC = MA;
}

static void uop_jumple (void)
{
  fprintf (stderr, "JumpLE\n");
  if (AR <= BR)
    PC = MA;
}

static void uop_jumpa (void)
{
  fprintf (stderr, "JumpN\n");
  PC = MA;
}

static void uop_jumpge (void)
{
  fprintf (stderr, "JumpN\n");
  if (AR >= BR)
    PC = MA;
}

static void uop_jumpn (void)
{
  fprintf (stderr, "JumpN\n");
  if (AR != BR)
    PC = MA;
}

static void uop_jumpg (void)
{
  fprintf (stderr, "JumpN\n");
  if (AR > BR)
    PC = MA;
}

static void uop_tlo (void)
{
  fprintf (stderr, "TLO\n");
  AR |= BR;
}

static void uop_hll (void)
{
  fprintf (stderr, "HLL\n");
  AR = (BR & 0777777000000LL) | (AR & 0777777);
}

static void uop_hrl (void)
{
  fprintf (stderr, "HRL\n");
  AR = ((BR & 0777777LL) << 18) | (AR & 0777777);
}

static void uop_hlr (void)
{
  fprintf (stderr, "HLR\n");
  AR = (AR & 0777777000000LL) | ((BR >> 18) & 0777777);
}

static void uop_hrr (void)
{
  fprintf (stderr, "HRR\n");
  AR = (AR & 0777777000000LL) | (BR & 0777777);
}

static void uop_muuo (void)
{
  its_muuo ();
}

/* Table to decode an opcode into an operation uop. */
static uop operate[] = {
  /* 000-077 */
  0, 0, 0, 0, 0, 0, 0, 0, 
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
  uop_move, uop_move, uop_move, uop_move, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  uop_exch, uop_blt, uop_aobjp, uop_aobjn, uop_jrst, uop_jfcl, uop_xct, 0, 
  uop_pushj, 0, 0, uop_popj, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
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
  uop *upc = ops + 3*PC;
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
  ops[3*PC]();
}

/* Uop to decode a single word and execute it. */
static void decode_word (void)
{
  fprintf (stderr, "Decode word\n");
  decode ();
  retry ();
}

/* Uop to decode a page and then resume execution. */
static void decode_page (void)
{
  int save = PC;
  fprintf (stderr, "Decode page\n");
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
  ops[3*a] = decode_word;
}

/* Fill a page with a uop. */
static void fill_page (int a, uop op)
{
  int i;
  uop *upc = &ops[3*(a & 0776000)];
  for (i = 0; i < 02000; i++) {
    *upc = op;
    upc += 3;
  }
}

/* Fill an unpure page. */
void unpure_page (int a)
{
  fprintf (stderr, "Unpure page at %06o\n", a);
  fill_page (a, decode_word);
}

/* Fill an pure page. */
void pure_page (int a)
{
  fprintf (stderr, "Pure page at %06o\n", a);
  fill_page (a, decode_page);
}

/* Uop for an unmapped word. */
static void unmapped_word (void)
{
  fprintf (stderr, "Unmapped word.\n");
  exit (1);
}

/* Fill an unmapped page. */
void unmapped_page (int a)
{
  fill_page (a, unmapped_word);
}

static void calculate_ea (void)
{
  word_t x = IR;
  int address, X, I;

  do {
    address = x & 0777777;
    X = (IR >> 18) & 017;
    I = (IR >> 22) & 01;
    if (X)
      address += FM[X];
    if (I)
      x = read_memory (address);
  } while (I);

  MA = address;
}

/* Execute one instruction. */
static void step (void)
{
  uop *upc = ops + 3*PC;
  fprintf (stderr, "\nExecute %06o\n", PC);
  IR = read_memory (PC);
  AC = (IR >> 23) & 017;
  fprintf (stderr, "IR %012llo\n", IR);
  calculate_ea ();
  fprintf (stderr, "EA %06o\n", MA);
  (*upc++) ();
  PC++; /* Needs to happen here. */
  (*upc++) ();
  (*upc++) ();
}

/* Run. */
void run (int start, struct pdp10_memory *m)
{
  memory = m;
  PC = start;
  for (;;)
    step ();
}
