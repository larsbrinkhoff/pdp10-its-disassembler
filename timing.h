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
