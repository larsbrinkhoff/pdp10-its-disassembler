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


/* Machine state. */

static int PC;
static int flags;
static word_t IR;
static word_t AR;
static word_t BR;
static word_t MQ;
static word_t MA;
static word_t MB;
static word_t AC[16];
static struct pdp10_memory *memory;

word_t fetch (int address)
{
  return get_word_at (memory, address);
}

/* Every word is decoded into three uops.  The first reads the
   operands, the second is the operation, and the third writes back
   the result. */

typedef void (*uop) (void);
static uop ops[3*MOBY];

static void uop_nop (void)
{
}

static void uop_rdma (void)
{
  AR = fetch (MA);
  fprintf (stderr, "AR %012llo\n", AR);
}

static void uop_write_ac (void)
{
  fprintf (stderr, "AC%llo %012llo\n", (IR >> 23) & 017, AR);
  AC[(IR >> 23) & 017] = AR;
}

/* Table to decode an opcode into a read uop. */
#define RDNO uop_nop
#define RDMA uop_rdma
#define RDAA 0
#define RDAB 0
#define RDAM 0
#define RDA2 0
#define RDB1 0
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
#define WRM1 0
#define WRA1 uop_write_ac
#define WRA2 0
#define WRA0 0
#define WRA3 0
#define WRB1 0
#define WRB2 0
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

static void uop_setz (void)
{
  fprintf (stderr, "SETZ\n");
  AR = 0;
}

static void uop_muuo (void)
{
  its_muuo (IR, MA);
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
  uop_move, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 300-377 */
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 400-477 */
  uop_setz, uop_setz, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  /* 500-577 */
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
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
  0, 0, 0, 0, 0, 0, 0, 0, 
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
  word_t opcode = IR >> 27;
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
    IR = fetch (PC);
    decode ();
    PC++;
  } while ((PC & 01777) != 0);
  PC = save;
  retry ();
}

/* Invalidate a single word. */
void invalidate_word (int a)
{
  uop *upc = ops + 3*a;
  *upc = decode_word;
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
      address += AC[X];
    if (I)
      x = fetch (address);
  } while (I);

  MA = address;
}

/* Execute one instruction. */
static void step (void)
{
  uop *upc = ops + 3*PC;
  fprintf (stderr, "Execute %06o\n", PC);
  IR = fetch (PC);
  fprintf (stderr, "IR %012llo\n", IR);
  calculate_ea ();
  fprintf (stderr, "EA %012llo\n", MA);
  (*upc++) ();
  PC++;
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
