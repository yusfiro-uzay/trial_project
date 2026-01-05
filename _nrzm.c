
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_CADU_SIZE		1115

struct sync_mark{
	
	int previous;
	int inverse;
	unsigned char v[4];
	
	};
/*NRZ-M decoding function*/

void math_nrzm_decode(unsigned char *input, unsigned char *output, struct sync_mark *v, int len){
	int i;
	for (i=0; i<len; i++) {
		if ((input[i] ^ v->previous) == 1)
			output[i]=1 ^ v->inverse;
		else
			output[i]=0 ^ v->inverse;
		v->previous=input[i];
	}
}
void math_nrzm_encode(unsigned char *input, unsigned char *output, struct sync_mark *v, int len){
	int i;
	for (i=0; i<len; i++) {
		if ((input[i] ^ v->previous) == 1)
			output[i]=1 ^ v->inverse;
		else
			output[i]=0 ^ v->inverse;
		v->previous=output[i];
	}
}
void bits_to_hex(unsigned char *bits, unsigned char *hex, int len){
	int i, j;
	for (i=0; i<len / 8; i++) {
		hex[i] = 0;
		for (j=0; j<8; j++){
			hex[i] |= bits[i * 8 + j] << (7 - j);
			}
	}
}
void hex_to_bits(const unsigned char *hex, unsigned char *bits, int len){
	int i, j;
	for (i=0; i<len; i++) {
		unsigned char c = hex[i];
		for (j=0; j<8; j++){
			 bits[i * 8 + j] = (c & (1 << (7 -j))) ? 1 : 0;
			}
	}
}
void nrzm_decode(const unsigned char *input, unsigned char *output, int len, int inverse_decode){
	struct sync_mark v = { 0, inverse_decode, { 0, 0, 0, 0 }};
	unsigned char bits[MAX_CADU_SIZE*16];
	unsigned char decoded[MAX_CADU_SIZE];
	
	int processed_bytes = 0;
	
	while(processed_bytes< len){
		
		int chunk_size = (len - processed_bytes >= 8) ? 8: (len - processed_bytes);
		hex_to_bits(input + processed_bytes, bits, chunk_size);
		math_nrzm_decode(bits,decoded, &v, chunk_size *8);
		bits_to_hex(decoded, output + processed_bytes, chunk_size *8);
		processed_bytes += chunk_size;
		}
}
void nrzm_encode(const unsigned char *input, unsigned char *output, int len, int inverse_decode){
	struct sync_mark v = { 0, inverse_decode, { 0, 0, 0, 0 }};
	unsigned char bits[MAX_CADU_SIZE*16];
	unsigned char decoded[MAX_CADU_SIZE];
	
	int processed_bytes = 0;
	
	while(processed_bytes< len){
		
		int chunk_size = (len - processed_bytes >= 8) ? 8: (len - processed_bytes);
		hex_to_bits(input + processed_bytes, bits, chunk_size);
		math_nrzm_encode(bits,decoded,&v, chunk_size *8);
		bits_to_hex(decoded, output + processed_bytes, chunk_size *8);
		processed_bytes += chunk_size;
		}
}


void ccsds_pipe_nrzm_decode(unsigned char *in, unsigned char *o, int len)
{
	int i,j;
	unsigned char octet[8];
	unsigned char coded[8];
	unsigned char num, previous, code;

	for(j=0; j<len; j++)
	{
		num = in[j];

		for (i=0; i<8; i++)
		{
			octet[7-i]=num&0x01;
			num = num>>1;
		}

		/* nrz-m decoding */
		previous = 0;
		for (i=0; i<8; i++)
		{
			coded[i] = previous^octet[i];
			previous = coded[i];
		}

		/* collapse array of bits to byte */
		code = 0;
		for (i=0; i<8; i++)
			code = code | coded[i] << (7-i%8);

		 o[j] = code;

	}

}
void ccsds_pipe_nrzm_encode(unsigned char *in, unsigned char *o, int len)
{
	int i,j;
	unsigned char octet[8];
	unsigned char coded[8];
	unsigned char num, previous, code;

	for(j=0; j<len; j++)
	{
		code = in[j];

		for (i=0; i<8; i++)
		{
			coded[i]=code>> (7 - i)&0x01;
			
		}

		/* nrz-m encoding */
		previous = 0;
		for (i=0; i<8; i++)
		{
			octet[i] = previous^coded[i];
			previous = octet[i];
		}

		/* collapse array of bits to byte */
		num = 0;
		for (i=0; i<8; i++)
			num |= octet[i] << (7-i);

		 o[j] = num;

	}

}
