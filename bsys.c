/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

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

#define LMEDER  030 /* Length of media error data. */

static const char *sri_arc =
  "augmentation research center\r\n"
  "stanford research institute\r\n"
  "333 ravenswood drive\r\n"
  "menlo park, california 94025\r\n"
  "(415)326-6200";
static const char *sri_aic =
  "Artificial Intelligence Center\r\n"
  "Stanford Research Institute\r\n"
  "333 Ravenswood Drive\r\n"
  "Menlo Park, California 94025\r\n"
  "(415)326-6200";

#define MAX  515
static word_t block[MAX];
static int extract = 0;
static int tapver = 0;

static FILE *list;
static FILE *info;
static FILE *debug;

static FILE *output;
static word_t checksum;
static char file_path[100];
static struct timeval timestamp[2];
static char *trailer = " (NO TRAILER)";
static int file_argc;
static char** file_argv;

/* Convert a WAITS date to year, month, day. */
static void
compute_date (word_t date, int *year, int *month, int *day)
{
  *day = date % 31;
  *month = (date / 31) % 12;
  *year = (date / 31 / 12) + 1964;
}

/* Convert a WAITS date to a struct timeval. */
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

/* Print WAITS date and time. */
static void
print_timestamp (FILE *f, word_t date, word_t minutes)
{
  int year, month, day;
  compute_date (date, &year, &month, &day);
  fprintf (f, "%4d-%02d-%02d %02lld:%02lld",
	   year, month + 1, day + 1,
	   minutes / 60, minutes % 60);
}

/* Convert a Unix time_t to WAITS date and time. */
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
close_file (void)
{
  fprintf (debug, "\nCLOSE %s", file_path);
  fclose (output);
  output = NULL;
  utimes (file_path, timestamp);
}

/* Convert WAITS file name to an acceptable Unix name. */
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

/* Ensure PRJ and PRG names have the right size and padding to the left. */
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

/* Check that a file name from the command line has the right syntax. */
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

/* Convert a file name from the command line to a WAITS file name. */
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

static void
write_block (word_t *data, int size)
{
  int i;
  for (i = 0; i < size; i++)
    write_word (output, *data++);
}

static word_t
read_block (FILE *f, word_t word, int offset, word_t *data, int *size)
{
  int i;

  *size = offset;
  data[0] = word;

  for (i = offset; i < MAX; i++)
    {
      word = get_word (f);
      if (word == -1)
	return -1;
      if (word & (START_RECORD|START_FILE|START_TAPE))
	return word;
      data[i] = word & 0777777777777LL;
      (*size)++;
    }
  
  do
    word = get_word (f);
  while ((word & START_RECORD) == 0);
    
  return word & 0777777777777LL;
}

static word_t
read_header (FILE *f, word_t word, int *dblks)
{
  int length;
  word = read_block (f, word, 1, block, &length);

  if (length != 64)
    {
      fprintf (stderr, "Header record is %d words, not 64.\n", length + 1);
      exit (1);
    }

  fprintf (debug,     "tpblk:  %012llo\n", block[0]);
  fprintf (debug,     "tpnum:  %012llo\n", block[1]);
  fprintf (debug,     "nxtblk: %012llo\n", block[2]);
  fprintf (debug,     "usrblk: %012llo\n", block[3]);
  fprintf (debug,     "tpspac: %012llo\n", block[4]);
  fprintf (debug,     "tpfils: %012llo\n", block[5]);
  fprintf (debug,     "site:   %012llo\n", block[6]);
  fprintf (debug,     "pages:  %012llo\n", block[7]);
  fprintf (debug,     "tdpgs:  %012llo\n", block[8]);
  fprintf (debug,     "tpseq:  %012llo\n", block[9]);
  fprintf (debug,     "dblks:  %012o\n",   *dblks = block[10]);
  fprintf (debug,     "tapver: %012o\n",   tapver = block[11]);
  if (tapver > 0)
    {
      fprintf (debug, "lngfln: %012llo\n", block[12]);
      fprintf (debug, "lngnxt: %012llo\n", block[13]);
      fprintf (debug, "lngfst: %012llo\n", block[14]);
      fprintf (debug, "lngnpg: %012llo\n", block[15]);
    }
  fprintf (debug, "tptxt:  ...\n");    //18-33
  fprintf (debug, "tpid:   ...\n");    //34-63

  return word;
}

static word_t
read_directory (FILE *f, word_t word, int dblks)
{
  int i, length;

  for (i = 0; i < dblks; i++)
    word = read_block (f, word, 1, block, &length);

  return word;
}

static word_t
read_user (FILE *f)
{
  int length;
  word_t word = read_block (f, 0, 0, block, &length);
  return word;
}

static word_t
read_start (FILE *f, int file_number)
{
  int length;
  word_t word = read_block (f, 0, 0, block, &length);

  if (extract)
    {
      open_file ("", "", "", "");
    }

  return word;
}

static word_t
read_data (FILE *f, int file_number, int page_number)
{
  int length;
  word_t word = read_block (f, 0, 0, block, &length);

  if (extract)
    {
      write_block (block, 512);
    }

  return word;
}

static word_t
read_trailer (FILE *f)
{
  int length;
  word_t word = read_block (f, 0, 0, block, &length);
  trailer = "";
  return word;
}

static word_t
read_record (FILE *f, word_t word1)
{
  word_t word2 = get_word (f);
  int block_number = right (word1);
  int file_number = left (word2);
  int page_number = right (word2);

  if (word1 == -1)
    {
      fprintf (list, "\nPHYSICAL END OF TAPE%s", trailer);
      exit (0);
    }

  if (word1 & START_TAPE)
    {
      fprintf (list, "\nLOGICAL END OF TAPE%s\n", trailer);
      trailer = " (NO TRAILER)";
    }
  if (word1 & START_FILE)
    {
      close_file ();
    }

  fprintf (debug, "\nBlock #%d, File #%d, Page #%d",
	   block_number, file_number, page_number);

  if (word2 == 0777777777777)
    {
      int length;
      fprintf (debug, "\nRECORD: Unused directory file block");
      return read_block (f, 0, 0, block, &length);
    }
  else if (word2 == 0777777777776)
    {
      fprintf (debug, "\nRECORD: User data record");
      return read_user (f);
    }
  else if (page_number == 0777777)
    {
      fprintf (debug, "\nRECORD: Trailer");
      return read_trailer (f);
    }
  else if (page_number == 0777775)
    {
      fprintf (debug, "\nRECORD: File start");
      return read_start (f, file_number);
    }
  else
    {
      fprintf (debug, "\nRECORD: File page");
      return read_data (f, file_number, page_number);
    }
}

static void
read_tape (FILE *f)
{
  int dblks;

  if (f == NULL)
    f = stdin;

  // 

  word_t word = get_word (f);
  if (word == -1)
    {
      fprintf (stderr, "\nEMPTY TAPE");
      exit (1);
    }

  word = read_header (f, word, &dblks);
  word = read_directory (f, word, dblks);

  for (;;)
    word = read_record (f, word);
}

static void
write_header (FILE *f, word_t type)
{
  int length = 0;
  time_t now = waits_timestamp (time (NULL));
}

static void
write_data (FILE *f, FILE *input, int offset, word_t start)
{
  int i, length;
  //length = read_block (input, block, offset);
  length += offset;
  write_word (f, length | start);
  checksum = 0;
  for (i = 0; i < length; i++)
    {
      write_word (f, block[i]);
      checksum ^= block[i];
    }
  write_word (f, checksum);
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

  write_data (f, input, offset, START_FILE);
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

  write_header (f, HEAD);
  for (i = 0; i < file_argc; i++)
    write_file (f, file_argv[i]);
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
  int verbose = 0;
  FILE *f = NULL;
  int opt;

  input_word_format = &tape_word_format;
  output_word_format = &aa_word_format;

  /* If you ask for a file listing with -t or -xv, it's considered the
     output data and written to stdout.  Overriden by -c, see below. */
  list = stdout;
  info = debug = stderr;

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
	  /* Do not write file listing to stdout since it may mix with
	     tape data written to stdout. */
	  list = stderr;
	  mode = "wb";
	  break;
	case '7':
	  input_word_format = &tape7_word_format;
	  break;
	case '8':
	  input_word_format = &data8_word_format;
	  break;
	case '9':
	  input_word_format = &tape_word_format;
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
