#ifndef MEMORY_H
#define MEMORY_H

#include "dis.h"

struct pdp10_area
{
  int		start;
  int		end;
  char *	data;
};

struct pdp10_memory
{
  int			areas;
  struct pdp10_area *	area;
  struct pdp10_area *	current_area;
  int			current_address;
};

extern void	init_memory (struct pdp10_memory *memory);
extern int	add_memory (struct pdp10_memory *memory,
			    int address, int length, char *data);
extern int	set_address (struct pdp10_memory *memory, int address);
extern int	get_address (struct pdp10_memory *memory);
extern word_t	get_next_word (struct pdp10_memory *memory);
extern word_t	get_word_at (struct pdp10_memory *memory, int address);

#endif /* MEMORY_H */
