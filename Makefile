CC = gcc
CFLAGS = -m32 -g -W -Wall -I. -fomit-frame-pointer -freg-struct-return

WORDS =  bin-word.o its-word.o x-word.o dta-word.o aa-word.o pt-word.o core-word.o

OBJS =	pdp10-opc.o info.o word.o sblk.o pdump.o dis.o symbols.o \
	timing.o timing_ka10.o timing_ki10.o memory.o $(WORDS)

UTILS =	bin2ascii bin2x its2x its2bin its2rim itsarc magdmp magfrm dskdmp \
	macdmp saildart macro-tapes tape-dir harscntopbm palx

all: dis10 $(UTILS)

clean:
	rm -f $(OBJS)
	rm -f dis10 core
	rm -f $(UTILS)
	rm -f main.o dmp.o raw.o
	rm -f cpu/*.o
	for f in $(UTILS); do rm -f $${f}.o; done
	rm -f *.dasm *.list
	rm -f ts.d.txt

dis10: main.o $(OBJS) dmp.o raw.o cpu/cpu.o cpu/its.o
	gcc -m32 -g $^ -o dis10

bin2ascii: bin2ascii.o
	$(CC) bin2ascii.o -o bin2ascii

bin2x: bin2x.o
	$(CC) bin2x.o -o bin2x

its2x: its2x.o word.o $(WORDS)
	$(CC) $^ -o its2x

its2bin: its2bin.o word.o $(WORDS)
	$(CC) $^ -o its2bin

its2rim: its2rim.o word.o $(WORDS)
	$(CC) $^ -o its2rim

dskdmp: dskdmp.c $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

macdmp: macdmp.c $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

saildart: saildart.o $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

magdmp: magdmp.c core-word.o $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

magfrm: magfrm.c core-word.o $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

itsarc: itsarc.o $(OBJS) cpu/stub.o cpu/stub.o
	$(CC) $^ -o $@

macro-tapes: macro-tapes.o $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

tape-dir: tape-dir.o $(OBJS) cpu/stub.o
	$(CC) $^ -o $@

harscntopbm: harscntopbm.o word.o $(WORDS)
	$(CC) $^ -o $@

palx: palx.o word.o $(WORDS)
	$(CC) $^ -o $@

test/test_write: test/test_write.o $(OBJS)
	$(CC) $^ -o $@

test/test_read: test/test_read.o $(OBJS)
	$(CC) $^ -o $@

ts.d.txt: dis10 samples/ts.d
	./dis10 -E samples/ts.d > $@ 2> /dev/null
	grep ':KILL' $@ > /dev/null

check: ts.obs.dasm ts.ksfedr.dasm ts.name.dasm ts.srccom.dasm atsign.tcp.dasm arc.code.list macro.low.dasm pt.rim.dasm visib1.bin.dasm visib2.bin.dasm visib3.bin.dasm ts.d.txt

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
