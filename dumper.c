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
#include <ctype.h>
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
#include "mkdirs.h"

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
static word_t *data = &block[6];
static int extract = 0;
static word_t tape_flags = 0;

static FILE *list;
static FILE *info;
static FILE *debug;

static FILE *output;
static char file_path[100];
static struct timeval tv[2];
static int file_argc;
static char** file_argv;
static int saveset_number;
static int tape_number;
static int file_number;
static int page_number;
static int record_number;
static int format;
static int word_bytes;
static word_t file_bytes;
static word_t file_octets;

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
  time_t t;
  struct tm *tm;

  if (timestamp == 0)
    {
      fprintf (f, "****-**-** **:**");
      return;
    }

  unix_timestamp (tv, timestamp);
  tv[1] = tv[0];
  t = tv[0].tv_sec;
  tm = gmtime (&t);
  strftime (string, sizeof string, "%Y-%m-%d %H:%M:%S", tm);
  if (1)
    fputs (string, f);
  else
    fprintf (f, "%s.%02ld", string, tv[0].tv_usec / 10000);
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
  flush_word (output);
  fclose (output);
  output = NULL;
  utimes (file_path, tv);
}

/* Convert TENEX file name to an acceptable Unix name. */
static void
mangle (void)
{
  int dir = 0, ver = 1;
  char *p;
  char *q = file_path;

  p = strchr (file_path, ':');
  if (p == NULL || p == file_path || p[-1] == 026)
    p = file_path;
  else
    p++;

  if (*p == '<')
    {
      dir = 1;
      p++;
    }

  for (; *p != 0; p++)
    {
      switch (*p)
	{
	case '>':
	  if (dir)
	    {
	      *q++ = '/';
	      dir = 0;
	    }
	  else
	    *q++ = '>';
	  break;
	case '.':
	  *q++ = (dir && format > 0) ? '/' : '.';
	  break;
	case ';':
	  if (format == 0 && ver)
	    *q++ = dir ? *p : '.';
	  else
	    *q++ = *p;
          ver = 0;
	  break;
	case '/':
	  *q++ = '\\';
	  break;
	case 026:
	  p++;
	  /* Fall through. */
	default:
	  *q++ = tolower (*p);
	  break;
	}
    }

  *q = 0;
}

static char *
upcase (char *string)
{
  char *p = string;
  while (*p)
    {
      *p = toupper (*p);
      p++;
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
	  int protection, const char *account, int *generation)
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

  if (d - directories > 2 && format == 0)
    return "TENEX doesn't support subdirectories";

  file = find (&name, ".", name);
  type = find (&name, ".;", name);
  version = find (&name, ";", name);
  if (version)
    {
      char *end;
      long x = strtol (version, &end, 10);
      if (end > version)
	*generation = x;
    }

  if (device)
    file_name += sprintf (file_name, "%s:", device);
  if (directories[0] != NULL)
    {
      file_name += sprintf (file_name, "<");
      for (d = &directories[0]; *d != NULL; d++, sep = ".")
	file_name += sprintf (file_name, "%s%s", sep, upcase (*d));
      file_name += sprintf (file_name, ">");
    }
  if (file)
    file_name += sprintf (file_name, "%s", upcase (file));
  file_name += sprintf (file_name, ".");
  if (type)
    file_name += sprintf (file_name, "%s", upcase (type));
  file_name += sprintf (file_name, "%c%u",
			format == 0 ? ';' : '.', *generation);
  file_name += sprintf (file_name, ";P%06o", protection);
  if (account)
    file_name += sprintf (file_name, ";A%s", account);

  return NULL;
}

static void
open_file (void)
{
  mangle ();
  mkdirs (file_path);

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

  //fprintf (stderr, "006: %012llo format\n", data[0]);

  read_asciz (name, &data[3]);
  fprintf (stderr, "DUMPER tape #%d, %s", right (block[2]), name);
  if (format > 0) {
    fputs (", ", stderr);
    print_timestamp (stderr, data[2]);
  }
  fputc ('\n', stderr);

  return word;
}

static void
print_info (FILE *f, word_t *fdb)
{
  int bits_per_byte, file_words;

  print_timestamp (f, fdb[5]);
  fprintf (f, " %4d", right (fdb[011]));
  file_bytes = fdb[012];
  bits_per_byte = (fdb[011] >> 24) & 077;
  word_bytes = bits_per_byte ? 36 / bits_per_byte : 0;
  file_words = file_bytes / word_bytes;
  file_octets = 5 * file_words;
  file_octets += ((file_bytes % word_bytes) * bits_per_byte + 7) / 8;
  fprintf (f, " %lld(%d)\n",
	   file_bytes, bits_per_byte);
}

static void
read_file (int offset)
{
  char *p;

  if (offset != 0206)
    {
      if (format == 0)
	print_info (stderr, block + offset);

      if (output != NULL)
	close_file ();
    }
  else
    {
      read_asciz (file_path, &data[0]);

      p = strchr (file_path, ';');
      if (format == 0 && p != NULL)
	p = strchr (p + 1, ';');
      if (p)
	*p = 0;

      fprintf (stderr, " %-40s", file_path);

      file_bytes = 0777777777777LL;
      if (format > 0)
	print_info (stderr, block + offset);

      if (extract)
	open_file ();
    }
}

static void
read_data (void)
{
  int i;
  if (output == NULL)
    return;
  for (i = 0; i < 512 && file_bytes >= 0; i++, file_bytes -= word_bytes)
    write_word (output, data[i]);
}

static void
read_tape (FILE *f)
{
  word_t word;

  word = get_word (f);

  word = read_tape_header (f, word);
  while (word != -1)
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
	  //exit (1);
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
write_mark (void)
{
  tape_flags = START_FILE;
}

static void
write_record (FILE *f, int type)
{
  word_t checksum = 0;
  int i;

  if (tape_flags & START_FILE)
    fprintf (debug, "\nWrite mark");

  fprintf (debug, "\nWrite record %s", type_name[type]);

  memset (block, 0, 6 * sizeof (word_t));

  block[2] = (saveset_number << 18) | tape_number;
  if (format > 0)
    block[2] |= saveset_number;
  block[3] = page_number;
  fprintf (debug, ", page %d, record %d", page_number, record_number);
  if (0)
    block[3] |= file_number << 18;
  block[4] = (-type) & 0777777777777LL;
  block[5] = record_number++;

  for (i = 0; i < MAX; i++)
    checksum = sum (checksum, block[i]);
  block[0] = (checksum ^ 0777777777777LL) | START_RECORD | tape_flags;
  tape_flags = 0;

  write_word (f, block[0]);
  for (i = 1; i < MAX; i++)
    write_word (f, block[i] & 0777777777777LL);

  memset (block, 0, sizeof block);
}

static int
get_page (FILE *f)
{
  word_t word;
  int ok = 0;
  int i;

  memset (data, 0, 512 * sizeof (word_t));

  /* If the first word indicates EOF, return "no page".
     In other cases, return a partial or full page. */
  for (i = 0; i < 512; i++)
    {
      word = get_word (f);
      if (word == -1)
	return ok;
      data[i] = word;
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
  const char *account;
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
  if (0)
    strcpy (device, "PS");
  byte_size = 36;
  protection = 0777777;
  generation = 1;
  account = "1";

  error = unmangle (file_name, NULL, name, protection, account, &generation);
  if (error)
    fprintf (stderr, "\nERROR: Bad file name \"%s\": %s", file_name, error);
  else
    fprintf (list, "\n  %s", file_name);

  memset (fdb, 0, sizeof fdb);
  //000 //header word
  fdb[004] = protection;
  fdb[004] |= 0500000LL << 18;
  fdb[005] = tops20_timestamp (st.st_mtime); //timestamp: last write
  //006 //pointer to string: account
  fdb[007] = (word_t)generation << 18;
  fdb[011] = (word_t)byte_size << 24;
  fdb[011] |= (size + 511) / 512;
  fdb[012] = size * (36 / byte_size);
  fdb[013] = tops20_timestamp (st.st_ctime); //timestamp: creation
  fdb[014] = tops20_timestamp (st.st_mtime); //timestamp: last user write
  fdb[015] = tops20_timestamp (st.st_atime); //timestamp: last nonwrite access
  write_asciz (file_name, data);
  memcpy (data + 0200, fdb, sizeof fdb);
  page_number = 0;
  write_record (f, FLHD);

  while (get_page (input))
    {
      write_record (f, DATA);
      page_number++;
    }
  fclose (input);

  memcpy (data, fdb, sizeof fdb);
  page_number = 0;
  write_record (f, FLTR);
  if (format == 0)
    {
      write_mark ();
      record_number++;
    }
}

static void
write_usr (FILE *f)
{
  write_record (f, USR);
}

static void
write_tape (FILE *f)
{
  struct word_format *tmp = input_word_format;
  input_word_format = output_word_format;
  output_word_format = tmp;
  int i, bfmsg = 0;

  if (f == NULL)
    f = stdout;

  saveset_number = 0;
  tape_number = 1;
  file_number = 1;

  if (format == 0)
    {
      record_number = 2;
    }
  else
    {
      record_number = 1;
      data[bfmsg++] = format;
      bfmsg++;
      data[bfmsg++] = tops20_timestamp (time (NULL));
      data[1] = bfmsg;
    }
  write_asciz ("Saveset name", data + bfmsg);

  write_record (f, TPHD);
  if (format == 0)
    {
      write_mark ();
      record_number++;
    }

  for (i = 0; i < file_argc; i++)
    {
      write_file (f, file_argv[i]);
      file_number++;
    }

  write_record (f, TPTR);
  write_mark ();
  flush_word (f);
}

static void
usage (const char *x)
{
  fprintf (stderr,
	   "Usage: %s -c|-t|-x [-v0123456] [-Wformat] [-Cdir] [-f file]\n", x);
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

  /* If the program is called mini-something, default to mini-dumper
     format.  Otherwise go with format 4 which is acceptable to a wide
     range of DUMPER versions. */
  if (strstr (argv[0], "mini") == 0)
    format = 4;
  else
    format = 0;

  while ((opt = getopt (argc, argv, "ctvx012345679f:W:C:")) != -1)
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
