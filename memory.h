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

#ifndef MEMORY_H
#define MEMORY_H

#include "dis.h"

#define MEMORY_PURE     0001

struct pdp10_area
{
  int start, end;
  unsigned flags;
  word_t *data;
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
			    int address, int length, word_t *data);
extern void	remove_memory (struct pdp10_memory *memory,
			       int address, int length);
extern void     purify_memory (struct pdp10_memory *memory, int address,
			       int length);
extern int	set_address (struct pdp10_memory *memory, int address);
extern int	get_address (struct pdp10_memory *memory);
extern word_t	get_next_word (struct pdp10_memory *memory);
extern word_t	get_word_at (struct pdp10_memory *memory, int address);
extern void	set_word_at (struct pdp10_memory *memory, int address, word_t);
extern int	pure_word_at (struct pdp10_memory *memory, int address);

#endif /* MEMORY_H */
