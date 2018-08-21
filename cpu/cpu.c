/* Just-in-time translating PDP-10 emulator.

   Entry points:

   pure_page (address) - Call this when a pure page has been loaded.

   unpure_page (address) - Call this when an unpure page has been loaded.

   unmapped_page (address) - Call this when there's an unmapped page.

   invalidate_word (address) - Call this when a word has been written.

   run (address) - Run the emulator.
*/

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "memory.h"
#include "cpu/its.h"

#define THREADED 0

#if defined(__GNUC__) && defined (__i386__)
#define REGPARM __attribute__((regparm(2)))
#else
#define REGPARM
#endif

typedef long long word_t;

#if THREADED
#define UDEF(NAME) int REGPARM NAME (int PC)
#define URET return PC
#define PCINC PC++
#define UPC PC
#define RETRY PC = retry (PC)
#define JUMP(X) PC = (X)
#else
#define UDEF(NAME) word_t REGPARM NAME (word_t AR)
#define URET return AR
#define PCINC
#define UPC (1 + ((size_t)__builtin_return_address (0) - (size_t)ops) / USIZE)
#define RETRY ((size_t *)__builtin_frame_address (0))[1] -= 5; \
              asm volatile("": : :"memory")
#define JUMP(X) do {                                                    \
    ((unsigned char **)__builtin_frame_address (0))[1] = (USIZE*(X) + ops); \
    asm volatile("": : :"memory");                                      \
  } while (0)
#endif

static UDEF(calculate_ea);
void invalidate_word (int a);
#if THREADED
static int execute (int);
#endif

#define DEBUG(X) fprintf X
#define TODO(X) DEBUG((stderr, "TODO: %s\n", #X)); exit (1)

#define SIGN_EXTEND(X)                          \
  if (X & 0400000000000LL)                      \
    X |= -1LL << 36

/* Machine state. */

int MA;
int AC;
int flags;
word_t IR;
#if THREADED
word_t AR;
#endif
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

typedef UDEF((*uop));

#if THREADED
#define USIZE 3
static uop ops[USIZE*MOBY];
typedef uop *upc_t;
#else
#define USIZE (4*5)
static unsigned char ops[USIZE*MOBY];
typedef unsigned char *upc_t;
#endif


static UDEF(uop_nop)
{
  URET;
}

static UDEF(uop_read_nop)
{
  PCINC;
  URET;
}

static UDEF(uop_read_immediate)
{
  AR = MA;
  BR = 0;
  DEBUG((stderr, "AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_immediate2)
{
  AR = 0;
  BR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_memory)
{
  AR = read_memory (MA);
  BR = 0;
  DEBUG((stderr, "AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_memory2)
{
  AR = 0;
  BR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_memory_plus)
{
  AR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  AR = (AR + 1) & 0777777777777LL;
  write_memory (MA, AR);
  BR = 0;
  DEBUG((stderr, "+1 -> AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_memory_minus)
{
  AR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  AR = (AR - 1) & 0777777777777LL;
  write_memory (MA, AR);
  BR = 0;
  DEBUG((stderr, "-1 -> AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_same)
{
  AR = read_memory (MA);
  BR = AR;
  DEBUG((stderr, "Same: AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_ac)
{
  AR = FM[AC];
  BR = 0;
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
  PCINC;
  URET;
}

static UDEF(uop_read_ac_plus)
{
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
  AR = (FM[AC] + 1) & 0777777777777LL;
  FM[AC] = AR;
  BR = 0;
  DEBUG((stderr, "+1 -> AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_ac_minus)
{
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
  AR = (FM[AC] - 1) & 0777777777777LL;
  FM[AC] = AR;
  BR = 0;
  DEBUG((stderr, "-1 -> AR %012llo\n", AR));
  PCINC;
  URET;
}

static UDEF(uop_read_ac2)
{
  AR = 0;
  BR = FM[AC];
  DEBUG((stderr, "AC %o, AR %012llo\n", AC, AR));
  PCINC;
  URET;
}

static UDEF(uop_read_ac_immediate)
{
  AR = FM[AC];
  BR = MA;
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "BR %012llo\n", BR));
  PCINC;
  URET;
}

static UDEF(uop_read_both)
{
  AR = FM[AC];
  BR = read_memory (MA);
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "AC %o, BR %012llo\n", AC, BR));
  PCINC;
  URET;
}

static UDEF(uop_read_both2)
{
  AR = read_memory (MA);
  BR = FM[AC];
  DEBUG((stderr, "AR %012llo\n", AR));
  DEBUG((stderr, "AC %o, BR %012llo\n", AC, BR));
  PCINC;
  URET;
}

static UDEF(uop_write_ac)
{
  DEBUG((stderr, "AC%o %012llo\n", AC, AR));
  FM[AC] = AR;
  URET;
}

#if 0
static UDEF(uop_write_ac1)
{
  DEBUG((stderr, "AC%o %012llo\n", AC + 1, MQ));
  FM[AC+1] = MQ;
  URET;
}
#endif

static UDEF(uop_write_acnz)
{
  if (AC != 0)
    FM[AC] = AR;
  URET;
}

static UDEF(uop_write_mem)
{
  write_memory (MA, AR);
  URET;
}

static UDEF(uop_write_both)
{
  FM[AC] = AR;
  write_memory (MA, AR);
  URET;
}

static UDEF(uop_write_ac_mem)
{
  FM[AC] = AR;
  write_memory (MA, BR);
  URET;
}

static UDEF(uop_write_same)
{
  if (AC != 0)
    FM[AC] = AR;
  write_memory (MA, AR);
  URET;
}

/* Table to decode an opcode into a read uop. */
#define RDNO uop_read_nop
#define RDIM uop_read_immediate
#define RDI2 uop_read_immediate2
#define RDMA uop_read_memory
#define RDM2 uop_read_memory2
#define RDMP uop_read_memory_plus
#define RDMD uop_read_memory_minus
#define RDAA uop_read_ac
#define RDAP uop_read_ac_plus
#define RDAD uop_read_ac_minus
#define RDAB uop_read_ac_immediate
#define RDAM 0
#define RDA2 uop_read_ac2
#define RDB1 uop_read_both
#define RDB2 uop_read_both2
#define RDSM uop_read_same
#define RDB3 0
#define RDB4 0

static uop read_operands[] = {
  /* 000-077 */
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  /* 100-177 */
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, // DFAD etc 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, // DMOVE etc
  RDB1, RDB2, RDAA, RDIM, RDIM, RDIM, RDIM, RDIM, // UFA etc
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FAD
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FSB
  RDB1, RDB1, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FMP
  RDB1, RDB4, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // FDV
  /* 200-277 */
  RDMA, RDIM, RDAA, RDMA, RDMA, RDIM, RDAA, RDMA, // MOVE, MOVS
  RDMA, RDIM, RDAA, RDMA, RDMA, RDIM, RDAA, RDMA, // MOVN, MOVM
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // IMUL, MUL
  RDB2, RDAA, RDB2, RDB2, RDB3, RDA2, RDB3, RDB3, // IDIV, DIV
  RDAA, RDAA, RDAA, RDAA, RDA2, RDA2, RDA2, RDIM, // ASH etc
  RDB1, RDAA, RDAA, RDAA, RDIM, RDIM, RDIM, RDIM, // EXCH etc
  RDAA, RDB1, RDAA, RDAA, RDIM, RDIM, RDAB, RDIM, // PUSHJ etc
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // ADD, SUB
  /* 300-377 */
  RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, RDAB, // CAI
  RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, RDB1, // CAM
  RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, RDAA, // JUMP
  RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, RDMA, // SKIP
  RDAP, RDAP, RDAP, RDAP, RDAP, RDAP, RDAP, RDAP, // AOJ
  RDMP, RDMP, RDMP, RDMP, RDMP, RDMP, RDMP, RDMP, // AOS
  RDAD, RDAD, RDAD, RDAD, RDAD, RDAD, RDAD, RDAD, // SOJ
  RDMD, RDMD, RDMD, RDMD, RDMD, RDMD, RDMD, RDMD, // SOS
  /* 400-477 */
  RDNO, RDNO, RDNO, RDNO, RDB1, RDAB, RDB1, RDB1, // SETZ, AND
  RDB1, RDAB, RDB1, RDB1, RDMA, RDIM, RDIM, RDMA, // ANDCA, SETM
  RDB1, RDAB, RDB1, RDB1, RDAB, RDAB, RDAB, RDAB, // ANDCM, SETA
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // XOR, IOR
  RDB1, RDAB, RDB1, RDB1, RDB1, RDAB, RDB1, RDB1, // ANDCB, EQV
  RDAB, RDAB, RDAB, RDAB, RDB1, RDAB, RDB1, RDB1, // SETCA, ORCA
  RDMA, RDIM, RDMA, RDMA, RDB1, RDAB, RDB1, RDB1, // SETCM, ORCM
  RDB1, RDAB, RDB1, RDB1, RDNO, RDNO, RDNO, RDNO, // ORCB, SETO
  /* 500-577 */
  RDB1, RDAB, RDB2, RDSM, RDB1, RDAB, RDB2, RDSM, // HLL, HRL
  RDM2, RDI2, RDA2, RDM2, RDM2, RDI2, RDA2, RDM2, // HLLZ, HRLZ
  RDM2, RDIM, RDA2, RDM2, RDM2, RDI2, RDA2, RDM2, // HLLO, HRLO
  RDM2, RDIM, RDA2, RDM2, RDM2, RDI2, RDA2, RDM2, // HLLE, HRLE
  RDB1, RDAB, RDB2, RDSM, RDB1, RDAB, RDB2, RDSM, // HRR, HLR
  RDM2, RDI2, RDA2, RDM2, RDM2, RDIM, RDA2, RDM2, // HRRZ, HLRZ
  RDM2, RDI2, RDA2, RDM2, RDM2, RDIM, RDA2, RDM2, // HRRO, HLRO
  RDM2, RDI2, RDA2, RDM2, RDM2, RDIM, RDA2, RDM2, // HRRE, HLRE
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
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
  RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, RDIM, 
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
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // DFAD etc 
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // DMOVE etc
  WRNO, WRA1, WRA1, WRNO, WRNO, WRNO, WRNO, WRNO, // UFA etc
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // FAD
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // FSB
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // FMP
  WRA1, WRA3, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // FDV
  /* 200-277 */
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // MOVE, MOVS
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // MOVN, MOVM
  WRA1, WRA1, WRM1, WRB1, WRA3, WRA3, WRM1, WRB3, // ASH etc
  WRA3, WRA3, WRM1, WRB3, WRA3, WRA3, WRM1, WRB3, // EXCH etc
  WRA1, WRA1, WRA1, WRNO, WRA3, WRA3, WRA3, WRNO, // PUSHJ etc
  WRB4, WRNO, WRA1, WRNO, WRNO, WRNO, WRNO, WRNO, // EXCH etc
  WRA1, WRA1, WRA1, WRA1, WRNO, WRNO, WRM1, WRNO, // PUSHJ etc
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // ADD, SUB
  /* 300-377 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // CAI
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // CAM
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // JUMP
  WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, WRA0, // SKIP
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // AOJ
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // AOS
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // SOJ
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // SOS
  /* 400-477 */
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // SETZ, AND
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRNO, WRA1, // ANDCA, SETM
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // ANDCM, SETA
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // XOR, IOR
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // ANDCB, EQV
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // SETCA, ORCA
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // SETCM, ORCM
  WRA1, WRA1, WRM1, WRB1, WRA1, WRA1, WRM1, WRB1, // ORCB, SETO
  /* 500-577 */
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HLL, HRL
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HLLZ, HRLZ
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HLLO, HRLO
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HLLE, HRLE
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HRR, HLR
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HRRZ, HLRZ
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HRRO, HLRO
  WRA1, WRA1, WRM1, WRB2, WRA1, WRA1, WRM1, WRB2, // HRRE, HLRE
  /* 600-677 */
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // TxN
  WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, WRNO, // TxN
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxZ
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxZ
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxC
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxC
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxO
  WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, WRA1, // TxO
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

static UDEF(uop_luuo)
{
  word_t x = IR & 0777740000000LL;
  write_memory (040, x + MA);
  // XCT 41; don't change PC.
  TODO(LUUO);
  URET;
}

static UDEF(uop_muuo)
{
  JUMP(its_muuo (UPC));
  URET;
}

static UDEF(uop_move)
{
  DEBUG((stderr, "MOVE\n"));
  DEBUG((stderr, "AR %012llo\n", AR));
  // TODO: flags
  URET;
}

static UDEF(uop_movs)
{
  DEBUG((stderr, "MOVS\n"));
  AR = ((AR >> 18) & 0777777) | ((AR & 0777777) << 18);
  DEBUG((stderr, "AR %012llo\n", AR));
  URET;
}

static UDEF(uop_movn)
{
  DEBUG((stderr, "MOVN\n"));
  AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
  URET;
}

static UDEF(uop_movm)
{
  DEBUG((stderr, "MOVM\n"));
  if (AR & 0400000000000LL)
    AR = (-AR) & 0777777777777LL;
  DEBUG((stderr, "AR %012llo\n", AR));
  URET;
}

static UDEF(uop_ash)
{
  DEBUG((stderr, "ASH\n"));
  DEBUG((stderr, "MA %06o\n", MA));
  if (MA & 0400000LL) {
    MA |= -1 << 18;
    if (AR & 0400000000000LL) {
      word_t sign = -1LL << (36+MA);
      AR >>= -MA;
      AR |= sign & 0777777777777;
    } else
      AR >>= -MA;
  } else
    AR = (AR << MA) & 0777777777777;
  URET;
}

static UDEF(uop_rot)
{
  TODO(ROT);
  URET;
}

static UDEF(uop_lsh)
{
  DEBUG((stderr, "LSH\n"));
  if (MA & 0400000000000LL)
    AR >>= MA;
  else
    AR <<= MA;
  URET;
}

static UDEF(uop_jffo)
{
  TODO(JFFO);
  URET;
}

static UDEF(uop_ashc)
{
  TODO(ASHC);
  URET;
}

static UDEF(uop_rotc)
{
  TODO(ROTC);
  URET;
}

static UDEF(uop_lshc)
{
  TODO(LSHC);
  URET;
}

static UDEF(uop_exch)
{
  DEBUG((stderr, "EXCH\n"));
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  MQ = AR;
  AR = BR;
  BR = MQ;
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  URET;
}

static UDEF(uop_blt)
{
  TODO(BLT);
  URET;
}

static UDEF(uop_aobjp)
{
  TODO(AOBJP);
  URET;
}

static UDEF(uop_aobjn)
{
  word_t LH = AR & 0777777000000LL;
  DEBUG((stderr, "AOBJN\n"));
  LH += 01000000LL;
  AR = (AR + 1) & 0777777LL;
  if (LH != 01000000000000LL) {
    FM[AC] = AR | LH;
    JUMP(MA);
  } else
    FM[AC] = AR;
  URET;
}

static UDEF(uop_jrst)
{
  DEBUG((stderr, "JRST\n"));
#if 0
  if (AC & 14)
    return uop_muuo (UPC);
#endif
  if (AC & 2)
    flags = MB >> 18;
  JUMP(MA);
  URET;
}

static UDEF(uop_jfcl)
{
  TODO(JFCL);
  URET;
}

static UDEF(uop_xct)
{
  TODO(XCT);
  // Don't set PC to target instruction.
  // Or do, and work around it.
  URET;
}

static UDEF(uop_pushj)
{
  DEBUG((stderr, "PUSHJ\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, (flags << 18) | UPC);
  FM[AC] = AR;
  JUMP(MA);
  URET;
}

static UDEF(uop_push)
{
  DEBUG((stderr, "PUSH\n"));
  AR = ((AR + 0000001000000LL) & 0777777000000LL) | ((AR + 1) & 0777777LL);
  write_memory (AR, BR);
  URET;
}

static UDEF(uop_popj)
{
  word_t x;
  DEBUG((stderr, "POPJ\n"));
  DEBUG((stderr, "AR %012llo\n", AR));
  x = read_memory (AR);
  flags = (x >> 18) & 0777777;
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
  FM[AC] = AR;
  JUMP(x & 0777777);
  URET;
}

static UDEF(uop_pop)
{
  DEBUG((stderr, "POP\n"));
  MB = read_memory (AR);
  write_memory (MA, MB);
  AR = ((AR + 0777777000000LL) & 0777777000000LL) | ((AR - 1) & 0777777LL);
  URET;
}

static UDEF(uop_jsr)
{
  DEBUG((stderr, "JSR\n"));
  write_memory (AR, (flags << 18) | UPC);
  JUMP(AR + 1);
  URET;
}

static UDEF(uop_jsp)
{
  DEBUG((stderr, "JSP\n"));
  AR = (flags << 18) | UPC;
  FM[AC] = AR;
  JUMP(MA);
  URET;
}

static UDEF(uop_jsa)
{
  TODO(JSA);
  URET;
}

static UDEF(uop_jra)
{
  TODO(JRA);
  URET;
}

static UDEF(uop_add)
{
  DEBUG((stderr, "ADD\n"));
  AR = AR + BR;
  AR &= 0777777777777LL;
  URET;
}

static UDEF(uop_sub)
{
  DEBUG((stderr, "SUB\n"));
  AR = AR - BR;
  AR &= 0777777777777LL;
  URET;
}

static UDEF(uop_setz)
{
  DEBUG((stderr, "SETZ\n"));
  AR = 0;
  URET;
}

static UDEF(uop_and)
{
  DEBUG((stderr, "AND\n"));
  AR &= BR;
  URET;
}

static UDEF(uop_andca)
{
  DEBUG((stderr, "ANDCA\n"));
  AR = BR & (~AR & 0777777777777LL);
  URET;
}

static UDEF(uop_andcm)
{
  DEBUG((stderr, "ANDCM\n"));
  AR &= ~BR & 0777777777777LL;
  URET;
}

static UDEF(uop_xor)
{
  DEBUG((stderr, "XOR\n"));
  AR ^= BR;
  URET;
}

static UDEF(uop_ior)
{
  DEBUG((stderr, "IOR\n"));
  AR |= BR;
  URET;
}

static UDEF(uop_andcb)
{
  DEBUG((stderr, "ANDCB\n"));
  AR = (~AR & 0777777777777LL) & (~BR & 0777777777777LL);
  URET;
}

static UDEF(uop_eqv)
{
  DEBUG((stderr, "EQV\n"));
  AR = ~(AR ^ BR) & 0777777777777LL;
  URET;
}

static UDEF(uop_setca)
{
  DEBUG((stderr, "SETCA\n"));
  AR = ~AR & 0777777777777LL;
  URET;
}

static UDEF(uop_orca)
{
  DEBUG((stderr, "ORCA\n"));
  AR = BR | (~AR & 0777777777777LL);
  URET;
}

static UDEF(uop_setcm)
{
  DEBUG((stderr, "SETCM\n"));
  AR ^= 0777777777777LL;
  URET;
}

static UDEF(uop_orcm)
{
  DEBUG((stderr, "ORCM\n"));
  AR |= ~BR & 0777777777777LL;
  URET;
}

static UDEF(uop_orcb)
{
  DEBUG((stderr, "ORCB\n"));
  AR = (~AR & 0777777777777LL) | (~BR & 0777777777777LL);
  URET;
}

static UDEF(uop_seto)
{
  DEBUG((stderr, "SETO\n"));
  AR = 0777777777777LL;
  URET;
}

static UDEF(uop_skipl)
{
  DEBUG((stderr, "SKIPL: %012llo < %012llo\n", AR, BR));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AC != 0)
    FM[AC] = AR;
  if (AR < BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_skipe)
{
  DEBUG((stderr, "SKIPE\n"));
  if (AC != 0)
    FM[AC] = AR;
  if (AR == BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_skiple)
{
  DEBUG((stderr, "SKIPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AC != 0)
    FM[AC] = AR;
  if (AR <= BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_skipa)
{
  DEBUG((stderr, "SKIPA\n"));
  if (AC != 0)
    FM[AC] = AR;
  JUMP(UPC+1);
  URET;
}

static UDEF(uop_skipge)
{
  DEBUG((stderr, "SKIPGE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AC != 0)
    FM[AC] = AR;
  if (AR >= BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_skipn)
{
  DEBUG((stderr, "SKIPN\n"));
  if (AC != 0)
    FM[AC] = AR;
  if (AR != BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_skipg)
{
  DEBUG((stderr, "SKIPG\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AC != 0)
    FM[AC] = AR;
  if (AR > BR)
    JUMP(UPC+1);
  URET;
}

static UDEF(uop_jumpl)
{
  DEBUG((stderr, "JUMPL: %012llo < 0\n", AR));
  SIGN_EXTEND (AR);
  if (AR < 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_jumpe)
{
  DEBUG((stderr, "JUMPE\n"));
  if (AR == 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_jumple)
{
  DEBUG((stderr, "JUMPLE\n"));
  SIGN_EXTEND (AR);
  SIGN_EXTEND (BR);
  if (AR <= 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_jumpa)
{
  DEBUG((stderr, "JUMPA\n"));
  JUMP(MA);
  URET;
}

static UDEF(uop_jumpge)
{
  DEBUG((stderr, "JUMPGE\n"));
  SIGN_EXTEND (AR);
  if (AR >= 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_jumpn)
{
  DEBUG((stderr, "JUMPN\n"));
  if (AR != 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_jumpg)
{
  DEBUG((stderr, "JUMPG\n"));
  SIGN_EXTEND (AR);
  if (AR > 0)
    JUMP(MA);
  URET;
}

static UDEF(uop_tlo)
{
  DEBUG((stderr, "TLO\n"));
  AR |= BR;
  URET;
}

static UDEF(uop_hll)
{
  DEBUG((stderr, "HLL\n"));
  AR = (BR & 0777777000000LL) | (AR & 0777777);
  URET;
}

static UDEF(uop_hrl)
{
  DEBUG((stderr, "HRL\n"));
  DEBUG((stderr, "AR %012llo, BR %012llo\n", AR, BR));
  AR = ((BR & 0777777LL) << 18) | (AR & 0777777);
  URET;
}

static UDEF(uop_hlr)
{
  DEBUG((stderr, "HLR\n"));
  AR = (AR & 0777777000000LL) | ((BR >> 18) & 0777777);
  URET;
}

static UDEF(uop_hrr)
{
  DEBUG((stderr, "HRR\n"));
  AR = (AR & 0777777000000LL) | (BR & 0777777);
  URET;
}

static UDEF(uop_hllo)
{
  DEBUG((stderr, "HLLO\n"));
  AR = (BR & 0777777000000LL) | 0777777LL;
  URET;
}

static UDEF(uop_hrlo)
{
  DEBUG((stderr, "HRLO\n"));
  AR = ((BR & 0777777LL) << 18) | 0777777LL;
  URET;
}

static UDEF(uop_hlro)
{
  DEBUG((stderr, "HLRO\n"));
  AR = 0777777000000LL | ((BR >> 18) & 0777777);
  URET;
}

static UDEF(uop_hrro)
{
  DEBUG((stderr, "HRRO\n"));
  AR = 0777777000000LL | (BR & 0777777);
  URET;
}

static UDEF(uop_hlle)
{
  DEBUG((stderr, "HLLE\n"));
  AR = (BR & 0400000000000LL) ? 0777777LL : 0;
  AR |= BR & 0777777000000LL;
  URET;
}

static UDEF(uop_hrle)
{
  DEBUG((stderr, "HRLE\n"));
  AR = (BR & 0400000LL) ? 0777777LL : 0;
  AR |= (BR & 0777777LL) << 18;
  URET;
}

static UDEF(uop_hlre)
{
  DEBUG((stderr, "HLRE\n"));
  AR = (BR & 0400000000000LL) ? 0777777000000LL : 0;
  AR |= (BR >> 18) & 0777777;
  URET;
}

static UDEF(uop_hrre)
{
  DEBUG((stderr, "HRRE\n"));
  AR = (BR & 0400000LL) ? 0777777000000LL : 0;
  AR |= BR & 0777777;
  URET;
}

/* Table to decode an opcode into an operation uop. */
static uop operate[] = {
  /* 000-077 */
  uop_muuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, 
  uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, 
  uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, 
  uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, uop_luuo, 
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
  0, 0, 0, 0, 0, 0, 0, 0, // IMUL, MUL
  0, 0, 0, 0, 0, 0, 0, 0, // IDIV, DIV
  uop_ash, uop_rot, uop_lsh, uop_jffo, uop_ashc, uop_rotc, uop_lshc, 0, 
  uop_exch, uop_blt, uop_aobjp, uop_aobjn, uop_jrst, uop_jfcl, uop_xct, 0, 
  uop_pushj, uop_push, uop_pop, uop_popj, uop_jsr, uop_jsp, uop_jsa, uop_jra,
  uop_add, uop_add, uop_add, uop_add, uop_sub, uop_sub, uop_sub, uop_sub,
  /* 300-377 */
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // CAI
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // CAM
  uop_nop, uop_jumpl, uop_jumpe, uop_jumple, uop_jumpa, uop_jumpge, uop_jumpn, uop_jumpg, // JUMP
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // SKIP
  uop_nop, uop_jumpl, uop_jumpe, uop_jumple, uop_jumpa, uop_jumpge, uop_jumpn, uop_jumpg, // AOJ
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // AOS
  uop_nop, uop_jumpl, uop_jumpe, uop_jumple, uop_jumpa, uop_jumpge, uop_jumpn, uop_jumpg, // SOJ
  uop_nop, uop_skipl, uop_skipe, uop_skiple, uop_skipa, uop_skipge, uop_skipn, uop_skipg, // SOS
  /* 400-477 */
  uop_setz, uop_setz, uop_setz, uop_setz, uop_and, uop_and, uop_and, uop_and, 
  uop_andca, uop_andca, uop_andca, uop_andca, uop_nop, uop_nop, uop_nop, uop_nop, 
  uop_andcm, uop_andcm, uop_andcm, uop_andcm, uop_nop, uop_nop, uop_nop, uop_nop, 
  uop_xor, uop_xor, uop_xor, uop_xor, uop_ior, uop_ior, uop_ior, uop_ior, 
  uop_andcb, uop_andcb, uop_andcb, uop_andcb, uop_eqv, uop_eqv, uop_eqv, uop_eqv, 
  uop_setca, uop_setca, uop_setca, uop_setca, uop_orca, uop_orca, uop_orca, uop_orca, 
  uop_setcm, uop_setcm, uop_setcm, uop_setcm, uop_orcm, uop_orcm, uop_orcm, uop_orcm, 
  uop_orcb, uop_orcb, uop_orcb, uop_orcb, uop_seto, uop_seto, uop_seto, uop_seto, 
  /* 500-577 */
  uop_hll, uop_hll, uop_hll, uop_hll, uop_hrl, uop_hrl, uop_hrl, uop_hrl,
  uop_hll, uop_hll, uop_hll, uop_hll, uop_hrl, uop_hrl, uop_hrl, uop_hrl,
  uop_hllo, uop_hllo, uop_hllo, uop_hllo, uop_hrlo, uop_hrlo, uop_hrlo, uop_hrlo,
  uop_hlle, uop_hlle, uop_hlle, uop_hlle, uop_hrle, uop_hrle, uop_hrle, uop_hrle,
  uop_hrr, uop_hrr, uop_hrr, uop_hrr, uop_hlr, uop_hlr, uop_hlr, uop_hlr,
  uop_hrr, uop_hrr, uop_hrr, uop_hrr, uop_hlr, uop_hlr, uop_hlr, uop_hlr,
  uop_hrro, uop_hrro, uop_hrro, uop_hrro, uop_hlro, uop_hlro, uop_hlro, uop_hlro,
  uop_hrre, uop_hrre, uop_hrre, uop_hrre, uop_hlre, uop_hlre, uop_hlre, uop_hlre,
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
static void decode (int a)
{
  upc_t upc = ops + USIZE*a;
  word_t opcode;
  IR = read_memory (a);

  opcode = (IR >> 27) & 0777;
#if !THREADED
  if (1 && (IR & 0777777000000) == 0254000000000LL) {
    //write_set_pc (&upc, IR & 0777777);
    write_jump (&upc, (uop)(ops + USIZE*(IR & 0777777)));
    write_nop (&upc);
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

#if THREADED
/* Retry executing the first uop after a decode operation. */
static UDEF(retry)
{
  DEBUG((stderr, "Retry %06o\n", PC));
  PC = ops[USIZE*PC](PC);
  URET;
}
#endif

/* Uop to decode a single word and execute it. */
static UDEF(decode_word)
{
  DEBUG((stderr, "\nDecode word\n"));
#if THREADED
  decode (PC);
#else
  decode (UPC-1);
#endif
  RETRY;
  URET;
}

/* Uop to decode a page and then resume execution. */
static UDEF(decode_page)
{
  int a = UPC & 0776000;
  DEBUG((stderr, "Decode page\n"));
  do {
    IR = read_memory (a);
    decode (a);
    a++;
  } while ((a & 01777) != 0);
  RETRY;
  URET;
}

/* Invalidate a single word. */
void invalidate_word (int a)
{
  upc_t upc = ops + USIZE*a;
  write_uop (&upc, decode_word);
}

/* Fill a page with a uop. */
static void fill_page (int a, uop op)
{
  int i;
  upc_t upc = &ops[USIZE*(a & 0776000)];
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
static UDEF(unmapped_word)
{
  DEBUG((stderr, "Unmapped word.\n"));
  exit (1);
  URET;
}

/* Fill an unmapped page. */
void unmapped_page (int a)
{
  fill_page (a, unmapped_word);
}

static UDEF(calculate_ea)
{
  int address, X, I;

  DEBUG((stderr, "\nCalculate EA (%p)\n", &address));
#if THREADED
  MB = IR = read_memory (UPC);
#else
  MB = IR = read_memory (UPC-1);
#endif
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
  URET;
}

#if !THREADED
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
#endif

#if THREADED
/* Execute one instruction. */
static int execute (int PC)
{
  upc_t upc = ops + USIZE*PC;
  PC = calculate_ea (PC);
  PC = (*upc++) (PC);
  PC = (*upc++) (PC);
  PC = (*upc++) (PC);
  URET;
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
    upc_t upc = ops + USIZE*PC;
    uop op = (uop)upc;
    op (0);
  }
#endif
}

typedef struct { int a; int b; } two;

two REGPARM foo (int x, int y)
{
  two c;
  c.a = x;
  c.b = y;
  return c;
}
