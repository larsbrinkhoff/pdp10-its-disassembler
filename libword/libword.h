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

#ifndef LIBWORD_H
#define LIBWORD_H

typedef long long word_t;

struct word_format {
  const char *name;
  word_t (*get_word) (FILE *);
  void (*rewind_word) (FILE *);		/* NULL means just rewind (f) */
  void (*seek_word) (FILE *, int);	/* NULL means rewind and go forward. */
  void (*write_word) (FILE *, word_t);
  void (*flush_word) (FILE *);		/* NULL means do nothing */
};

enum {
  START_FILE = 1LL << 36,
  START_RECORD = 1LL << 37,
  START_TAPE = 1LL << 38
};

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
extern void     (*tape_hook) (int code);
extern int      get_7track_record (FILE *f, word_t **buffer);
extern int      get_9track_record (FILE *f, word_t **buffer);
extern void     write_7track_record (FILE *f, word_t *buffer, int);
extern void     write_9track_record (FILE *f, word_t *buffer, int);
extern void     write_tape_mark (FILE *f);
extern void     write_tape_eof (FILE *f);
extern void     write_tape_eot (FILE *f);
extern void     write_tape_gap (FILE *f, unsigned code);
extern void     write_tape_error (FILE *f, unsigned code);
extern word_t	get_core_word (FILE *f);
extern void	write_core_word (FILE *f, word_t word);

#endif /* LIBWORD_H */
