#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/*
int printarray(unsigned char *out, unsigned char *m, int len) {

#if 1
	memcpy(out,m,len);
	return len;
#else

	int s; char b[64]; int l = 0, _l, i;

	if (!len)
		return 0;

	for (i=0; i < len; i++) {
		s=m[i] & 0xff;
		sprintf(b,"%02X",s & 0xff);
		_l = strlen(b);
		l += _l;
		ccsds_out_put(out, b, _l);
	};

	return l;

#endif

}*/

//#include "ccsds_io.h"

/*--------------------------------------------------------------------------- */
/* This function pseudo-randomises the incoming bit stream using the          */
/* following standard polynomial.                                             */
/*                            TM: h(x) = x^8 + x^7 + x^5 + x^3 + 1.           */
/*                            TC: h(x) = x^8 + x^6 + x^4 + x^3 + x^2 + x + 1  */
/*                                                                            */
/*    +---XOR<----XOR<----XOR<--------------+                                 */
/*    |    ^       ^       ^                |                                 */
/*    |    |       |       |                |                                 */
/*    |  +-+-+---+-+-+---+-+-+---+---+---+  |                                 */
/*    +->| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |--+                                 */
/*       +---+---+---+---+---+---+---+---+                                    */
/*                                                                            */
/*    x^8 x^7 x^6 x^5 x^4 x^3 x^2 x^1 x^0                                     */
/*                                                                            */
/*     (0 xor 3 xor 5 xor 7) & 7654321 => 7 & 6543210                         */
/*                                                                            */
/* Many-to-one implementation: Fibonacci version of LFSR                      */
/*                                                                            */
/* The generator is initialised to all ones.                                  */
/* The generated pseudo-random bit stream is xor-ed with the incoming data.   */
/*--------------------------------------------------------------------------- */
static void ccsds_psr_lfsr(
	unsigned char* lfsr, /* 8*1 bit per char */
	int spsr_tc
)
{
	spsr_tc = 0;
	unsigned char  feedback;

	/* feed back */
	if (spsr_tc) {
		feedback = lfsr[0] ^ lfsr[1] ^ lfsr[2] ^ lfsr[3] ^ lfsr[4] ^ lfsr[6];
	}
	else {
		feedback = lfsr[0] ^ lfsr[3] ^ lfsr[5] ^ lfsr[7];
	}

	/* shift lfsr */
	lfsr[0] = lfsr[1];
	lfsr[1] = lfsr[2];
	lfsr[2] = lfsr[3];
	lfsr[3] = lfsr[4];
	lfsr[4] = lfsr[5];
	lfsr[5] = lfsr[6];
	lfsr[6] = lfsr[7];
	lfsr[7] = feedback;
};


/* (de-)randomise data */
void ccsds_psr_data(
	int    length,
	unsigned char* psr_data, unsigned char* psr_line, unsigned char* psr_code, int spsr_tc)
{
	unsigned char  lfsr[8];
	int            i;/*,j;*/

	/* expand array of octets to array of bits */
	for (i = 0; i < length * 8; i++)
		psr_line[i] = (psr_data[i / 8] >> (7 - i % 8)) & 1;

	/* initialise lfsr */
	for (i = 0; i < 8; i++) lfsr[i] = 1;

	/* (de-)randomise data bit stream*/
	for (i = 0; i < length * 8; i++) {
		psr_line[i] = psr_line[i] ^ lfsr[0];
		ccsds_psr_lfsr(lfsr, spsr_tc);
	};

	/* initialise output */
	for (i = 0; i < length; i++) psr_code[i] = 0;

	/* collapse array of bits to array of octets */
	for (i = 0; i < length * 8; i++)
		psr_code[i / 8] = psr_code[i / 8] | psr_line[i] << (7 - i % 8);
};

/* struct ccsds_state *state */
unsigned char* psr_data, * psr_code, * psr_line;
int psr_tc;
int psr_init = 0;

void ccsds_state_psr_init(int tc) {
	psr_data = (unsigned char*)malloc(16384);
	psr_code = (unsigned char*)malloc(16384);
	psr_line = (unsigned char*)malloc(131072);
	psr_tc = tc;
}

int ccsds_do_psr(unsigned char* out, unsigned char* m, int len) {

	/*struct ccsds_frame_param *p*/
	int synclength = 4;
	int framelength = len * 4;
	static int framecntr = 0;

	int i;



	if (!psr_init) {
		ccsds_state_psr_init(0);
		psr_init++;
	}

	int ret = 0, al = len - synclength;
	unsigned char* h = m;
	//+ synclength;
	if (al > 0) {
		if (al > framelength)
			al = framelength;
		memcpy(psr_data, h, al);
		if (al < framelength)
			memset(psr_data + al, 0, framelength - al);

	}

	ccsds_psr_data(al, psr_data, psr_line, out, psr_tc);

	/*sleep(2);
	printf("\nrmgerwkmof�mp�erg�erger\n");
	for (i = 0; i < (1279); i++) {
		printf("%02X ", psr_code[i]);
	}
	sleep(3);
	*out = psr_code;*/
	framecntr++;

	return ret;

}

