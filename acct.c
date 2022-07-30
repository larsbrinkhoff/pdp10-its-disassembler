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

/*
1975 format:
tab4:	xwd	j,jppn		;first entry is ppn of loser
	xwd	0,k		;job number. negative means forced out
				;by a reload
	xwd	j,jlogin	;login date/time
	xwd	0,ntime		;current time
	xwd	j,jlin		;line number
	xwd	j,jtime		;ttime of job
	xwd	j,jkcj		;kcj of job

1978 format:
tab4:	ntime			;time
	tmplog			;login date/time
	xwd	j,jppn		;ppn of loser
	tmpcpu			;incremental ttime of job
	tmpkcj			;incremental kcj of job
	xwd	j,jldck		;load measure
	tmpdsk			;incremental disk
	xwd	j,jname		;job name

	400000,,20		;mark the buffer
	440000,,2		;LOAD RELOAD CODE INTO B
	500000,,11		;logout header
	510000,,11		;a crash record?
	540000,,11		;a check point?
	600000,,4		;program check flag
	640000,,3		;header word for load
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dis.h"

static const char *started = "initiated";

static void
print_timestamp (FILE *f, word_t date, word_t minutes)
{
  print_dec_timestamp (f, date);
  fprintf (f, " %02lld:%02lld", minutes / 60, minutes % 60);
}

static void
print_octal (word_t word)
{
  if (word == -1)
    {
      printf ("               ");
      return;
    }
  
  printf (" %06llo,,%06llo", word >> 18, word & 0777777LL);
}

static void
print_sixbit (word_t word)
{
  char string[7];

  if (word == -1)
    {
      printf ("       ");
      return;
    }
  
  sixbit_to_ascii (word, string);
  printf (" %s", string);
}

static void
print_ppn (word_t word)
{
  char prj[7];
  char prg[4];

  sixbit_to_ascii (word, prj);
  strncpy (prg, prj+3, 3);
  prj[3] = 0;
  prg[3] = 0;
  printf ("%s,%s", prj, prg);
}

static void
print_ascii (word_t word)
{
  int i;

  if (word == -1)
    return;

  for (i = 0; i < 5; i++)
    {
      char c = (word >> 29) & 0177;
      if (c < 32 || c == 0177)
	c = '.';
      putchar (c);
      word <<= 7;
    }
}

static void
print_header (FILE *f, word_t first)
{
  word_t date, minutes;
  word_t word;
  int i = 1;

  printf ("Accounting %s:  ", started);
  started = "restarted";

  word = get_word (f);
  i++;
  date = word >> 18;
  minutes = word & 07777;
  print_timestamp (stdout, date, minutes);
  fputs ("  ", stdout);

  do
    {
      word = get_word (f);
      i++;
      print_ascii (word);
    }
  while (word & 0376);
  putchar ('\n');

  printf ("  PRJ,PRG  Job  Login             Logout            Line  Runtime   Kcticks\n");

  if ((first >> 18) == 0400000)
    {
      for (; i < (first & 0777777); i++)
	word = get_word (f);
    }
}
//sys.act/12.dat/.149
//1975-05-12 00:00  Stanford 7.01/E 04-30-75.

//sys.act/30.dat/.326
//1978-05-30 00:00  SU-AI WAITS 8.70/Z  Assembled 05-27-78..
//sys.act/1.dat/.327
//1978-06-01 00:03  SU-AI WAITS 8.71    Assembled 05-31-78..

//jun  1  1978
//may.txt 325
//[B,AVH]		      1    2:32    0:47    1218

static void
print_trailer (FILE *f)
{
  word_t date, minutes;
  word_t word;
  printf ("Accounting stopped: ");
  word = get_word (f);
  date = word >> 18;
  minutes = word & 07777;
  print_timestamp (stdout, date, minutes);
  putchar ('\n');
}

static void
print_old_record (FILE *f, word_t word)
{
  word_t date, minutes;
  word_t word2;

  fputs ("  ", stdout);
  print_ppn (word);

  word2 = get_word (f);
  if (word2 & 0400000000000)
    printf ("  %3llo  ", 01000000000000LL - (word2 & WORDMASK));
  else
    printf ("  %3llo  ", word2);

  word = get_word (f);
  date = word >> 18;
  minutes = word & 07777;
  print_timestamp (stdout, date, minutes);
  fputs ("  ", stdout);

  word = get_word (f);
  date = word >> 18;
  minutes = word & 07777;
  print_timestamp (stdout, date, minutes);
  fputs ("  ", stdout);

  word = get_word (f);
  if (word == 0777777777777LL)
    fputs ("DET  ", stdout);
  else
    printf ("%3llo  ", word & 0777777);

  word = get_word (f);
  printf ("%8lld  ", word);

  word = get_word (f);
  printf ("%8lld ", word);

  if ((word2 & 0400000000000) == 0)
    {
      putchar ('\n');
      return;
    }

  word = get_word (f);
  word = get_word (f);
  print_sixbit (word);

  putchar ('\n');
}

static void
print_new_record (FILE *f, word_t first)
{
  word_t date, minutes;
  word_t word, time, login;

  time = get_word (f);
  login = get_word (f);

  fputs ("  ", stdout);
  word = get_word (f);
  print_ppn (word);

  printf ("  %3llo  ", (first >> 18) & 0777);

  date = login >> 18;
  minutes = login & 07777;
  print_timestamp (stdout, date, minutes);
  fputs ("  ", stdout);

  date = time >> 18;
  minutes = time & 07777;
  print_timestamp (stdout, date, minutes);

  printf ("  %3llo  ", (first >> 9) & 0777);

  word = get_word (f);
  printf ("%8lld  ", word);

  word = get_word (f);
  printf ("%8lld  ", word);

  word = get_word (f);
  //printf ("%012llo ", word);
  word = get_word (f);
  //printf ("%012llo ", word);

  word = get_word (f);
  print_sixbit (word);

  putchar ('\n');
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s [-Wformat] < file\n", x);
  usage_word_format ();
  exit (1);
}

static void
parse_old_format (FILE *f, word_t word)
{
  for (;;)
    {
      if (word != -1)
	word &= WORDMASK;
      switch (word)
	{
	case -1:
	  exit (0);
	case 0777777777777LL:
	  print_header (f, word);
	  break;
	case 0777777777776LL:
	  print_trailer (f);
	  break;
	default:
	  print_old_record (f, word);
	  break;
	}
      word = get_word (f);
    }
}

static void
parse_new_format (FILE *f, word_t word)
{
  int length;

  for (;;)
    {
      if (word == -1)
	exit (0);

      length = word & 0777;
      switch ((word >> 27) & 0777)
	{
	case 0400:
	  print_header (f, word);
	  break;
	  //case 0500: //logout
	case 0440: //reload
	case 0450: //reload
	case 0510: //crash
	case 0600: //program check
	case 0610:
	case 0640: //load
	  while (--length)
	    word = get_word (f);
	  break;
	case 0500:
	case 0540:
	  print_new_record (f, word);
	  break;
	default:
	  printf ("Unknown: %03llo\n", (word >> 27) & 0777);
	  break;
	}
      word = get_word (f);
    }
}

int
main (int argc, char **argv)
{
  word_t word;
  int opt;

  while ((opt = getopt (argc, argv, "W:")) != -1)
    {
      switch (opt)
	{
	case 'W':
	  if (parse_input_word_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	}
    }

  word = get_word (stdin);
  word &= WORDMASK;
  if (word == 0777777777777LL)
    parse_old_format (stdin, word);
  else if (word == 0400000000020LL)
    parse_new_format (stdin, word);
  else
    ;

  return 0;
}
