/* Copyright (C) 2019 Lars Brinkhoff <lars@nocrew.org>

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

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "dis.h"

extern char * dis11 (unsigned short, unsigned short);
extern void *mem;

static word_t sector0[128];
static word_t sector1[128];
static word_t sector10[128];

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s home.blocks primry.boot\n", x);
  exit (1);
}

static void
word_to_ascii (word_t w, char *string)
{
  string[0] = (w >> 26) & 0377;
  string[1] = (w >> 18) & 0377;
  string[2] = (w >>  8) & 0377;
  string[3] = (w >>  0) & 0377;
  string[4] = 0;
}

static void
squoze11_to_ascii (int squoze, char *ascii)
{
  static char table[] =
    {
      ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
      'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
      'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
      'X', 'Y', 'Z', '$', '.', '%', '0', '1',
      '2', '3', '4', '5', '6', '7', '8', '9'
    };
  int i;

  squoze &= 0177777;

  for (i = 0; i < 3; i++)
    {
      ascii[2-i] = table[squoze % 40];
      squoze /= 40;
    }
  ascii[3] = 0;
}

static int foo32 (word_t w)
{
  return ((w >> 2) & 0xFFFF0000) | (w & 0xFFFF);
}

static word_t insn_data;
unsigned short fetch (void)
{
  unsigned short x = insn_data >> 18;
  x &= 0177777;
  insn_data <<= 18;
  return x;
}

static void dis11_sector (word_t *sector, int addr)
{
  int i, a;

  for (i = 0; i < 128; i++)
    {
      a = addr + 4*i;
      insn_data = sector[i] & 0177777;
      insn_data <<= 18;
      insn_data |= sector[i+1] >> 18;
      printf ("%04o  %06llo  %s\n",
	      a, sector[i] >> 18, dis11 (a, sector[i] >> 18));
      a += 2;
      insn_data = sector[i+1];
      printf ("%04o  %06llo  %s\n",
	      a, sector[i] & 0177777, dis11 (a, sector[i] & 0177777));
    }
}

static void
read_sector (FILE *f, word_t *words)
{
  int i;
  for (i = 0; i < 128; i++)
    words[i] = get_word (f);
  if (words[127] == -1)
    fprintf (stderr, "WARNING: truncated block\n");
}

static void
read_home_blocks (FILE *f)
{
  word_t sector[128];
  char string[7];
  int i;

  printf ("SECTOR 0\n");
  read_sector (f, sector0);
  printf ("Primary bootstrap cylinder: %3lld\n", sector0[020] & 0777777);
  dis11_sector (sector0, 0);

  printf ("SECTOR 1\n");
  read_sector (f, sector1);
  for (i = 0; i < 128; i += 4)
    {
      printf ("%03o  %06llo,,%06llo", i, sector1[i] >> 18, sector1[i] & 0777777);
      printf ("  %06llo,,%06llo", sector1[i+1] >> 18, sector1[i+1] & 0777777);
      printf ("  %06llo,,%06llo", sector1[i+2] >> 18, sector1[i+2] & 0777777);
      printf ("  %06llo,,%06llo\n", sector1[i+3] >> 18, sector1[i+3] & 0777777);
    }
  printf ("HOME BLOCK\n");
  sixbit_to_ascii (sector1[0], string);
  printf ("Magic: %s\n", string);
  printf ("Directory: %012llo, %012llo, %012llo\n",
	  sector1[061], sector1[062], sector1[063]);
  printf ("LH(64): %6lld\n", sector1[064] >> 18);
  printf ("Index cylinder: %6lld\n", sector1[064] & 0777777);
  printf ("Index PBN: %6lld\n", sector1[065] >> 18);
  printf ("Index NCB: %6lld\n", sector1[065] & 0777777);
  printf ("Directory cylinder: %6lld\n", sector1[066] >> 18);
  printf ("Directory PBN: %6lld\n", sector1[066] & 0777777);
  printf ("Directory NCB: %6lld\n", sector1[067] >> 18);
  printf ("Bit map cylinder: %6lld\n", sector1[067] & 0777777);
  printf ("Bit map PBN: %6lld\n", sector1[070] >> 18);
  printf ("Bit map NCB: %6lld\n", sector1[070] & 0777777);
  printf ("Bad block PBN: %6lld\n", sector1[071] >> 18);
  printf ("Bad block NCB: %6lld\n", sector1[071] & 0777777);
  printf ("Extended home block PBN: %6lld\n", sector1[072] >> 18);
  printf ("Extended home block NCB: %6lld\n", sector1[073] & 0777777);
  word_to_ascii (sector1[074], string);
  printf ("Function: %s", string);
  word_to_ascii (sector1[075], string);
  printf ("%s", string);
  word_to_ascii (sector1[076], string);
  printf ("%s\n", string);
  word_to_ascii (sector1[077], string);
  printf ("Owner: %s", string);
  word_to_ascii (sector1[0100], string);
  printf ("%s", string);
  word_to_ascii (sector1[0101], string);
  printf ("%s\n", string);

  /* 2 - 9 */
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);

  printf ("SECTOR 10\n");
  read_sector (f, sector10);

  if (memcmp (sector1, sector10, sizeof sector1) != 0)
    fprintf (stderr, "WARNING: Home blocks doesn't match\n");

  /* 11 - 15 */
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);
  read_sector (f, sector);

  if (get_word (f) != -1)
    fprintf (stderr, "WARNING: Excess data in Home blocks\n");
}

static void
read_primary_boot (FILE *f)
{
  word_t sector[128];

  printf ("PRIMARY BOOT\n");

  read_sector (f, sector);
  dis11_sector (sector, 0);

  /* 1 */
  read_sector (f, sector);
  dis11_sector (sector, 01000);
}

static void
read_directory (FILE *f)
{
  word_t sector[128];
  char string[7];
  int n, i;
  int cyl, sur, sec;

  printf ("DIRECTORY\n");

  for (n = 0; n < 64; n++)
    {
      read_sector (f, sector);

      /*
DIRPBN==BUF+2		;PHYS BLOCK NUMBER, BYTE(18)CYL(10)SURF(8)SECT
DIRALC==BUF+3		;WORDS ALLOC LH MOST SIGNIFICANT 16 BITS, RH LEAST
DIRWRT==BUF+4		;WORDS WRITTEN SAME FORMAT AS DIRALC
DIRLOD==BUF+5		;LH 11 LOAD ADR, RH 11 START ADDR
DIRSTS==BUF+6		;LH "FILE TYPE & STATUS", RH CHECKSUM
DIR7==BUF+7		;RESERVED MUST BE ZERO
      */

      for (i = 0; i < 128; i += 8)
	{
	  if (sector[i] == 0777776777776LL)
	    continue;
	  squoze11_to_ascii (sector[i] >> 18, string);
	  squoze11_to_ascii (sector[i], string+3);
	  printf ("File: %s", string);
	  squoze11_to_ascii (sector[i+1] >> 18, string);
	  printf (" %s", string);
	  cyl = sector[i+2] >> 16;
	  sur = (sector[i+2] >> 8) & 01777;
	  sec = sector[i+2] & 0177;;
	  printf ("  @(%d,%d,%d)", cyl, sur, sec);
	  printf ("  %d", foo32(sector[i+4]));
	  printf ("/%d\n", foo32(sector[i+3]));
	}
    }
}

int
main (int argc, char **argv)
{
  FILE *f;

  file_36bit_format = FORMAT_ITS;

  if (argc != 4)
    usage (argv[0]);

  f = fopen (argv[1], "rb");
  read_home_blocks (f);
  fclose (f);

  f = fopen (argv[2], "rb");
  read_primary_boot (f);
  fclose (f);

  f = fopen (argv[3], "rb");
  read_directory (f);
  fclose (f);

  return 0;
}
