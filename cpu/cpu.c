/* Just-in-time translating PDP-10 emulator.

   Entry points:

   pure_page (address) - Call this when a pure page has been loaded.

   unpure_page (address) - Call this when an unpure page has been loaded.

   unmapped_page (address) - Call this when there's an unmapped page.

   invalidate_word (address) - Call this when a word has been written.

   run (address) - Run the emulator.
*/

#define MOBY  (256*1024)

/* Machine state. */

static int PC;
static int flags;
static word_t IR;
static word_t AR;
static word_t BR;
static word_t MQ;
static word_t MA;
static word_t MB;


/* Every word is decoded into three uops.  The first reads the
   operands, the second is the operation, and the third writes back
   the result. */

typedef void (*uop) (void);
static uop ops[3*MOBY];

/* Table to decode an opcode into a read uop. */
static unsigned char read_operands[] = {
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
static unsigned char write_back[] = {
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

/* Table to decode an opcode into an operation uop. */
static unsigned char operate[] = {

};

/* Table to decode an IO instruction into a uop. */
static unsigned char iot[] = {
  CONI, CONO, DATAI, DATAO
}

/* Decode one word. */
static void decode (void)
{
  word_t opcode = IR >> 27;
  uop *upc = ops + 3*PC;
  *upc++ = read_operands[opcode];
  if (opcode == 0777)
    *upc++ = iot[(insn >> 23) & 3];
  else
    *upc++ = operate[opcode];
  *upc++ = write_back[opcode];
}

/* Retry executing the first uop after a decode operation. */
static void retry (void)
{
  uop *upc = ops + 3*PC;
  (*upc) ();
}

/* Uop to decode a single word and execute it. */
static void decode_word (void)
{
  decode ();
  retry ();
}

/* Uop to decode a page and then resume execution. */
static void decode_page (void)
{
  int save = PC;
  PC &= 0776000;
  do {
    IR = fetch (PC);
    decode ();
    PC++;
  } until ((PC & 01777) == 0);
  PC = save;
  retry ();
}

/* Invalidate a single word. */
void invalidate_word (int a)
{
  uop *upc = ops[3*a];
  ops[upc] = decode_word;
}

/* Fill a page with a uop. */
static int fill_page (int a, uop op)
{
  int i;
  uop *upc = ops[3*(a & 0776000)];
  for (i = 0; i < 02000; i++) {
    ops[upc] = op;
    upc += 3;
  }
}

/* Fill an unpure page. */
int unpure_page (int a)
{
  fill_page (a, decode_word);
}

/* Fill an pure page. */
int pure_page (int a)
{
  fill_page (a, decode_page);
}

/* Uop for an unmapped word. */
static void unmapped_word (void)
{
  /* error */
}

/* Fill an unmapped page. */
int unmapped_page (int a)
{
  fill_page (a, unmapped_word);
}

/* Execute one instruction. */
static void step (void)
{
  uop *upc = ops + 3*PC;
  IR = fetch (PC);
  calculate_ea ();
  (*uop++) ();
  PC++;
  (*uop++) ();
  (*uop++) ();
}

/* Run. */
void run (int start)
{
  PC = start;
  for (;;)
    step ();
}
