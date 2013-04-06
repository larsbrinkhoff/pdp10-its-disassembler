#ifndef DIS_H
#define DIS_H

#include <stdio.h>

#define WORDMASK	(0777777777777LL)
#define SIGNBIT		(0400000000000LL)

/* ITS page size, in words */
#define ITS_PAGESIZE 1024

typedef long long word_t;

#define JRST_1 ((word_t)(0254000000001LL))

enum { FORMAT_BIN, FORMAT_ITS, FORMAT_X };

struct FILE;
struct pdp10_file;
struct pdp10_memory;

extern int	file_36bit_format;
extern word_t	get_word (FILE *f);
extern void	rewind_word (FILE *f);
extern void	dis_pdump (FILE *f, int cpu_model);
extern void	dis_sblk (FILE *f, int cpu_model);
extern void	read_pdump (FILE *f, struct pdp10_memory *memory, int cpu);
extern void	read_sblk (FILE *f, struct pdp10_memory *memory, int cpu);
extern void	sblk_info (FILE *f, word_t word0, int cpu_model);
extern void	dis (struct pdp10_memory *memory, int cpu_model);
extern void	disassemble_word (struct pdp10_memory *memory, word_t word,
				  int address, int cpu_model);
extern void	sixbit_to_ascii (word_t sixbit, char *ascii);

#endif /* DIS_H */
