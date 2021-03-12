/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

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

#include <time.h>
#include <utime.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "dis.h"

#define DART    0444162640000LL /* DART   */
#define HEAD    0125045414412LL /* *HEAD* */
#define TAIL    0126441515412LL /* *TAIL* */
#define PRMEND  0046045564404LL /* $PEND$ */

#define LMEDER  030

#define MAX 10240
static word_t block[MAX];
static int extract = 0;
static int verbose = 0;

static FILE *list;
static FILE *info;
static FILE *debug;

static FILE *output;
static word_t checksum;
static char file_path[100];
static struct timeval timestamp[2];
static int dart;
static int iover = 2;
static int trailer;
static int file_argc;
static char** file_argv;
static int tape_frames;
static int tape_bpw; /* Tape frames per word. */
static int tape_bpi;

static void
compute_date (word_t date, int *year, int *month, int *day)
{
  *day = date % 31;
  *month = (date / 31) % 12;
  *year = (date / 31 / 12) + 1964;
}

static void
unix_time (struct timeval *tv, word_t date, word_t minutes)
{
  struct tm tm;
  compute_date (date, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
  tm.tm_sec = 0;
  tm.tm_min = minutes % 60;
  tm.tm_hour = minutes / 60;
  tm.tm_mday++;
  tm.tm_year -= 1900;
  tm.tm_isdst = 0;
  tv->tv_sec = mktime (&tm);
  tv->tv_usec = 0;
}

static void
print_timestamp (FILE *f, word_t date, word_t minutes)
{
  int year, month, day;
  compute_date (date, &year, &month, &day);
  fprintf (f, "%4d-%02d-%02d %02lld:%02lld",
	   year, month + 1, day + 1,
	   minutes / 60, minutes % 60);
}

static word_t
waits_timestamp (time_t t)
{
  struct tm *tm = localtime (&t);
  int minutes = tm->tm_min + 60 * tm->tm_hour;
  int days = 31 * (12 * (tm->tm_year - 64) + tm->tm_mon) + tm->tm_mday - 1;
  return (days & 07777LL) | minutes << 12 | ((word_t)days & 070000) << 21;
}

static int
right (word_t word)
{
  return word & 0777777LL;
}

static int
left (word_t word)
{
  return right (word >> 18);
}

static void
print_rib (FILE *f, word_t *rib)
{
  word_t date, minutes;

  fprintf (f, "\nDDLOC:  %012llo", rib[004]); //location of first block
  if (rib[006] != 0)
    {
      fprintf (f, "\nReferenced: ");
      date = rib[006] & 07777;
      minutes = rib[006] >> 12;
      minutes &= 03777;
      print_timestamp (f, date, minutes);
    }
  fprintf (f, "\nDDMPTM: %012llo", rib[007]); //dump status
  fprintf (f, "\nDGRP1R: %012llo", rib[010]); //group 1
  fprintf (f, "\nDNXTGP: %012llo", rib[011]); //next group
  fprintf (f, "\nDSATID: %012llo", rib[012]); //SAT ID
  fprintf (f, "\nDQINFO: %012llo", rib[013]); //login
  if (iover >= 2)
    fprintf (f, "\nRecord offset: %012llo", rib[017]);
}

static void
print_ext (FILE *f, word_t *data, word_t type)
{
  word_t date, minutes;
  char string[7], prj[4], prg[4];

  if (data[000] != DART)
    fprintf (stderr, "\nEXPECTED DART");
  if (data[001] != type)
    fprintf (stderr, "\nEXPECTED *FILE* or CON,,IOVER");

  fprintf (f, "\n     Dumped: ");
  date = data[2] & 07777;
  if (dart >= 5)
    date |= (data[2] >> 21) & 070000;
  minutes = (data[2] >> 12) & 03777;
  print_timestamp (f, date, minutes);

  sixbit_to_ascii (data[3], string);
  strncpy (prj, string, 3);
  prj[3] = 0;
  strncpy (prg, string + 3, 3);
  prg[3] = 0;
  fprintf (f, " [%s,%s]", prj, prg);
  fprintf (f, " %d,,%d", left (data[4]), right (data[4]));
  fprintf (f, " %d/%d", left (data[5]), right (data[5]));
  fprintf (f, " %lld feet", data[6]);
  fprintf (f, " %lld words left", data[012]);
}

static void
print_mederr (FILE *f, word_t *mederr)
{
  int i;
  if (mederr[LMEDER - 1] != PRMEND)
    fprintf (stderr, "\nEXPECTED $PEND$");
  for (i = 0; i < LMEDER - 1; i++)
    {
      if (mederr[i] != 0)
	goto print;
    }
  return;
 print:
  fprintf (list, "   MEDERR");
  for (i = 0; i < LMEDER - 1; i++)
    {
      if ((i % 6) == 0)
	fputc ('\n', f);
      fprintf (f, " %012llo", mederr[i]);
    }
}  

static void
get_block (FILE *f, word_t *buffer, int words)
{
  int i;

  if (words > MAX)
    {
      fprintf (stderr, "\nEXPECTED SMALLER RECORD LENGTH %d", words);
      exit (1);
    }

  checksum = 0;
  for (i = 0; i < words; i++)
    {
      buffer[i] = get_word (f);
      if (buffer[i] == -1)
	{
	  fprintf (stderr, "\nUNEXPECTED END OF TAPE");
	  exit (1);
	}
      if (buffer[i] & (START_RECORD|START_FILE|START_TAPE))
	{
	  fprintf (stderr, "\nRecord too short.");
	  exit (1);
	}
      checksum ^= buffer[i];
    }
}

static void
close_file (void)
{
  fprintf (debug, "\nCLOSE %s", file_path);
  fclose (output);
  output = NULL;
  utimes (file_path, timestamp);
}

static char *
mangle (char *string)
{
  char *p;
  while (*string == ' ')
    string++;
  p = string;
  for (p = string; *p != 0; p++)
    {
      if (*p == ' ')
	*p = 0;
      *p = tolower (*p);
    }      
  return string;
}

static int
check_name (char *string, size_t m)
{
  return strlen (string) <= m;
}

static int
fix_name (char *string)
{
  int n = strlen (string);
  if (n > 3)
    return 0;
  memmove (string - n, string, n);
  *string = 0;
  return 1;
}

static int
names_ok (int n, char *prj, char *prg, char *nam, char *ext)
{
  if (n == 3)
    return check_name (nam, 6) &&
      fix_name (prg) &&
      fix_name (prj);
  if (n == 4)
    return check_name (nam, 6) &&
      check_name (ext, 3) &&
      fix_name (prg) &&
      fix_name (prj);
  return 0;
}

static void
unmangle (char *name, char *nam, char *ext, char *prj, char *prg)
{
  int n;

  if (strlen (name) > 18)
    {
      fprintf (stderr, "\nInput file name too long: %s", name);
      exit (1);
    }

  strcpy (prj, "   ");
  strcpy (prg, "   ");
  prj += 3;
  prg += 3;
  *ext = 0;

  n = sscanf (name, "%[^/]/%[^/]/%[^.].%s", prg, prj, nam, ext);
  if (n > 2 && names_ok (n, prg, prj, nam, ext))
    return;

  n = sscanf (name, "%[^.].%[^/]/%[^.].%s", prg, prj, nam, ext);
  if (names_ok (n, prg, prj, nam, ext))
    return;

  fprintf (stderr, "\nBad input file name: %s", name);
  exit (1);
}

static void
open_file (char *name, char *ext, char *prj, char *prg)
{
  name = mangle (name);
  ext = mangle (ext);
  prj = mangle (prj);
  prg = mangle (prg);

  fprintf (debug, "\nFile name: \"%s\"", name);
  fprintf (debug, "\nFile ext: \"%s\"", ext);
  fprintf (debug, "\nFile project: \"%s\"", prj);
  fprintf (debug, "\nFile programmer: \"%s\"", prg);

  if (mkdir (prg, 0777) == -1 && errno != EEXIST)
    fprintf (stderr, "\nError creating output directory %s: %s",
	     prg, strerror (errno));

  snprintf (file_path, sizeof file_path, "%s/%s", prg, prj);
  if (mkdir (file_path, 0777) == -1 && errno != EEXIST)
    fprintf (stderr, "\nError creating output directory %s: %s",
	     file_path, strerror (errno));

  strcat (file_path, "/");
  strcat (file_path, name);
  if (*ext != 0)
    {
      strcat (file_path, ".");
      strcat (file_path, ext);
    }

  fprintf (debug, "\nFILE: %s", file_path);
  output = fopen (file_path, "wb");
  if (output == NULL)
    fprintf (stderr, "\nError opening output file %s: %s",
	     file_path, strerror (errno));
}

static word_t
file_size (char *name)
{
  word_t size = 0;
  FILE *f = fopen (name, "rb");
  if (f == NULL)
    {
      fprintf (stderr, "\nError opening input file %s: %s",
	       name, strerror (errno));
      exit (1);
    }

  while (get_word (f) != -1)
    size++;
  fclose (f);
  fprintf (debug, "\nFile %s size: %lld", name, size);
  return size;
}

static word_t
rotchk (word_t x)
{
  checksum = (checksum << 1) | (checksum >> 35);
  checksum += x;
  checksum &= 0777777777777LL;
  return x;
}

static void
write_block (word_t *data, int size)
{
  int i;
  for (i = 0; i < size; i++)
    write_word (output, *data++);
}

static word_t
read_start (FILE *f, int length)
{
  word_t word, date, minutes;
  char string[8];
  char prj[4], prg[4];
  char name[7], ext[7];
  int offset = 022;

  get_block (f, block + 1, length);

  sixbit_to_ascii (block[1], string);
  string[6] = ' ';
  string[7] = ' ';
  *strchr (string, ' ') = ':';
  fprintf (list, "\n   %s", string);
  *strchr (string, ':') = ' ';
  sixbit_to_ascii (block[2], name);
  fprintf (list, "%s", name);
  sixbit_to_ascii (block[3], ext);
  ext[3] = 0;
  fprintf (list, ".%s", ext);

  sixbit_to_ascii (block[5], string);
  strncpy (prj, string, 3);
  prj[3] = 0;
  strncpy (prg, string + 3, 3);
  prg[3] = 0;
  fprintf (list, "[%s,%s]   ", prj, prg);

  fprintf (list, "%8lld   ", block[007]);

  date = block[4] & 07777;
  if (dart >= 5)
    date |= block[3] & 070000;
  minutes = (block[4] >> 12) & 03777;
  unix_time (&timestamp[0], date, minutes);
  unix_time (&timestamp[1], date, minutes);
  if (date != 0)
    print_timestamp (list, date, minutes);
  //fprintf (list, " [%ld]", ftell (f));

  print_rib (debug, &block[2]);

  if (iover >= 3)
    {
      print_ext (info, &block[022], ascii_to_sixbit ("*FILE*"));
      print_mederr (info, &block[length - LMEDER + 1]);
      offset = 044;
      length -= LMEDER;
    }

  if (extract)
    {
      open_file (name, ext, prj, prg);
      write_block (block + offset, length - offset + 1);
    }

  word = get_word (f);
  if (checksum != word)
    fprintf (stderr, "\nBad checksum: %012llo != %012llo", word, checksum);
  else
    fprintf (debug, "\nGood checksum: %012llo", checksum);

  word = get_word (f);
  if (left (word) != 0 && extract)
    close_file ();

  return word;
}

static word_t
read_data (FILE *f, int length)
{
  word_t word;
  int offset = 0;

  get_block (f, block, length);

  if (iover >= 3)
    {
      print_rib (debug, &block[1]);
      print_ext (debug, &block[021], ascii_to_sixbit ("CON   ") | iover);
      print_mederr (info, &block[length - LMEDER]);
      length -= LMEDER;
      offset = 043;
    }

  if (extract)
    write_block (block + offset, length - offset);

  word = get_word (f);
  if (checksum != word)
    fprintf (stderr, "\nBad checksum: %012llo != %012llo", word, checksum);
  else
    fprintf (debug, "\nGood checksum: %012llo", checksum);

  word = get_word (f);
  if (left (word) != 0 && extract)
    close_file ();
  return word;
}

static void
read_header (FILE *f, word_t word)
{
  int length, i;
  char string[7];
  char prj[4], prg[4];
  word_t date, minutes;

  length = right (word);
  get_block (f, block + 1, length);

  if (block[2] != HEAD && block[2] != TAIL)
    fprintf (stderr, "\nEXPECTED *HEAD* OR *TAIL*");
  trailer = (block[2] == TAIL);

  dart = left (word);
  fprintf (list, "\nDART VERSION %-2d TAPE %s",
	   dart, block[2] == HEAD ? "HEADER" : "TRAILER");

  if (block[1] != DART)
    fprintf (stderr, "\nEXPECTED DART MAGIC");

  sixbit_to_ascii (block[4], string);
  strncpy (prj, string, 3);
  prj[3] = 0;
  strncpy (prg, string + 3, 3);
  prg[3] = 0;
  fprintf (list, "\nRECORDED ");
  date = block[3] & 07777;
  if (dart >= 5)
    date |= (block[3] >> 21) & 070000;
  minutes = (block[3] >> 12) & 03777;
  print_timestamp (list, date, minutes);
  fprintf (list, ",  BY [%s,%s] %s CLASS",
	   prj, prg, left (block[5]) ? "SYSTEM" : "USER");
  if (left (block[5]))
    fprintf (list, " TAPE #%d", right (block[5]));
  if (block[2] == TAIL)
    {
      fputc ('\n', list);
      return;
    }

  if (length <= 5)
    return;
  if (length != 013)
    fprintf (stderr, "\nEXPECTED 13 WORD HEADER");
  fprintf (info, "\n     Tape %d/%d, position %lld",
	   left (block[6]), right (block[6]), block[7]);

  checksum = 0;
  for (i = 1; i < length; i++)
    rotchk (block[i]);
  if (block[013] != checksum)
    fprintf (stderr, "\nBad checksum: %012llo != %012llo",
	     block[013], checksum);
  else
    fprintf (debug, "\nGood checksum: %012llo", checksum);
}

static word_t
read_gap (FILE *f)
{
  word_t word = get_word (f);
  int i;

  for (i = 0; i < word; i++)
    {
      if (get_word (f) == -1)
	{
	  fprintf (stderr, "\nUNEXPECTED END OF TAPE");
	  exit (1);
	}
    }
  
  fprintf (list, "\n   GAP");
  fprintf (info, " %lld", word);
  return get_word (f);
}

static word_t
read_record (FILE *f, word_t word)
{
  int length = right (word);
  if (word == -1)
    {
      fprintf (list, "\nEND OF TAPE%s",
	       trailer ? "" : " (NO TRAILER)");
      exit (0);
    }
  trailer = 0;
  switch (left (word))
    {
    case 0:
      fprintf (debug, "\nRECORD: DATA");
      return read_data (f, length);
    case 0777775:
    case 0777776:
    case 0777777:
      iover = 01000000 - left (word);
      fprintf (debug, "\nRECORD: FILE (IOVER%d)", iover);
      return read_start (f, length);
    case 0777767:
      fprintf (debug, "\nRECORD: GAP");
      return read_gap (f);
    default:
      fprintf (debug, "\nRECORD: HEAD");
      if (left (word) < 10)
	{
	  read_header (f, word);
	  return get_word (f);
	}
      else
        {
          fprintf (stderr, "\nEXPECTED RECORD TYPE got %o", left (word));
          exit (1);
        }
    }
}

static void
read_tape (FILE *f)
{
  if (f == NULL)
    f = stdin;

  word_t word = get_word (f);
  if (word == -1)
    {
      fprintf (stderr, "\nEMPTY TAPE");
      exit (1);
    }
  for (;;)
    word = read_record (f, word);
}

static word_t
tape_feet (void)
{
  return tape_frames / tape_bpi / 12;
}

static void
write_header (FILE *f, word_t type)
{
  int length = iover < 3 ? 5 : 013;
  time_t now = waits_timestamp (time (NULL));
  if (dart < 5)
    now &= 037777777LL;

  checksum = 0;
  write_word (f, dart << 18 | length | START_FILE);
  write_word (f, rotchk (DART));
  write_word (f, rotchk (type));
  write_word (f, rotchk (now));
  write_word (f, rotchk (ascii_to_sixbit ("DMPSYS")));
  write_word (f, rotchk (0 | 0));

  tape_frames += tape_bpw * (length + 1);

  if (iover < 3)
    return;

  write_word (f, rotchk (1 << 18 | 1));
  write_word (f, rotchk (tape_feet ()));
  write_word (f, rotchk (0));
  write_word (f, rotchk (0777777777777LL));
  write_word (f, rotchk (0));
  write_word (f, checksum);
}

static int
read_block (FILE *f, word_t *data, int offset)
{
  int i, max, size = 0;
  word_t word;

  max = iover < 3 ? 1280 : 10240 - 030;
  max -= 2; /* Leave room for length and checksum words. */

  for (i = offset; i < max; i++)
    {
      word = get_word (f);
      if (word == -1)
	return size;
      data[i] = word;
      size++;
    }
  
  return size;
}

static void
write_data (FILE *f, FILE *input, int offset, word_t start)
{
  int i, length;
  length = read_block (input, block, offset);
  length += offset;
  if (iover >= 3)
    {
      block[027] = tape_feet ();
      memset (block + length, 0, 030 * sizeof (word_t));
      length += 030;
      block[length - 1] = PRMEND;
    }
  write_word (f, length | start);
  checksum = 0;
  for (i = 0; i < length; i++)
    {
      write_word (f, block[i]);
      checksum ^= block[i];
    }
  write_word (f, checksum);
  if (iover >= 3)
    block[033] -= length - offset; /* Update file words left. */
  tape_frames += tape_bpw * (length + 2);
  tape_frames += 3 * tape_bpi / 5;
}

static void
write_file (FILE *f, char *name)
{
  char nam[18], ext[18], prj[18], prg[18];
  word_t timestamp;
  struct stat st;
  int offset = 021;
  FILE *input = fopen (name, "rb");
  if (input == NULL)
    {
      fprintf (stderr, "\nError opening input file %s: %s",
	       name, strerror (errno));
      return;
    }

  if (fstat (fileno (input), &st) == -1)
    fprintf (stderr, "\nError calling stat on file %s: %s",
	     name, strerror (errno));
  timestamp = waits_timestamp (st.st_mtime);

  unmangle (name, nam, ext, prj, prg);
  block[0] = ascii_to_sixbit ("DSK   ");
  block[1] = ascii_to_sixbit (nam);
  block[2] = ascii_to_sixbit (ext);
  block[3] = timestamp & 037777777LL;
  if (dart >= 5)
    block[2] |= (timestamp >> 21) & 070000LL;
  block[4] = ascii_to_sixbit (prj) | ascii_to_sixbit (prg) >> 18;
  fprintf (debug, "\nFILE: %s.%s[%s,%s]", nam, ext, prj, prg);

  block[005] = 0; //ddloc location of first block
  block[006] = file_size (name);
  block[007] = 0; //dreftm, referenced
  block[010] = 0; //ddmptm, dump time
  block[011] = 1; //dgrp1r, group 1
  block[012] = 0; //dnxtgp, next group
  block[013] = 0; //dsatid, SAT ID
  block[014] = 0; //dqinfo, login
  block[015] = block[016] = block[017] = 0;
  if (iover == 1)
    block[020] = 0;
  else
    block[020] = 1;

  if (iover >= 3)
    {
      block[021] = DART;
      block[022] = ascii_to_sixbit ("*FILE*");
      block[023] = waits_timestamp (time (NULL));
      block[024] = ascii_to_sixbit ("DMPSYS");
      block[025] = 0 | 0;
      block[026] = 1 << 18 | 1;
      block[027] = 0;
      block[030] = 0;
      block[031] = 0777777777777LL;
      block[032] = 0;
      block[033] = block[006];
      memset (block + 034, 0, 7 * sizeof (word_t));
      offset = 043;
    }

  tape_frames += 3 * tape_bpi; /* Tape mark. */
  write_data (f, input, offset, (01000000 - iover) << 18 | START_FILE);
  if (iover >= 3)
    block[022] = ascii_to_sixbit ("CON   ") | iover;
  else
    offset = 0;
  while (!feof (input))
    write_data (f, input, offset, START_RECORD);
  fclose (input);
}

static void
write_tape (FILE *f)
{
  int i;
  struct word_format *tmp = input_word_format;
  input_word_format = output_word_format;
  output_word_format = tmp;

  if (f == NULL)
    f = stdout;

  dart = 5;
  tape_frames = 0;
  write_header (f, HEAD);
  for (i = 0; i < file_argc; i++)
    write_file (f, file_argv[i]);
  tape_frames += 3 * tape_bpi; /* Tape mark. */
  write_header (f, TAIL);
  flush_word (f);
}

static void
usage (const char *x)
{
  fprintf (stderr,
	   "Usage: %s -c|-t|-x [-v789] [-Wformat] [-Cdir] [-f file]\n", x);
  usage_word_format ();
  exit (1);
}

static void
newline (void)
{
  if (list)
    fputc ('\n', list);
}

int
main (int argc, char **argv)
{
  void (*process_tape) (FILE *) = NULL;
  char *tape_name = NULL, *mode;
  char *directory = NULL;
  FILE *f;
  int opt;

  tape_bpi = 800;
  input_word_format = &tape7_word_format;
  output_word_format = &aa_word_format;

  while ((opt = getopt (argc, argv, "ctvx123789f:W:C:")) != -1)
    {
      switch (opt)
	{
	case 'f':
	  if (tape_name != NULL)
	    {
	      fprintf (stderr, "Just one -f allowed.\n");
	      exit (1);
	    }
	  tape_name = optarg;
	  break;
	case 't':
	  if (process_tape != NULL)
	    {
	      fprintf (stderr, "Just one of -c, -t, or -x allowed.\n");
	      exit (1);
	    }
	  process_tape = read_tape;
	  mode = "rb";
	  verbose++;
	  break;
	case 'v':
	  verbose++;
	  break;
	case 'x':
	  if (process_tape != NULL)
	    {
	      fprintf (stderr, "Just one of -c, -t, or -x allowed.\n");
	      exit (1);
	    }
	  process_tape = read_tape;
	  mode = "rb";
	  extract = 1;
	  break;
	case 'c':
	  if (process_tape != NULL)
	    {
	      fprintf (stderr, "Just one of -c, -t, or -x allowed.\n");
	      exit (1);
	    }
	  process_tape = write_tape;
	  mode = "wb";
	  break;
	case '1':
	case '2':
	case '3':
	  iover = opt - '0';
	  dart = iover + 3;
	  break;
	case '7':
	  input_word_format = &tape7_word_format;
	  tape_bpw = 6;
	  tape_bpi = 800;
	  break;
	case '8':
	  input_word_format = &data8_word_format;
	  tape_bpw = 5;
	  tape_bpi = 6250;
	  break;
	case '9':
	  input_word_format = &tape_word_format;
	  tape_bpw = 5;
	  tape_bpi = 6250;
	  break;
	case 'W':
	  if (parse_output_word_format (optarg))
	    usage (argv[0]);
	  break;
	case 'C':
	  if (mkdir (optarg, 0777) == -1 && errno != EEXIST)
	    {
	      fprintf (stderr, "\nError creating directory %s: %s",
		       optarg, strerror (errno));
	      exit (1);
	    }
	  directory = optarg;
	  break;
	default:
	  usage (argv[0]);
	}
    }

  if (process_tape == NULL)
    usage (argv[0]);

  list = info = debug = stdout;
  if (verbose < 1)
    list = fopen ("/dev/null", "w");
  if (verbose < 2)
    info = fopen ("/dev/null", "w");
  if (verbose < 3)
    debug = fopen ("/dev/null", "w");

  if (tape_name && (f = fopen (tape_name, mode)) == NULL)
    {
      fprintf (stderr, "Error opening input %s: %s\n",
	       optarg, strerror (errno));
      exit (1);
    }

  if (directory && chdir (directory) == -1)
    {
      fprintf (stderr, "\nError entering directory %s: %s",
	       directory, strerror (errno));
      exit (1);
    }

  file_argc = argc - optind;
  file_argv = argv + optind;
  atexit (newline);
  process_tape (f);

  return 0;
}
