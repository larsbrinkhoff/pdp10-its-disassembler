CC = gcc
CFLAGS = -g -W -Wall

OBJS =	pdp10-opc.o info.o word.o bin-word.o its-word.o x-word.o dta-word.o \
	sblk.o pdump.o dis.o timing.o timing_ka10.o timing_ki10.o memory.o
	#file.o

UTILS =	bin2ascii bin2x its2x its2bin its2rim itsarc magdmp magfrm dskdmp \
	macdmp

all: dis10 $(UTILS)

clean:
	rm -f $(OBJS)
	rm -f dis10 core
	rm -f $(UTILS)
	rm -f bin2ascii.o bin2x.o its2x.o its2bin.o

dis10: main.o $(OBJS) raw.o
	gcc $^ -o dis10

bin2ascii: bin2ascii.o
	$(CC) bin2ascii.o -o bin2ascii

bin2x: bin2x.o
	$(CC) bin2x.o -o bin2x

its2x: its2x.o word.o bin-word.o its-word.o x-word.o
	$(CC) its2x.o word.o bin-word.o its-word.o x-word.o -o its2x

its2bin: its2bin.o word.o bin-word.o its-word.o x-word.o
	$(CC) its2bin.o word.o bin-word.o its-word.o x-word.o -o its2bin

its2rim: its2rim.o word.o bin-word.o its-word.o x-word.o
	$(CC) its2rim.o word.o bin-word.o its-word.o x-word.o -o its2rim

dskdmp: dskdmp.c $(OBJS)
	$(CC) $^ -o $@

macdmp: macdmp.c $(OBJS)
	$(CC) $^ -o $@

magdmp: magdmp.c core-word.o $(OBJS)
	$(CC) $^ -o $@

magfrm: magfrm.c core-word.o $(OBJS)
	$(CC) $^ -o $@

itsarc: itsarc.o $(OBJS)
	$(CC) $^ -o $@

#dependencies
bin-word.o: bin-word.c dis.h
bin2ascii.o: bin2ascii.c
bin2x.o: bin2x.c
dis.o: dis.c opcode/pdp10.h dis.h memory.h timing.h
info.o: info.c dis.h memory.h
its-word.o: its-word.c dis.h
its2bin.o: its2bin.c dis.h
its2x.o: its2x.c dis.h
main.o: main.c dis.h opcode/pdp10.h memory.h
memory.o: memory.c memory.h dis.h
pdp10-opc.o: pdp10-opc.c opcode/pdp10.h
pdump.o: pdump.c dis.h memory.h
sblk.o: sblk.c dis.h memory.h
timing.o: timing.c opcode/pdp10.h timing.h dis.h
timing_ka10.o: timing_ka10.c opcode/pdp10.h dis.h timing.h
timing_ki10.o: timing_ki10.c opcode/pdp10.h dis.h timing.h
word.o: word.c dis.h
x-word.o: x-word.c dis.h
