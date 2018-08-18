#ifndef CPU_H
#define CPU_H

extern void pure_page (int);
extern void unpure_page (int);
extern void unmapped_page (int);
extern void invalidate_word (int);
extern void run (int, struct pdp10_memory *);

extern int PC;
extern int flags;
extern word_t IR;
extern word_t AR;
extern word_t BR;
extern word_t MQ;
extern word_t MA;
extern word_t MB;
extern word_t FM[16];
extern struct pdp10_memory *memory;

#endif /* CPU_H */
