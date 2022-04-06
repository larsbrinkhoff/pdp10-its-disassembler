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

/* Record types. */
#define DATA  0  /* Contents of file page. */
#define TPHD  1  /* Saveset header. */
#define FLHD  2  /* File header. */
#define FLTR  3  /* File trailer. */
#define TPTR  4  /* Tape trailer. */
#define USR   5  /* User directory. */
#define CTPH  6  /* Continued saveset header. */
#define FILL  7  /* Padding. */

static const char *type_name[] =
  {
    "DATA", "TPDH", "FLDH", "FLTR", "TPTR", "USR", "CTPH", "FILL"
  };

#define MAX  518
static word_t block[MAX];
static int extract = 0;

static FILE *list;
static FILE *info;
static FILE *debug;

static FILE *output;
static char file_path[100];
static struct timeval timestamp[2];
static int file_argc;
static char** file_argv;
static int saveset_number;
static int tape_number;
static int file_number;
static int page_number;
static int record_number;
static int bfmsg = 3;
static int format = 4;

/*
Format:
0 - BBN format.
3 - Full dir spec.
4 - Lowest legal for last DUMPER.
5 - Password encryption.
6 - "Tonext" record type.
*/

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

/* Convert from TOPS-20 timestamp. */
static void
unix_timestamp (struct timeval *tv, word_t timestamp)
{
  long days = left (timestamp);
  long ticks = right (timestamp);
  if (timestamp == 0)
    {
      tv->tv_sec = 0;
      tv->tv_usec = 0;
      return;
    }
  days -= 0117213;

  tv->tv_sec = 24*60*60*days;
  if (format > 0)
    {
      tv->tv_sec += (675*ticks) / 2048;
      tv->tv_usec = (675*ticks) % 2048;
      tv->tv_usec *= 15625;
      tv->tv_usec /= 32;
    }
  else
    {
      tv->tv_sec += ticks;
      tv->tv_usec = 0;
    }
}

/* Print TOPS-20 timestamp. */
static void
print_timestamp (FILE *f, word_t timestamp)
{
  char string[100];
  struct timeval tv;
  time_t t;
  struct tm *tm;

  if (timestamp == 0)
    {
      fprintf (f, "****-**-** **:**");
      return;
    }

  unix_timestamp (&tv, timestamp);
  t = tv.tv_sec;
  tm = gmtime (&t);
  strftime (string, sizeof string, "%Y-%m-%d %H:%M:%S", tm);
  if (1)
    fputs (string, f);
  else
    fprintf (f, "%s.%02ld", string, tv.tv_usec / 10000);
}

/* Convert a Unix time_t to TOPS-20 or TENEX timestamp. */
static word_t
tops20_timestamp (time_t t)
{
  long days, ticks;

  if (t == 0 || t == -1)
    return 0;

  days = t / (24*60*60);
  days += 0117213;

  ticks = t % (24*60*60);
  if (format > 0)
    {
      ticks *= 2048;
      ticks /= 675;
    }

  return (days << 18) | ticks;
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

static char *
find (char **string, const char *required, char *fail)
{
  char *start = *string;
  char *p = start;

  if (*p == 0)
    {
      *string = p;
      return NULL;
    }

  do
    {
      if (strchr (required, *p))
	{
	  *p = 0;
	  *string = p + 1;
	  return start;
	}
      p++;
    }
  while (*p);

  if (fail != NULL)
    *string = p;
  return fail;
}

/* Convert a file name from the command line to a TOPS-20 file name. */
static const char *
unmangle (char *file_name, char *device, char *name,
	  int protection, const char *author)
{
  char original_name[100];
  char *directories[100];
  char *file, *type, *version;
  char **d = &directories[0];
  char *sep = "";

  strcpy (original_name, name);

  do
    *d = find (&name, "/", NULL);
  while (*d++ != NULL);

  if (d - directories > 1 && format == 0)
    return "TENEX doesn't support subdirectories";

  file = find (&name, ".", name);
  type = find (&name, ".;", name);
  version = find (&name, ";", name);

  if (device)
    file_name += sprintf (file_name, "%s:", device);
  if (directories[0] != NULL)
    {
      file_name += sprintf (file_name, "<");
      for (d = &directories[0]; *d != NULL; d++, sep = ".")
	file_name += sprintf (file_name, "%s%s", sep, *d);
      file_name += sprintf (file_name, ">");
    }
  if (file)
    file_name += sprintf (file_name, "%s", file);
  if (type)
    file_name += sprintf (file_name, ".%s", type);
  if (version)
    file_name += sprintf (file_name, "%c%s", format == 0 ? ';' : '.', version);
  file_name += sprintf (file_name, ";P%06o", protection);
  if (author)
    file_name += sprintf (file_name, ";A%s", author);

  return NULL;
}

static void
open_file (char *name, char *ext, char *prj, char *prg)
{
  name = mangle (name);
  ext = mangle (ext);
  prj = mangle (prj);
  prg = mangle (prg);

#if 0
  dev = find (&name, ":", NULL);
  if (dev == NULL)
    dev = device;
  if (*name == '<')
    {
      dir = find (&name, ">", NULL);
      if (dir == NULL)
	;
    }
  nam = find (&name, ".;", name);
  typ = find (&name, ";", name);
  ver = name;
#endif

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
read_asciz (char *string, word_t *data)
{
  word_t word;
  int i;
  
  for (;;)
    {
      word = *data++;
      for (i = 0; i < 5; i++)
	{
	  *string = (word >> 29) & 0177;
	  if (*string == 0)
	    return;
	  word <<= 7;
	  string++;
	}
    }
}

static word_t
sum (word_t checksum, word_t data)
{
  data &= 0777777777777LL;

  if (format <= 4)
    {
      checksum += data;
      if (checksum & 01000000000000LL)
	checksum++;
    }
  else
    {
      checksum = (checksum << 1) | (checksum >> 35);
      checksum += data;
    }

  return checksum & 0777777777777LL;
}

static word_t
read_record (FILE *f, word_t word)
{
  word_t checksum = 0;
  int i;

  block[0] = word & 0777777777777LL;
  checksum = sum (checksum, block[0]);
  for (i = 1; i < MAX; i++)
    {
      word = get_word (f);
      if (word == -1)
	return word;
      if (word & (START_RECORD | START_FILE | START_TAPE))
	return word;
      block[i] = word;
      checksum = sum (checksum, word);
    }

#if 0
  fprintf (stderr, "Checksum: %012llo\n", checksum);
  fprintf (stderr, "000: %012llo\n", block[0]);
  fprintf (stderr, "001: %012llo\n", block[1]);
  fprintf (stderr, "002: %012llo saveset,,tape\n", block[2]);
  fprintf (stderr, "003: %012llo file,,page\n", block[3]);
  fprintf (stderr, "004: %012llo type\n", block[4]);
  fprintf (stderr, "005: %012llo record\n", block[5]);
#endif

  return get_word (f);
}

static word_t
read_tape_header (FILE *f, word_t word)
{
  char name[100];

  word = read_record (f, word);

  //fprintf (stderr, "006: %012llo format\n", block[6]);

  read_asciz (name, &block[9]);
  fprintf (stderr, "DUMPER tape #%d, %s, ", right (block[2]), name);
  print_timestamp (stderr, block[8]);
  fputc ('\n', stderr);

  return word;
}

static void
read_file (int offset)
{
  char name[100];
  char *p;

  if (offset != 0206)
    return;

  read_asciz (name, &block[6]);
  p = strchr (name, ';');
  if (p)
    *p = 0;
  fprintf (stderr, " %-40s", name);
  print_timestamp (stderr, block[offset + 5]);
  fprintf (stderr, " %4d", right (block[offset + 011]));
  fprintf (stderr, " %lld(%lld)\n",
	   block[offset + 012],
	   (block[offset + 011] >> 24) & 077);

#if 0
  fprintf (stderr, "006: %012llo file name\n", block[6]);
  fprintf (stderr, "Timestamp, last write: ");
  print_timestamp (stderr, block[offset + 5]);
  fputc ('\n', stderr);
  fprintf (stderr, "Timestamp, creation: ");
  print_timestamp (stderr, block[offset + 013]);
  fputc ('\n', stderr);
  fprintf (stderr, "Timestamp, user write: ");
  print_timestamp (stderr, block[offset + 014]);
  fputc ('\n', stderr);
  fprintf (stderr, "Timestamp, last nonwrite: ");
  print_timestamp (stderr, block[offset + 015]);
  fputc ('\n', stderr);

  for (i = offset; i < offset + 030; i++)
    fprintf (stderr, "%03o: %012llo\n", i, block[i]);
#endif
}

static void
read_data (void)
{
}

static void
read_tape (FILE *f)
{
  word_t word;

  word = get_word (f);

  word = read_tape_header (f, word);
  for (;;)
    {
      word = read_record (f, word);
      switch ((01000000000000LL - block[4]) & 0777777777777LL)
	{
	case DATA:
	  read_data ();
	  break;
	case TPHD:
	  break;
	case FLHD:
	  read_file (0206);
	  break;
	case FLTR:
	  read_file (0006);
	  break;
	case TPTR:
	  break;
	case USR:
	  break;
	case CTPH:
	  break;
	case FILL:
	  break;
	default:
	  fprintf (stderr, "Uknown record type.\n");
	  exit (1);
	  break;
	}
    }
}

static void
write_asciz (const char *string, word_t *data)
{
  const char *z = "\0\0\0\0\0";
  word_t word;
  int i;

  for (;;)
    {
      word = 0;
      for (i = 0; i < 5; i++)
	{
	  if (string[i] == 0)
	    string = z;
	  word <<= 7;
	  word |= string[i] << 1;
	}
      *data++ = word;
      if (string == z)
	return;
      string += 5;
    }
}

static void
write_record (FILE *f, int type, word_t flags)
{
  word_t checksum = 0;
  int i;

  fprintf (stderr, "\nWrite record %s", type_name[type]);

  block[1] = 0;
  block[2] = (saveset_number << 18) | tape_number;
  block[3] = page_number;
  fprintf (stderr, ", page %d", page_number);
  if (0)
    block[3] |= file_number << 18;
  block[4] = (-type) & 0777777777777LL;
  block[5] = record_number++;

  for (i = 0; i < MAX; i++)
    checksum = sum (checksum, block[i]);
  block[0] = (checksum ^ 0777777777777LL) | START_RECORD | flags;

  for (i = 0; i < MAX; i++)
    write_word (f, block[i]);

  memset (block, 0, sizeof block);
}

static int
get_page (FILE *f)
{
  word_t word;
  int ok = 0;
  int i;

  memset (block + 6, 0, 512 * sizeof (word_t));

  /* If the first word indicates EOF, return "no page".
     In other cases, return a partial or full page. */
  for (i = 0; i < 512; i++)
    {
      word = get_word (f);
      if (word == -1)
	return ok;
      block[6 + i] = word;
      ok = 1;
    }

  return 1;
}

static void
write_file (FILE *f, char *name)
{
  char file_name[100], device[100];
  word_t fdb[030];
  struct stat st;
  int size, byte_size;
  int protection, generation;
  const char *author;
  const char *error;
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

  size = file_size (name);
  strcpy (device, "PS");
  byte_size = 36;
  protection = 0777777;
  generation = 1;
  author = "OPERATOR";

  error = unmangle (file_name, device, name, protection, author);
  if (error)
    fprintf (debug, "\nERROR: Bad file name \"%s\": %s", file_name, error);
  else
    fprintf (debug, "\nFILE: %s", file_name);

  memset (fdb, 0, sizeof fdb);
  //000 //header word
  fdb[004] = protection;
  fdb[004] |= 0500000LL << 18;
  fdb[005] = tops20_timestamp (st.st_mtime); //timestamp: last write
  //006 //pointer to string: author
  fdb[007] = (word_t)generation << 18;
  fdb[011] = (word_t)byte_size << 24;
  fdb[011] |= (size + 511) / 512;
  fdb[012] = size * (36 / byte_size);
  fdb[013] = tops20_timestamp (st.st_ctime); //timestamp: creation
  fdb[014] = tops20_timestamp (st.st_mtime); //timestamp: last user write
  fdb[015] = tops20_timestamp (st.st_atime); //timestamp: last nonwrite access

  write_asciz (file_name, block + 6);
  memcpy (block + 0206, fdb, sizeof fdb);
  page_number = 0;
  write_record (f, FLHD, 0);

  while (get_page (input))
    {
      write_record (f, DATA, 0);
      page_number++;
    }
  fclose (input);

  memcpy (block + 6, fdb, sizeof fdb);
  page_number = 0;
  write_record (f, FLTR, 0);
}

static void
write_usr (FILE *f)
{
  write_record (f, USR, 0);
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

  saveset_number = 0;
  tape_number = 1;
  file_number = 1;
  record_number = 1;

  block[6] = format;
  block[7] = bfmsg;
  block[8] = tops20_timestamp (time (NULL));
  write_asciz ("Saveset name", block + 6 + bfmsg);
  write_record (f, TPHD, START_FILE);

  for (i = 0; i < file_argc; i++)
    {
      write_file (f, file_argv[i]);
      file_number++;
    }

  write_record (f, TPTR, 0);
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

  while ((opt = getopt (argc, argv, "ctvx012379f:W:C:")) != -1)
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
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	  format = opt - '0';
	  break;
	case '7':
	  input_word_format = &tape7_word_format;
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
