#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NPACKS  1      //Number of packs in the structure.
#define NSWCYL  10     //Cylinders for swapping, per unit.
#define RECSIZ  0200   //Size of a disk record, or sector.
#define PAGSIZ  01000  //Size of a page.
#define MFDSIZ  2048   //Entries in MFD.

#define SETZ    400000000000ULL

/* Disk geometry; this is for RP06. */
#define NRECTK  18   //Logical records per track.
#define NTRKCY  19   //Surfaces.
#define NUMCYL  815  //Cylinders.
#define BLKSIZ  9    //Records per block, including retrieval area.
#define PRECTK  20   //Pysical records per track.

/* Derived from above. */
#define NRECPG  (PAGSIZ/RECSIZ)
#define NPPCYL  (NRECTK*NTRKCY/NRECPG)
#define BLKCYL  (NPPCYL*NRECPG/BLKSIZ)
#define RECCYL  (NRECPG*NPPCYL)
#define NRECUN  (RECCYL*NUMCYL)
//#define MFDLOC  ((FSWCYL+NSWCYL)*RECCYL)
#define MFDLOC  (SATLOC+SATSIZ)
#define FSWCYL  ((NUMCYL-NSWCYL)/2)
#define BLKSTR  (BLKCYL*NUMCYL*NPACKS)
#define SATLOC  (0*NRECUN+(FSWCYL+NSWCYL)*RECCYL)
#define SATSIZ  (((BLKSTR+35)/36+RECSIZ-1)/RECSIZ+1)

typedef unsigned long long word_t;

static char *strnam = "RSK   ";
static word_t block[RECSIZ];
static word_t sat[RECSIZ*SATSIZ];
static word_t *satbit = &sat[RECSIZ];
static word_t mfd[MFDSIZ*16];

static word_t sixbit(const char *string)
{
  int i;
  word_t data = 0;
  for (i = 0; i < 6; i++) {
    unsigned char c = string[i];
    data <<= 6;
    c -= 32;
    if (c > 64)
      c -= 32;
    data |= c;
  }
  return data;
}

static word_t halves(int left, int right)
{
  word_t data = left;
  data <<= 18;
  data |= right;
  return data;
}

static int record_to_block(long address)
{
  long block = (address/RECCYL) * BLKCYL;
  block += (address%RECCYL) / BLKSIZ;
  return block;
}

static void allocate_record(long address)
{
  long block = record_to_block(address);
  if (block >= BLKSTR) {
    fprintf(stderr, "Allocating block outside structure.\n");
    exit(1);
  }
  word_t mask = SETZ >> (block%36);
  satbit[block/36] &= ~mask;
  sat[3] ^= mask;
  sat[0]++;
}

static void allocate_records(long address, long end)
{
  while (address < end)
    allocate_record(address++);
}

static void make_hom(word_t home, int unit)
{
  memset(block, 0, sizeof block);
  block[0000] = sixbit("HOM   ");
  block[0001] = 0;
  block[0002] = 0;
  block[0003] = sixbit(strnam);
  block[0004] = halves(NPACKS, unit);
  block[0005] = home;
  block[0006] = NPPCYL*NSWCYL;
  block[0007] = FSWCYL;
  block[0010] = MFDLOC;
  block[0011] = SATLOC;
  block[0012] = SATSIZ;
  block[0013] = NRECUN;
  block[0014] = NPACKS*NUMCYL;
  block[0015] = 0;
  //0061 fe address
  //0062 fe size
  //0101 bootstrap address
  //0102 bootstrap size
  //0164 cpu serial number
  //0165 unit ID
  //0170 owner ID
  block[0173] = 020040040527; //File system type.
  block[0174] = 052111020123; //"WAITS    "
  block[0175] = 020040020040; //in weird PDP-11 format.
  block[0176] = 0707070;
  block[0177] = home & 0777777;
}

static void make_bat(void)
{
  memset(block, 0, sizeof block);
  block[0000] = sixbit("BAT   ");
  block[0001] = 077760600004;
  block[0002] = 0550000;
  block[0176] = 0606060;
  block[0177] = 2;
}

static void make_sat(void)
{
  memset(sat, 0xFF, sizeof sat);
  memset(sat, 0, RECSIZ * sizeof(word_t));
  sat[0000] = 0;
  sat[0001] = 0;
  sat[0002] = sixbit(strnam);
  sat[0003] = 0; //Xor of SAT bits.
  sat[0005] = 0; //Checksum of bad block table.
  sat[0063] = sixbit("SATID ");
  allocate_records(SATLOC, SATLOC + SATSIZ);
}

static void make_mfd(void)
{
  memset(mfd, 0, sizeof mfd);
  mfd[0000] = sixbit("  1  1");
  mfd[0001] = sixbit("UFD   ");
  mfd[0002] = halves(0155740, 0);
  mfd[0003] = sixbit("  1  1");
  mfd[0004] = MFDLOC;
  mfd[0005] = MFDSIZ * 16;
  mfd[0010] = 1;
  mfd[0012] = sixbit(strnam);
  mfd[0020] = MFDLOC;
  allocate_records(MFDLOC, MFDLOC + MFDSIZ * 16 / RECSIZ);
}

static void write_word(FILE *f, word_t data)
{
  int i;
  /* This writes the SIMH disk format: 36 bits as a little endian
     64-bit number. */
  for (i = 0; i < 8; i++) {
    fputc(data & 0xFF, f);
    data >>= 8;
  }
}

/* Write a block, converting from a linear address to a location in
   the disk image.  The linear address into on-disk data may store
   fewer records on a track then the disk physically holds, and a page
   must not cross a cylinder boundary. */
static void write_block(FILE *f, long address, word_t *data)
{
  int i;
  int sector = address % NRECTK;
  int page = address / 4;
  int head = (page / NRECTK) % NTRKCY;
  int cylinder = page / NPPCYL;
  address -= cylinder * NPPCYL * NRECPG;
  head = address / NRECTK;
  sector = address % NRECTK;
  address = cylinder * PRECTK * NTRKCY;
  address += head * PRECTK;
  address += sector;
  fseek(f, address * RECSIZ * 8, SEEK_SET);
  for (i = 0; i < RECSIZ; i++)
    write_word(f, data[i]);
}

static void write_blocks(FILE *f, long address, word_t *data, int blocks)
{
  int i;
  for (i = 0; i < blocks; i++) {
    write_block(f, address, data);
    address++;
    data += RECSIZ;
  }
}

static void fatal(const char *message)
{
  fprintf(stderr, "%s\n", message);
  exit(1);
}

static void usage(const char *name)
{
  fprintf(stderr, "%s: <output file> <unit number>\n", name);
  exit(1);
}

int main(int argc, char **argv)
{
  FILE *f;

  if (argc != 3)
    usage(argv[0]);

  int unit = atoi(argv[2]);

  f = fopen(argv[1], "wb");
  if (f == NULL)
    fatal("Could not open output file.");

  memset(block, 0, sizeof block);
  write_block(f, NRECUN-1, block);

  make_sat();

  allocate_records(FSWCYL*RECCYL, (FSWCYL+NSWCYL)*RECCYL);

  make_hom(halves(10, 1), unit);
  write_block(f, 1, block);
  make_hom(halves(1, 10), unit);
  write_block(f, 10, block);
  make_bat();
  write_block(f, 2, block);
  allocate_records(0, 11);

  make_mfd();
  write_blocks(f, MFDLOC, mfd, MFDSIZ * 16 / RECSIZ);

  write_blocks(f, SATLOC, sat, SATSIZ);
  fclose(f);
  return 0;
}
