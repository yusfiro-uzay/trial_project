#define BITMASK(x)      (1<<(x))
#define GETBIT(val, x)  (val>>(x) & 1)

/* Calculate CCSDS BCH(63,56) with h(x) = x^7 + x^6 + x^2 + 1
 *
 * Input: bch  - pointer to buffer where 8 byte code block will be stored
 *        data - pointer to buffer where 7 data bytes are located
 *
 *
 * Returns: A 63-bit BCH(63,56) codeblock with 1 filler bit
 */
void bch(unsigned char *bch, unsigned char *data)
{
    unsigned char res = 0;
    unsigned int tmp;
    int i,j;

    /* Process 7 information bytes */
    for (i = 0; i < 7; i++) {

        /* copy information bytes */
        bch[i] = data[i];

        for (j = 7; j >= 0; j--) {
            tmp = GETBIT(res,6) ^ GETBIT(data[i], j);
            res = (res << 1);
            if (tmp) {
                res = res ^ 0x45;
            }
            res &= 0x7f;
        }
    }

    bch[7] = (~res)<<1; /* invert and add filler */
}

/* Create CCSDS TC code blocks of data
 *
 * Takes any data block and pads it to be a multiple of 56 bits (7 bytes)
 * and for each 56-bits of data creates a 64-bit code block.
 *
 * Input: tcbf - pointer to buffer where the code blocks will be stored
                 (must be at least (1+(len-1)/7)*8 bytes)
 *        data - pointer to information data
 *        len  - length of data in bytes
 *
 * Returns: size of encoded data in bytes , i.e (1+(len-1)/7)*8
 */
unsigned int tcbf(unsigned char *tcbf, unsigned char *data, int len)
{

    int res_len = len, org_len = len;
    int i=0, k=0;

    while (len >= 7) {
        bch(&tcbf[k], &data[i]);
        i += 7;
        k += 8;
        len -= 7;
    }

    if (len > 0) {
        memset(&data[i+len], 0x55, 7-len);
        bch(&tcbf[k], &data[i]);
        res_len = org_len+7-len;
    }

    return (1+(res_len-1)/7)*8;
}

/* Creates CCSDS CLTU */
unsigned int ccsds_create_cltu(unsigned char *cltu, unsigned char *data, int len, int pss)
{

    unsigned char tail, tail_end;
    int tcbf_len;
    int i;

    /* Select tail sequence */
    if (pss) {
        tail = tail_end = 0x55;
    }
    else {
        tail = 0xC5;
        tail_end = 0x79;
    }

    /* Start sequence */
    cltu[0] = 0xEB;
    cltu[1] = 0x90;

    /* Create code blocks */
    tcbf_len = tcbf(&cltu[2], data, len);

    /* Insert tail sequence */
    for (i = 0; i < 7; i++)
        cltu[2+tcbf_len+i] = tail;

    cltu[2+tcbf_len+7] = tail_end;

    return (2+tcbf_len+8);
}
