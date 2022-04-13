CFLAGS = -g -W -Wall

OBJS =	aa-word.o alto-word.o bin-word.o cadr-word.o core-word.o \
	data8-word.o dta-word.o its-word.o oct-word.o pt-word.o \
	sail-word.o tape-word.o

all: libword.a

clean:
	rm -f libword.a *.o

libword.a: word.o $(OBJS)
	$(AR) -crs $@ $^

