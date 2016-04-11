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

#include "dis.h"

extern int instruction_time (word_t instruction, int cpu_model);
extern int timing_ka10 (word_t instruction);
extern int timing_ki10 (word_t instruction);

extern int memory_read (word_t instruction);
extern int memory_read_modify_write (word_t instruction);
extern int memory_write (word_t instruction);
extern int floating_point_immediate (word_t instruction);
extern int accumulator_read (word_t instruction);
extern int accumulator_write (word_t instruction);
