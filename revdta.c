#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "pdp6common.h"

#define RELOC ((word)01000000000000)
#define NRELOC ((word)02000000000000)
#define LRELOC ((word)04000000000000)
#define LNRELOC ((word)010000000000000)

#define W 0777777777777

#include "data.inc"

typedef unsigned char uchar;
typedef unsigned int uint;
#define nil NULL

#define DTLEN (2*01102*0200)

word dtbuf[DTLEN];

#define LDB(p, s, w) ((w)>>(p) & (1<<(s))-1)
#define XLDB(ppss, w) LDB((ppss)>>6 & 077, (ppss)&077, w)
#define MASK(p, s) ((1<<(s))-1 << (p))
#define DPB(b, p, s, w) ((w)&~MASK(p,s) | (b)<<(p) & MASK(p,s))
#define XDPB(b, ppss, w) DPB(b, (ppss)>>6 & 077, (ppss)&077, w)

void
writesimh(FILE *f, word w)
{
	uchar c[8];
	uint l, r;

	r = LDB(0, 18, w);
	l = LDB(18, 18, w);
	c[0] = LDB(0, 8, l);
	c[1] = LDB(8, 8, l);
	c[2] = LDB(16, 8, l);
	c[3] = LDB(24, 8, l);
	c[4] = LDB(0, 8, r);
	c[5] = LDB(8, 8, r);
	c[6] = LDB(16, 8, r);
	c[7] = LDB(24, 8, r);
	fwrite(c, 1, 8, f);
}

word
readsimh(FILE *f)
{
	uchar c[8];
	hword w[2];
	if(fread(c, 1, 8, f) != 8)
		return ~0;
	w[0] = c[3]<<24 | c[2]<<16 | c[1]<<8 | c[0];
	w[1] = c[7]<<24 | c[6]<<16 | c[5]<<8 | c[4];
	return ((word)w[0]<<18 | w[1]) & 0777777777777;
}

word
revobv(word w)
{
	word r;
	int i;

	r = 0;
	for(i = 0; i < 12; i++){
		r <<= 3;
		r |= ~w & 7;
		w >>= 3;
	}
	return r;
}

int
main()
{
	int i;
	word w;

	i = 0;
	while(w = readsimh(stdin), w != ~0){
//		printf("%012lo\n", revobv(w));
		dtbuf[i++] = revobv(w);
	}
	while(--i >= 0)
//		printf("%012lo\n", dtbuf[i]);
		writesimh(stdout, dtbuf[i]);

	return 0;
}
