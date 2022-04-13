/* Copyright (C) 2013, 2021-2022 Lars Brinkhoff <lars@nocrew.org>

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

#ifndef DIS_H
#define DIS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define WORDMASK	(0777777777777LL)
#define SIGNBIT		(0400000000000LL)

/* ITS page size, in words */
#define ITS_PAGESIZE 1024

/* DEC page size, in words */
#define DEC_PAGESIZE 512

typedef long long word_t;

#define JRST   ((word_t)(0254000000000LL))
#define JRST_1 (JRST + 1)
#define JUMPA  ((word_t)(0324000000000LL))

struct pdp10_file;
struct pdp10_memory;
extern word_t start_instruction;
extern FILE *output_file;

struct file_format {
  const char *name;
  void (*read) (FILE *f, struct pdp10_memory *memory, int cpu);
  void (*write) (FILE *f, struct pdp10_memory *memory);
};

struct word_format {
  const char *name;
  word_t (*get_word) (FILE *);
  void (*rewind_word) (FILE *);		/* NULL means just rewind (f) */
  void (*seek_word) (FILE *, int);	/* NULL means rewind and go forward. */
  void (*write_word) (FILE *, word_t);
  void (*flush_word) (FILE *);		/* NULL means do nothing */
};

enum { SYMBOLS_NONE, SYMBOLS_DDT, SYMBOLS_ALL };

enum {
  START_FILE = 1LL << 36,
  START_RECORD = 1LL << 37,
  START_TAPE = 1LL << 38
};

extern struct file_format *input_file_format;
extern struct file_format *output_file_format;
extern struct file_format atari_file_format;
extern struct file_format cross_file_format;
extern struct file_format csave_file_format;
extern struct file_format dmp_file_format;
extern struct file_format exb_file_format;
extern struct file_format exe_file_format;
extern struct file_format fasl_file_format;
extern struct file_format hex_file_format;
extern struct file_format hiseg_file_format;
extern struct file_format iml_file_format;
extern struct file_format lda_file_format;
extern struct file_format mdl_file_format;
extern struct file_format nsave_file_format;
extern struct file_format osave_file_format;
extern struct file_format palx_file_format;
extern struct file_format pdump_file_format;
extern struct file_format raw_file_format;
extern struct file_format rim10_file_format;
extern struct file_format sblk_file_format;
extern struct file_format shr_file_format;
extern struct file_format tenex_file_format;

extern struct word_format *input_word_format;
extern struct word_format *output_word_format;
extern struct word_format aa_word_format;
extern struct word_format alto_word_format;
extern struct word_format bin_word_format;
extern struct word_format cadr_word_format;
extern struct word_format core_word_format;
extern struct word_format data8_word_format;
extern struct word_format dta_word_format;
extern struct word_format its_word_format;
extern struct word_format oct_word_format;
extern struct word_format pt_word_format;
extern struct word_format sail_word_format;
extern struct word_format tape_word_format;
extern struct word_format tape7_word_format;

extern void     usage_file_format (void);
extern int      parse_input_file_format (const char *);
extern int      parse_output_file_format (const char *);
extern void     guess_input_file_format (FILE *);
extern void     usage_word_format (void);
extern int      parse_input_word_format (const char *);
extern int      parse_output_word_format (const char *);
extern word_t	get_word (FILE *f);
extern word_t	get_checksummed_word (FILE *f);
extern void	reset_checksum (word_t);
extern void	check_checksum (word_t);
extern void	rewind_word (FILE *f);
extern void	seek_word (FILE *f, int position);
extern void	by_five_octets (FILE *f, int position);
extern void	by_eight_octets (FILE *f, int position);
extern void	write_word (FILE *, word_t);
extern void	flush_word (FILE *);
extern int      get_7track_record (FILE *f, word_t **buffer);
extern int      get_9track_record (FILE *f, word_t **buffer);
extern void     write_7track_record (FILE *f, word_t *buffer, int);
extern void     write_9track_record (FILE *f, word_t *buffer, int);
extern word_t	get_core_word (FILE *f);
extern void	write_core_word (FILE *f, word_t word);
extern void	read_raw_at (FILE *f, struct pdp10_memory *memory,
			     int address);
extern void	write_raw_at (FILE *f, struct pdp10_memory *memory,
			      int address);
extern void	write_sblk_core (FILE *f, struct pdp10_memory *, int begin);
extern void	write_sblk_symbols (FILE *f);
extern void	write_dec_symbols (struct pdp10_memory *memory);
extern void	sblk_info (FILE *f, word_t word0, int cpu_model);
extern void     dmp_info (struct pdp10_memory *memory, int cpu_model);
extern void     dec_info (struct pdp10_memory *memory,
			  word_t entry_vec_len, word_t entry_vec_addr,
			  int cpu_model);
extern void	ntsddt_info (struct pdp10_memory *memory, int);
extern void     usage_symbols_mode (void);
extern int      parse_symbols_mode (const char *string);
extern void     usage_machine (void);
extern int      parse_machine (const char *string, int *machine);
extern void	dis (struct pdp10_memory *memory, int cpu_model);
extern void	disassemble_word (struct pdp10_memory *memory, word_t word,
				  int address, int cpu_model);
extern word_t   ascii_to_sixbit (const char *ascii);
extern void	sixbit_to_ascii (word_t sixbit, char *ascii);
extern word_t	ascii_to_squoze (const char *ascii);
extern void	squoze_to_ascii (word_t squoze, char *ascii);
extern void	print_date (FILE *, word_t t);
extern void	print_time (FILE *, word_t t);
extern void	print_datime (FILE *, word_t t);
extern void	print_dec_timestamp (FILE *f, word_t timestamp);
extern void	timestamp_from_dec (struct tm *tm, word_t timestamp);
extern int	byte_size (int, int *);
extern void	scramble (int decrypt, int verbose, word_t password,
		          const word_t *input, word_t *output, int count);

extern void weenixname (char *);
extern void weenixpath (char *, word_t, word_t, word_t);
extern void winningname (word_t *, word_t *, const char *);

#endif /* DIS_H */
