CC = gcc
CFLAGS = -g -W -Wall

WORDS =  bin-word.o its-word.o x-word.o dta-word.o aa-word.o pt-word.o core-word.o tape-word.o

OBJS =	pdp10-opc.o info.o word.o sblk.o pdump.o dis.o symbols.o \
	timing.o timing_ka10.o timing_ki10.o memory.o

UTILS =	bin2ascii bin2x its2x its2bin its2rim itsarc magdmp magfrm dskdmp \
	macdmp saildart macro-tapes tape-dir harscntopbm palx its2ascii \
	tracks ipak

all: dis10 $(UTILS) check

clean:
	rm -f $(OBJS)
	rm -f dis10 core
	rm -f $(UTILS)
	rm -f main.o dmp.o raw.o
	for f in $(UTILS); do rm -f $${f}.o; done
	rm -f *.dasm *.list

dis10: main.o $(OBJS) dmp.o raw.o libwords.a
	gcc $^ -o dis10

libwords.a: word.o $(WORDS)
	ar -crs $@ $^

bin2ascii: bin2ascii.o
	$(CC) bin2ascii.o -o bin2ascii

bin2x: bin2x.o
	$(CC) bin2x.o -o bin2x

its2x: its2x.o libwords.a
	$(CC) $^ -o its2x

its2bin: its2bin.o libwords.a
	$(CC) $^ -o its2bin

its2rim: its2rim.o libwords.a
	$(CC) $^ -o its2rim

its2ascii: its2ascii.o libwords.a
	$(CC) $^ -o $@

dskdmp: dskdmp.c $(OBJS) libwords.a
	$(CC) $^ -o $@

macdmp: macdmp.c $(OBJS) libwords.a
	$(CC) $^ -o $@

saildart: saildart.o $(OBJS) libwords.a
	$(CC) $^ -o $@

magdmp: magdmp.c core-word.o $(OBJS) libwords.a
	$(CC) $^ -o $@

magfrm: magfrm.c core-word.o $(OBJS) libwords.a
	$(CC) $^ -o $@

ipak: ipak.o $(OBJS) libwords.a
	$(CC) $^ -o $@

itsarc: itsarc.o $(OBJS) libwords.a
	$(CC) $^ -o $@

macro-tapes: macro-tapes.o $(OBJS) libwords.a
	$(CC) $^ -o $@

tape-dir: tape-dir.o $(OBJS) libwords.a
	$(CC) $^ -o $@

tracks: tracks.o $(OBJS) libwords.a
	$(CC) $^ -o $@

harscntopbm: harscntopbm.o libwords.a
	$(CC) $^ -o $@

palx: palx.o libwords.a
	$(CC) $^ -o $@

test/test_write: test/test_write.o $(OBJS) libwords.a
	$(CC) $^ -o $@

test/test_read: test/test_read.o $(OBJS) libwords.a
	$(CC) $^ -o $@

check: ts.obs.dasm ts.ksfedr.dasm ts.name.dasm ts.srccom.dasm atsign.tcp.dasm arc.code.list macro.low.dasm pt.rim.dasm visib1.bin.dasm visib2.bin.dasm visib3.bin.dasm @.midas.dasm

samples/ts.obs = -Wits
samples/ts.ksfedr = -Wits
samples/ts.name = -Wits
samples/ts.srccom = -Wits
samples/atsign.tcp = -Wits
samples/macro.low = -r -Wascii
samples/pt.rim = -r -Wpt
samples/visib1.bin = -Wits -Sddt
samples/visib2.bin = -Wits -Sddt
samples/visib3.bin = -Wits -Sall
samples/@.midas = -D774000 -Sall

%.dasm: samples/% dis10 test/%.dasm
	./dis10 $($<) $< > $@
	cmp $@ test/$@ || rm $@ /no-such-file

%.list: samples/% itsarc test/%.list
	./itsarc -t $< 2> $@
	cmp $@ test/$@ || rm $@ /no-such-file

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
saildart.o: saildart.c dis.h
sblk.o: sblk.c dis.h memory.h
timing.o: timing.c opcode/pdp10.h timing.h dis.h
timing_ka10.o: timing_ka10.c opcode/pdp10.h dis.h timing.h
timing_ki10.o: timing_ki10.c opcode/pdp10.h dis.h timing.h
word.o: word.c dis.h
x-word.o: x-word.c dis.h
