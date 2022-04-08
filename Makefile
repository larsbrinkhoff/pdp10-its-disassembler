
CFLAGS = -g -W -Wall

FILES =  sblk-file.o pdump-file.o dmp-file.o raw-file.o exe-file.o \
	 mdl-file.o rim10-file.o fasl-file.o palx-file.o lda-file.o \
	 cross-file.o hex-file.o atari-file.o iml-file.o exb-file.o \
	 tenex-file.o csave-file.o hiseg-file.o

WORDS =  aa-word.o alto-word.o bin-word.o cadr-word.o core-word.o \
	 data8-word.o dta-word.o its-word.o oct-word.o pt-word.o \
	 sail-word.o tape-word.o

OBJS =	pdp10-opc.o info.o dis.o symbols.o \
	timing.o timing_ka10.o timing_ki10.o memory.o weenix.o

UTILS =	cat36 itsarc magdmp magfrm dskdmp dump \
	macdmp macro-tapes tape-dir harscntopbm palx cross \
	ipak kldcp klfedr scrmbl unscr tvpic tito dart od10 \
	constantinople dumper linum

all: dis10 $(UTILS) check

clean:
	rm -f $(OBJS) $(WORDS) libfiles.a libwords.a
	rm -f dis10 core
	rm -f $(UTILS)
	rm -f main.o dmp.o raw.o das.o crypt.o
	for f in $(UTILS); do rm -f $${f}.o; done
	rm -f out/*
	rm -f check

dis10: main.o $(OBJS) libfiles.a libwords.a
	$(CC) $(CFLAGS) $^ -o $@

libfiles.a: file.o $(FILES)
	ar -crs $@ $^

libwords.a: word.o $(WORDS)
	ar -crs $@ $^

cat36: cat36.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

dump: dump.c $(OBJS) libfiles.a libwords.a
	$(CC) $(CFLAGS) $^ -o $@

dskdmp: dskdmp.c $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

macdmp: macdmp.c $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

magdmp: magdmp.c core-word.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

magfrm: magfrm.c core-word.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

ipak: ipak.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

itsarc: itsarc.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

macro-tapes: macro-tapes.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

tape-dir: tape-dir.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

tito: tito.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

dart: dart.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

dumper: dumper.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

od10: od10.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

linum: linum.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

constantinople: constantinople.o $(OBJS) libfiles.a libwords.a
	$(CC) $(CFLAGS) $^ -o $@

harscntopbm: harscntopbm.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

palx: palx.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

cross: cross.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

kldcp: kldcp.o $(OBJS) das.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

klfedr: klfedr.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

scrmbl: scrmbl.o crypt.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

unscr: unscr.o crypt.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

lodepng.c: lodepng/lodepng.cpp
	cp $< $@

lodepng.h: lodepng/lodepng.h
	cp $< $@

tvpic.o: tvpic.c lodepng.h

tvpic: tvpic.o lodepng.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

test/test_write: test/test_write.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

test/test_read: test/test_read.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

check: check.sh
	sh check.sh && touch $@

#dependencies
bin-word.o: bin-word.c dis.h
cat36.o: dis.h
data8-word.o: data8-word.c dis.h
dis.o: dis.c opcode/pdp10.h dis.h memory.h timing.h
info.o: info.c dis.h memory.h
its-word.o: its-word.c dis.h
main.o: main.c dis.h opcode/pdp10.h memory.h
memory.o: memory.c memory.h dis.h
oct-word.o: oct-word.c dis.h
pdp10-opc.o: pdp10-opc.c opcode/pdp10.h
pdump.o: pdump.c dis.h memory.h
sail-word.o: sail-word.c dis.h
sblk.o: sblk.c dis.h memory.h
scrmbl.o: scrmbl.c dis.h
timing.o: timing.c opcode/pdp10.h timing.h dis.h
timing_ka10.o: timing_ka10.c opcode/pdp10.h dis.h timing.h
timing_ki10.o: timing_ki10.c opcode/pdp10.h dis.h timing.h
word.o: word.c dis.h
x-word.o: x-word.c dis.h
