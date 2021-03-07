
CFLAGS = -g -W -Wall

FILES =  sblk-file.o pdump-file.o dmp-file.o raw-file.o shr-file.o \
	 mdl-file.o

WORDS =  aa-word.o bin-word.o cadr-word.o core-word.o data8-word.o \
	 dta-word.o its-word.o oct-word.o pt-word.o tape-word.o x-word.o

OBJS =	pdp10-opc.o info.o dis.o symbols.o \
	timing.o timing_ka10.o timing_ki10.o memory.o weenix.o

UTILS =	conv36 bin2ascii bin2x its2x its2bin its2rim itsarc magdmp magfrm dskdmp \
	macdmp macro-tapes tape-dir harscntopbm palx its2ascii \
	tracks ipak kldcp klfedr scrmbl unscr tvpic tito

all: dis10 $(UTILS) check

clean:
	rm -f $(OBJS) $(WORDS) libfiles.a libwords.a
	rm -f dis10 core
	rm -f $(UTILS)
	rm -f main.o dmp.o raw.o das.o crypt.o
	for f in $(UTILS); do rm -f $${f}.o; done
	rm -f out/*

dis10: main.o $(OBJS) libfiles.a libwords.a
	$(CC) $(CFLAGS) $^ -o $@

libfiles.a: file.o $(FILES)
	ar -crs $@ $^

libwords.a: word.o $(WORDS)
	ar -crs $@ $^

conv36: conv36.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

bin2ascii: bin2ascii.o
	$(CC) $(CFLAGS) $^ -o $@

bin2x: bin2x.o
	$(CC) $(CFLAGS) $^ -o $@

its2x: its2x.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

its2bin: its2bin.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

its2rim: its2rim.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

its2ascii: its2ascii.o libwords.a
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

tracks: tracks.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

tito: tito.o $(OBJS) libwords.a
	$(CC) $(CFLAGS) $^ -o $@

harscntopbm: harscntopbm.o libwords.a
	$(CC) $(CFLAGS) $^ -o $@

palx: palx.o $(OBJS) libwords.a
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

check: \
	out/ts.obs.dasm out/ts.ksfedr.dasm out/ts.name.dasm \
	out/ts.srccom.dasm out/atsign.tcp.dasm out/arc.code.list \
	out/macro.low.dasm out/pt.rim.dasm out/visib1.bin.dasm \
	out/visib2.bin.dasm out/visib3.bin.dasm out/@.midas.dasm \
	out/srccom.exe.dasm out/dart.dmp.dasm \
	out/stink.-ipak-.ipak \
	out/thirty.scrmbl out/sixbit.scrmbl out/pdpten.scrmbl \
	out/aaaaaa.scrmbl out/0s.scrmbl

samples/ts.obs = -Wits
samples/ts.ksfedr = -Wits
samples/ts.name = -Wits -Sall
samples/ts.srccom = -Wits
samples/atsign.tcp = -Wits
samples/macro.low = -r -Wascii
samples/pt.rim = -r -Wpt
samples/visib1.bin = -Wits -Sddt
samples/visib2.bin = -Wits -Sddt
samples/visib3.bin = -Wits -Sall
samples/@.midas = -D774000 -Sall
samples/stink.-ipak- = -Wascii
samples/srccom.exe = -Wascii
samples/dart.dmp = -6 -Wdata8

out/%.dasm: samples/% dis10 test/%.dasm
	./dis10 $($<) $< > $@
	cmp $@ test/$*.dasm || rm $@ /no-such-file

out/%.list: samples/% itsarc test/%.list
	./itsarc -t $< 2> $@
	cmp $@ test/$*.list || rm $@ /no-such-file

out/%.ipak: samples/% ipak test/%.ipak
	./ipak -t $($<) $< 2> $@
	cmp $@ test/$*.ipak || rm $@ /no-such-file

out/%.scrmbl: samples/zeros.%.scrmbl scrmbl its2bin samples/zeros.scrmbl
	./scrmbl -Wbin $* samples/zeros.scrmbl $@
	./its2bin $@ | cmp - $< || rm $@ /no-such-file
	./scrmbl -d -Wits $* $@ out/$*.unscrm
	./its2bin out/$*.unscrm | cmp - samples/zeros.scrmbl \
		|| rm $@ /no-such-file

#dependencies
bin-word.o: bin-word.c dis.h
bin2ascii.o: bin2ascii.c
bin2x.o: bin2x.c
conv36.o: dis.h
data8-word.o: data8-word.c dis.h
dis.o: dis.c opcode/pdp10.h dis.h memory.h timing.h
info.o: info.c dis.h memory.h
its-word.o: its-word.c dis.h
its2bin.o: its2bin.c dis.h
its2x.o: its2x.c dis.h
main.o: main.c dis.h opcode/pdp10.h memory.h
memory.o: memory.c memory.h dis.h
oct-word.o: oct-word.c dis.h
pdp10-opc.o: pdp10-opc.c opcode/pdp10.h
pdump.o: pdump.c dis.h memory.h
sblk.o: sblk.c dis.h memory.h
scrmbl.o: scrmbl.c dis.h
timing.o: timing.c opcode/pdp10.h timing.h dis.h
timing_ka10.o: timing_ka10.c opcode/pdp10.h dis.h timing.h
timing_ki10.o: timing_ki10.c opcode/pdp10.h dis.h timing.h
word.o: word.c dis.h
x-word.o: x-word.c dis.h
