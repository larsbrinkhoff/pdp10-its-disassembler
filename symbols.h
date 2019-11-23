/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2018, 2019 Adam Sampson <ats@offog.org>

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

#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "dis.h"

#define SYMBOL_GLOBAL       (1 << 0)
#define SYMBOL_HALFKILLED   (1 << 1)
#define SYMBOL_KILLED       (1 << 2)

struct symbol
{
  const char *name;
  word_t value;
  int sequence;
  int flags;
  /* XXX struct symbol *next; for multiple results? */
};

extern void add_symbol (const char *name, word_t value, int flags);
extern const struct symbol *get_symbol_by_value (word_t value);

#endif
