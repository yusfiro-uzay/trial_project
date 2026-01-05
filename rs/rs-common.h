#ifndef RS_COMMON_H
#define RS_COMMON_H

#include <stdio.h>
typedef unsigned char data_t;

struct rs {
    unsigned int mm;         /* Bits per symbol */
    unsigned int nn;         /* Symbols per block (= (1<<mm)-1) */
    unsigned char* alpha_to; /* log lookup table */
    unsigned char* index_of; /* Antilog lookup table */
    unsigned char* genpoly;  /* Generator polynomial */
    unsigned int nroots;     /* Number of generator roots = number of parity symbols */
    unsigned char fcr;       /* First consecutive root, index form */
    unsigned char prim;      /* Primitive element, index form */
    unsigned char iprim;     /* prim-th root of 1, index form */
    int* modnn_table;        /* modnn lookup table, 512 entries */
};


static inline int modnn(struct rs* rs, int x) {
    while (x >= rs->nn) {
        x -= rs->nn;
        x = (x >> rs->mm) + (x & rs->nn);
    }
    return x;
}
#endif /* RS_COMMON_H */
