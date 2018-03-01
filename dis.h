/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>

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

#include <stdio.h>
#include <stdlib.h>

#define WORDMASK	(0777777777777LL)
#define SIGNBIT		(0400000000000LL)

/* ITS page size, in words */
#define ITS_PAGESIZE 1024

typedef long long word_t;

#define JRST_1 ((word_t)(0254000000001LL))

enum { FORMAT_BIN, FORMAT_ITS, FORMAT_X, FORMAT_DTA };

struct FILE;
struct pdp10_file;
struct pdp10_memory;

extern int	file_36bit_format;
extern word_t	get_word (FILE *f);
extern word_t	get_checksummed_word (FILE *f);
extern void	reset_checksum (word_t);
extern void	check_checksum (word_t);
extern void	rewind_word (FILE *f);
extern void	dis_pdump (FILE *f, int cpu_model);
extern void	dis_sblk (FILE *f, int cpu_model);
extern void	read_pdump (FILE *f, struct pdp10_memory *memory, int cpu);
extern void	read_sblk (FILE *f, struct pdp10_memory *memory, int cpu);
extern void	read_raw (FILE *f, struct pdp10_memory *memory, int cpu);
extern void	sblk_info (FILE *f, word_t word0, int cpu_model);
extern void	dis (struct pdp10_memory *memory, int cpu_model);
extern void	disassemble_word (struct pdp10_memory *memory, word_t word,
				  int address, int cpu_model);
extern void	sixbit_to_ascii (word_t sixbit, char *ascii);
extern void	print_date (FILE *, word_t t);
extern void	print_datime (FILE *, word_t t);
extern int	byte_size (int, int *);

#endif /* DIS_H */
