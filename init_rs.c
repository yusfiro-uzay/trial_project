#include <stdlib.h>
#include "C:\Users\4\Documents\modem-proxy\src\ccsds\rs\rs-common.h"
#include "C:\Users\4\Documents\modem-proxy\src\ccsds\rs\char.h"
#include "C:\Users\4\Documents\modem-proxy\src\ccsds\rs\init_rs.h"
/*
struct rs* init_rs(int symsize, int gfpoly, int fcr, int prim, int nroots, int pad) {
    struct rs* rs = NULL;
    int i, j, sr, root, iprim;

    if (symsize <= 0 || symsize > 8 * sizeof(data_t)) {
        return NULL; // Invalid symbol size
    }
    if (fcr < 0 || fcr >= (1 << symsize) || prim <= 0 || prim >= (1 << symsize)) {
        return NULL; // Invalid first consecutive root or primitive element
    }
    if (nroots < 0 || nroots >= (1 << symsize)) {
        return NULL; // Can't have more roots than symbol values
    }
    if (pad < 0 || pad >= ((1 << symsize) - 1 - nroots)) {
        return NULL; // Too much padding
    }

    rs = (struct rs*)calloc(1, sizeof(struct rs));
    if (rs == NULL) {
        return NULL; // Memory allocation failed
    }

    rs->mm = symsize;
    rs->nn = (1 << symsize) - 1;
    rs->pad = pad;

    rs->alpha_to = (data_t*)malloc(sizeof(data_t) * (rs->nn + 1));
    rs->index_of = (data_t*)malloc(sizeof(data_t) * (rs->nn + 1));
    if (rs->alpha_to == NULL || rs->index_of == NULL) {
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL; // Memory allocation failed
    }

    rs->index_of[0] = A0;  // log(0) = -inf
    rs->alpha_to[A0] = 0;  // alpha^(-inf) = 0
    sr = 1;
    for (i = 0; i < rs->nn; i++) {
        rs->index_of[sr] = i;
        rs->alpha_to[i] = sr;
        sr <<= 1;
        if (sr & (1 << symsize)) {
            sr ^= gfpoly; // Apply generator polynomial
        }
        sr &= rs->nn;
    }
    if (sr != 1) {
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL;
    }

    rs->genpoly = (data_t*)malloc(sizeof(data_t) * (nroots + 1));
    if (rs->genpoly == NULL) {
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL; // Memory allocation failed
    }
    rs->fcr = fcr;
    rs->prim = prim;
    rs->nroots = nroots;

    for (iprim = 1; (iprim % prim) != 0; iprim += rs->nn)
        ;
    rs->iprim = iprim / prim;

    rs->genpoly[0] = 1;
    for (i = 0, root = fcr * prim; i < nroots; i++, root += prim) {
        rs->genpoly[i + 1] = 1;

        for (j = i; j > 0; j--) {
            if (rs->genpoly[j] != 0) {
                rs->genpoly[j] = rs->genpoly[j - 1] ^
                    rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[j]] + root)];
            }
            else {
                rs->genpoly[j] = rs->genpoly[j - 1];
            }
        }
        rs->genpoly[0] = rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[0]] + root)];
    }

    for (i = 0; i <= nroots; i++) {
        rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
    }

    return rs;
}
*/

/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
#include <stdlib.h>

#define DTYPE unsigned char

#ifndef NULL
#define NULL ((void*)0)
#endif

void FREE_RS(void* p)
{
    struct rs* rs = (struct rs*)p;

    free(rs->alpha_to);
    free(rs->index_of);
    free(rs->genpoly);
#ifdef FIXED
#elif defined(BIGSYM)
#else
    free(rs->modnn_table);
#endif
    free(rs);
}
/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits (1-8)
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 */
struct rs* INIT_RS(unsigned int symsize,
    unsigned int gfpoly,
    unsigned fcr,
    unsigned prim,
    unsigned int nroots)
{   
    struct rs* rs;
    int sr, root, iprim;
    unsigned int i, j;

    if (symsize > 8 * sizeof(DTYPE))
        return NULL; /* Need version with ints rather than chars */

    if (fcr >= (1u << symsize))
        return NULL;
    if (prim == 0 || prim >= (1u << symsize))
        return NULL;
    if (nroots >= (1u << symsize))
        return NULL; /* Can't have more roots than symbol values! */

    rs = (struct rs*)calloc(1, sizeof(struct rs));
    if (rs == NULL)
        return NULL;
    rs->mm = symsize;
    rs->nn = (1 << symsize) - 1;

    rs->alpha_to = (DTYPE*)malloc(sizeof(DTYPE) * (rs->nn + 1));
    if (rs->alpha_to == NULL) {
        free(rs);
        return NULL;
    }
    rs->index_of = (DTYPE*)malloc(sizeof(DTYPE) * (rs->nn + 1));
    if (rs->index_of == NULL) {
        free(rs->alpha_to);
        free(rs);
        return NULL;
    }

    /* Generate Galois field lookup tables */
    rs->index_of[0] = A0; /* log(zero) = -inf */
    rs->alpha_to[A0] = 0; /* alpha**-inf = 0 */
    sr = 1;
    for (i = 0; i < rs->nn; i++) {
        rs->index_of[sr] = i;
        rs->alpha_to[i] = sr;
        sr <<= 1;
        if (sr & (1 << symsize))
            sr ^= gfpoly;
        sr &= rs->nn;
    }
    if (sr != 1) {
        /* field generator polynomial is not primitive! */
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL;
    }

    /* Form RS code generator polynomial from its roots */
    rs->genpoly = (DTYPE*)malloc(sizeof(DTYPE) * (nroots + 1));
    if (rs->genpoly == NULL) {
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL;
    }
    rs->fcr = fcr;
    rs->prim = prim;
    rs->nroots = nroots;

    /* Find prim-th root of 1, used in decoding */
    for (iprim = 1; (iprim % prim) != 0; iprim += rs->nn)
        ;
    rs->iprim = iprim / prim;

    rs->genpoly[0] = 1;
    for (i = 0, root = fcr * prim; i < nroots; i++, root += prim) {
        rs->genpoly[i + 1] = 1;

        /* Multiply rs->genpoly[] by  @**(root + x) */
        for (j = i; j > 0; j--) {
            if (rs->genpoly[j] != 0)
                rs->genpoly[j] =
                rs->genpoly[j - 1] ^
                rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[j]] + root)];
            else
                rs->genpoly[j] = rs->genpoly[j - 1];
        }
        /* rs->genpoly[0] can never be zero */
        rs->genpoly[0] = rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[0]] + root)];
    }
    /* convert rs->genpoly[] to index form for quicker encoding */
    for (i = 0; i <= nroots; i++)
        rs->genpoly[i] = rs->index_of[rs->genpoly[i]];

#ifdef FIXED
#elif defined(BIGSYM)
#else
    /* Form modnn lookup table */
    rs->modnn_table = (int*)malloc(sizeof(int) * (2 << ((sizeof(unsigned char)) * 8)));
    if (rs->modnn_table == NULL) {
        free(rs->genpoly);
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return NULL;
    }
    for (i = 0; i < (2 << ((sizeof(unsigned char)) * 8)); i++) {
        j = i;
        rs->modnn_table[i] = modnn(rs, j);
    }
#endif

#if 0
    printf("genpoly:\n");
    for (i = nroots; i >= 0; i--) {
        printf("  %3d*X^%d\n", rs->alpha_to[rs->genpoly[i]], i);
    }
#endif

    return rs;
}
