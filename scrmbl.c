/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2020 Adam Sampson <ats@offog.org>

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

/* This is based on the algorithm implemented in EJS; SCRMBL 73. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dis.h"

#define BITMASK(width) ((1LL << (width)) - 1)
#define FIELD(pos, width, value) (((value) >> (pos)) & BITMASK (width))

static int verbose = 0;

static word_t random_seed;

static word_t
get_random (void)
{
  word_t exp, frac, half;

  /* This is the same RNG used by SPCWAR! It returns an 18-bit result,
     the same in both halves. */

  /* FMPB B,RAN - floating-point square unrounded.  Simpler than a
     real FMP implementation because we know the output must be
     positive. */

  /* Unpack the input and make it positive. */
  exp = FIELD (27, 8, random_seed) - 0200;
  frac = FIELD (0, 27, random_seed);
  if ((random_seed >> 35) & 1) {
    /* Negative input. */
    exp ^= BITMASK (8);
    frac |= ~BITMASK (27);
  }

  /* Square it. */
  exp = (exp * 2) + 0200;
  frac *= frac;

  /* Normalise so the top bit of the fraction is 1. */
  while (frac != 0 && !FIELD (27 * 2 - 1, 1, frac))
    {
      frac <<= 1;
      exp--;
    }
  frac >>= 27;

  /* If the fraction is 0, FMPB also makes the exponent 0. */
  if (frac == 0)
    exp = 0;

  /* Pack it back together. */
  random_seed = ((exp & BITMASK (8)) << 27) | (frac & BITMASK (27));

  /* TSC B,B - XOR the two halves together to give the result. */
  half = (random_seed >> 18) ^ (random_seed & BITMASK (18));
  return (half << 18) | half;
}

typedef enum
  {
   SCRAMBLE_NONE = 0777777,
   SCRAMBLE_COMPLEMENT = 0,
   SCRAMBLE_SWAP,
   SCRAMBLE_XOR,
   SCRAMBLE_ROTATE,
   NUM_STEPS,
  } scramble_t;

static int
scramble_step_compare (const void *a, const void *b)
{
  /* When encrypting, the largest order goes first. */
  word_t a_order = *((word_t *) a);
  word_t b_order = *((word_t *) b);
  if (a_order < b_order)
    return 1;
  else if (a_order == b_order)
    return 0;
  else
    return -1;
}

static void
scramble (int decrypt, word_t password, const word_t *input, word_t *output, int count)
{
  int i, j;
  int shift_size = 0;
  word_t steps[NUM_STEPS];
  word_t word;

  /* Decide what order to do the possible scrambling operations in. */
  for (i = 0; i < NUM_STEPS; i++)
    {
      steps[i] = (FIELD (27 - (9 * i), 8, password) << 18) | i;
    }
  qsort (steps, NUM_STEPS, sizeof (*steps), scramble_step_compare);
  if (decrypt)
    {
      /* Reverse the order. */
      for (i = 0; i < NUM_STEPS / 2; i++)
        {
          word = steps[i];
          steps[i] = steps[NUM_STEPS - i - 1];
          steps[NUM_STEPS - i - 1] = word;
        }
    }

  /* Enable/disable optional operations. */
  for (i = 0; i < NUM_STEPS; i++)
    switch (steps[i] & BITMASK (18))
      {
      case SCRAMBLE_COMPLEMENT:
        if ((FIELD (033, 010, password) / 3) & 1)
          steps[i] = (steps[i] & ~BITMASK (18)) | SCRAMBLE_NONE;
        break;
      case SCRAMBLE_SWAP:
        if (((FIELD (022, 010, password) & ~FIELD (011, 010, password)) / 3) & 1)
          steps[i] = (steps[i] & ~BITMASK (18)) | SCRAMBLE_NONE;
        break;
      case SCRAMBLE_XOR:
        if (((FIELD (011, 010, password) + FIELD (033, 010, password)) >> 3) & 1)
          steps[i] = (steps[i] & ~BITMASK (18)) | SCRAMBLE_NONE;
        break;
      default:
        /* SCRAMBLE_ROTATE always happens. */
        break;
      }

  /* Compute initial random seed. */
  if (FIELD (032, 1, password))
    random_seed = (password >> 18) & BITMASK (18);
  else
    random_seed = password & BITMASK (18);
  if (FIELD (010, 1, password))
    random_seed |= 1LL << 18;
  else
    random_seed <<= 1;
  if (FIELD (021, 1, password))
    random_seed = (-random_seed) & WORDMASK;

  /* Compute SCRAMBLE_ROTATE shift size. */
  shift_size = (password * password) & 077;
  if (FIELD (0, 1, password))
    shift_size *= -1;
  while (shift_size < 0)
    shift_size += 36;
  shift_size %= 36;
  if (decrypt)
    shift_size = 36 - shift_size;

  if (verbose)
    {
      /* Show our equivalents of SCRMBL's variables, for comparison. */
      fprintf (stderr, "SCR/ %012llo\n", password);
      fprintf (stderr, "RAN/ %012llo\n", random_seed);
      for (i = 0; i < NUM_STEPS; i++)
        {
          fprintf (stderr, "X%d/  %012llo\n", i + 1, steps[i]);
        }
    }

  for (i = 0; i < count; i++)
    {
      word = input[i];

      if (!decrypt)
        word ^= get_random ();

      for (j = 0; j < NUM_STEPS; j++)
        switch (steps[j] & BITMASK (18))
          {
          case SCRAMBLE_COMPLEMENT:
            word = (~word) & WORDMASK;
            break;
          case SCRAMBLE_SWAP:
            word = ((word >> 18) & BITMASK(18)) | ((word & BITMASK (18)) << 18);
            break;
          case SCRAMBLE_XOR:
            word ^= password;
            break;
          case SCRAMBLE_ROTATE:
            word = (FIELD (0, 36 - shift_size, word) << shift_size)
              | FIELD (36 - shift_size, shift_size, word);
            break;
          default:
            break;
          }

      if (decrypt)
        word ^= get_random ();

      output[i] = word;
    }
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-d] [-v] [-W<word format>] <password> <input file> <output file>\n\n", argv[0]);
  usage_word_format ();
  fprintf (stderr, "Output is always in its word format.\n"); /* FIXME */
  exit (1);
}

int
main (int argc, char **argv)
{
  int i;
  word_t *input = NULL;
  int input_count = 0;
  int input_size = 0;
  int decrypt = 0;
  FILE *file;
  int opt;
  word_t *output = NULL;
  word_t password;
  word_t word;

  while ((opt = getopt (argc, argv, "dvW:")) != -1)
    {
      switch (opt)
        {
        case 'd':
          decrypt = 1;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'W':
          if (parse_word_format (optarg))
            usage (argv);
          break;
        default:
          usage (argv);
        }
    }

  if (optind != argc - 3)
    usage (argv);

  password = ascii_to_sixbit (argv[optind]);

  /* Read the whole input into memory. */
  file = fopen (argv[optind + 1], "rb");
  if (file == NULL)
    {
      fprintf (stderr, "%s: Error opening %s: %s\n",
               argv[0], argv[optind + 1], strerror (errno));
      return 1;
    }
  while ((word = get_word (file)) != -1)
    {
      if (input_count >= input_size)
        {
          input_size += 1024;
          input = realloc (input, input_size * sizeof (*input));
          if (input == NULL)
            {
              fprintf (stderr, "out of memory\n");
              return 1;
            }
        }

      input[input_count++] = word;
    }
  fclose (file);

  output = calloc (input_count, sizeof (*output));
  if (output == NULL)
    {
      fprintf (stderr, "out of memory\n");
      return 1;
    }

  scramble (decrypt, password, input, output, input_count);

  file = fopen (argv[optind + 2], "wb");
  for (i = 0; i < input_count; i++)
    {
      write_its_word (file, output[i]);
    }
  fclose (file);

  free (input);
  free (output);

  return 0;
}
