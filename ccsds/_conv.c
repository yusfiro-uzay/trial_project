#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
//#include "unistd_win.h"
#else
#include <unistd.h>
#endif
//#include <sys/time.h>
//#include <sys/resource.h>
#include <memory.h>
#include <math.h>

#include "rs/fec.h"
#include "conv/viterbi27.c"
#include "conv/cpu_mode_ppc.c"
#include "conv/viterbi27_port.c"
#include "conv/fec.c"

#define MAXBYTES 40936*100
#define FRAMEBITS 40936*8*100
//unsigned char symbols[8 * 2 * (MAXBYTES + 6)] = {0};

void bits_to_bits_conv(unsigned char *bits, unsigned char *_out_bits_255,int *bit_len, int high_bit_value) {
  for (int i = 0; i < bit_len; i++) _out_bits_255[i] =(unsigned char) (bits[i] * high_bit_value);
}
void hex_to_bits_conv(unsigned char *hex_data, unsigned char *bits, int *bit_len, int high_bit_value, int hex_len){
    *bit_len = 0;
    int i;

    for(i = 0; i < hex_len; i++){

        unsigned char value = hex_data[i];
        int j;
        for(j = 7; j >= 0; j--){
            bits[(*bit_len)++] = (value & (1 << j)) ? high_bit_value : 0;
        }
    }
}



void convolutional_decode_viterbi(unsigned char *input, unsigned char *output,int noutputitemsnumber){
    int  bit_len = 0;
    void *vp;
    size_t count = (size_t)2 * noutputitemsnumber * 8 + 6;
    unsigned char *symbols = calloc(count, sizeof(unsigned char));

    if ((vp = create_viterbi27(noutputitemsnumber*8)) == NULL) {
        printf("create_viterbi27 failed \n");
    }
        hex_to_bits_conv(input, symbols, &bit_len, 255, noutputitemsnumber*2 );
    
    //bits_to_bits_conv(input, symbols, noutputitemsnumber, 255);
    /* Initialize Viterbi decoder */
    init_viterbi27(vp, 0);

    /* Decode block */
    update_viterbi27_blk(vp, symbols, (noutputitemsnumber*8+6) );

    /* Do Viterbi chainback */
    chainback_viterbi27(vp, output, noutputitemsnumber*8, 0);

    delete_viterbi27(vp);
    free(symbols);
}

/* GRLIB kodu
#define MAX_CONV_SIZE 1279*2

void convolve_decode(unsigned char *input, unsigned char *output){
    int c1 = 0, c2 = 0, c3 = 0, odd = 1;
    unsigned char r1[6] = {0}, r2[6] = {0}, r[7] = {0}, doutar[8]={0};
    unsigned char dinar[8] = {0}; // Keep 8 bits of data
    unsigned char h, g1 = 0, g2 = 0;
    int dout = 0;
    int idx, j;
    //Each byte must be processed individually
    for (idx = 0; idx < MAX_CONV_SIZE; idx ++){
        for(j = 0; j < 8; j ++){
            dinar[j] = (input[idx] & (1 << (7 - j))) ? 1 : 0; // Check every bit of the byte
        }


        int i, k, x = 0;
        for(i = 0; i < 8; i++){
           if(odd){
               // H1 decoder process
               for (k = 5; k > 0; k--){
                   r1[k] = r1[k - 1];
               }
               r1[1] = dinar[i];
               if(c2 >= 12){
                   c1 = c2 = c3 = 0;
                   odd = 1;
               } else if (g2 != r2[3]){
                   c2++;
                   c3 = 0;
                   odd = 0;
               } else {
                   if (c3 < 15) {
                       c3++;
                   } else {
                       c3 = 15;
                       c2 = 0;
                       c1 = 1;
                   }
                   odd = 0;
               }
           } else {
               // H2 decoder process
               for (k = 5; k > 0; k--) {
                   r2[k] = r2[k - 1];
               }
               r2[1] = dinar[i];

               // H1 & H2 result
               h = (r1[1] ^ r1[5]) ^ !(r2[1] ^ r2[2] ^ r2[3] ^ r2[4] ^ r2[5]);

               // Save the decoded data
               doutar[x++] = h;

               // The decoded data is encoded internally for checking
               // that it corresponds to the incoming bit stream.
               g1 = h ^ r[1] ^ r[2] ^ r[3] ^ r[6];
               g2 = !(h ^ r[2] ^ r[3] ^ r[5] ^ r[6]);

               // Shift register process
               for (k = 6; k > 0; k--) {
                   r[k] = r[k - 1];
               }
               r[1] = h;

               // Error limit check
               if (c1 >= 12) {
                   c1 = c2 = c3 = 0;
                   odd = 0;
               } else if (g1 != r1[3]) {
                   c1++;
                   odd = 1;
               } else {
                   odd = 1;
               }
           }
        }
        dout = 0;
        int j;
        for (j = 0; j < 4; j++) {
            dout = dout | ((doutar[j] & 1) << (3 - j % 4));
        }
        output[idx] = dout & 0xff;
    }
}
*/
