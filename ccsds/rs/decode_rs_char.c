/*
 * General purpose Reed-Solomon decoder for 8-bit symbols or less
 * Copyright 2003 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#ifdef DEBUG
#include <stdio.h>
#endif

#include <string.h>
#include "decode_rs.h" // Include the updated header

 // Assuming `data_t` and `struct rs` are defined in "reed_solomon_decoder.h"

int decode_rs_char(void* p, data_t* data, int* eras_pos, int no_eras) {
    int retval;

    // Cast the input pointer to the Reed-Solomon structure
    struct rs* rs = (struct rs*)p;

    // Validate the input
    if (rs == NULL || data == NULL) {
        fprintf(stderr, "Invalid parameters passed to decode_rs_char.\n");
        return -1; // Return error code for invalid parameters
    }
    // Call the decoding logic
    reed_solomon_decode(rs, data, eras_pos, no_eras, &retval);

    return retval;
}
#define min(a, b) ((a) < (b) ? (a) : (b))
#define	MIN(a,b)	((a) < (b) ? (a) : (b))

void reed_solomon_decode(struct rs* rs, data_t* data, int* eras_pos, int no_eras, int* retval)

{
    int deg_lambda, el, deg_omega;
    int i, j, r, k;
    data_t u, q, tmp, num1, num2, den, discr_r;
    data_t lambda[NROOTS + 1], s[NROOTS];	/* Err+Eras Locator poly
                       * and syndrome poly */
    data_t b[NROOTS + 1], t[NROOTS + 1], omega[NROOTS + 1];
    data_t root[NROOTS], reg[NROOTS + 1], loc[NROOTS];
    int syn_error, count;

    /* form the syndromes; i.e., evaluate data(x) at roots of g(x) */
    for (i = 0;i < NROOTS;i++)
        s[i] = data[0];
    for (j = 1;j < rs->nn;j++) {
        for (i = 0;i < NROOTS;i++) {
            if (s[i] == 0) {
                s[i] = data[j];
            }
            else {
                s[i] = data[j] ^ rs->alpha_to[MODNN(rs->index_of[s[i]] + (rs->fcr + i) * rs->prim)];
            }
        }
    }
    /* Convert syndromes to index form, checking for nonzero condition */
    syn_error = 0;
    for (i = 0;i < NROOTS;i++) {
        syn_error |= s[i];
        s[i] = rs->index_of[s[i]];
    }

    if (!syn_error) {
        /* if syndrome is zero, data[] is a codeword and there are no
         * errors to correct. So return data[] unmodified
         */
        count = 0;
        goto finish;
    }
    memset(&lambda[1], 0, NROOTS * sizeof(lambda[0]));
    lambda[0] = 1;

    if (no_eras > 0) {
        /* Init lambda to be the erasure locator polynomial */
        lambda[1] = rs->alpha_to[MODNN(rs->prim * (rs->nn - 1 - eras_pos[0]))];
        for (i = 1; i < no_eras; i++) {
            u = MODNN(rs->prim * (rs->nn - 1 - eras_pos[i]));
            for (j = i + 1; j > 0; j--) {
                tmp = rs->index_of[lambda[j - 1]];
                if (tmp != rs->nn)
                    lambda[j] ^= rs->alpha_to[MODNN(u + tmp)];
            }
        }
    }
    for (i = 0;i < NROOTS + 1;i++)
        b[i] = rs->index_of[lambda[i]];

    /*
     * Begin Berlekamp-Massey algorithm to determine error+erasure
     * locator polynomial
     */
    r = no_eras;
    el = no_eras;
    while (++r <= NROOTS) {	/* r is the step number */
        /* Compute discrepancy at the r-th step in poly-form */
        discr_r = 0;
        for (i = 0; i < r; i++) {
            if ((lambda[i] != 0) && (s[r - i - 1] != rs->nn)) {
                discr_r ^= rs->alpha_to[MODNN(rs->index_of[lambda[i]] + s[r - i - 1])];
            }
        }
        discr_r = rs->index_of[discr_r];	/* Index form */
        if (discr_r == rs->nn) {
            /* 2 lines below: B(x) <-- x*B(x) */
            memmove(&b[1], b, NROOTS * sizeof(b[0]));
            b[0] = rs->nn;
        }
        else {
            /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
            t[0] = lambda[0];
            for (i = 0; i < NROOTS; i++) {
                if (b[i] != rs->nn)
                    t[i + 1] = lambda[i + 1] ^ rs->alpha_to[MODNN(discr_r + b[i])];
                else
                    t[i + 1] = lambda[i + 1];
            }
            if (2 * el <= r + no_eras - 1) {
                el = r + no_eras - el;
                /*
                 * 2 lines below: B(x) <-- inv(discr_r) *
                 * lambda(x)
                 */
                for (i = 0; i <= NROOTS; i++)
                    b[i] = (lambda[i] == 0) ? rs->nn : MODNN(rs->index_of[lambda[i]] - discr_r + rs->nn);
            }
            else {
                /* 2 lines below: B(x) <-- x*B(x) */
                memmove(&b[1], b, NROOTS * sizeof(b[0]));
                b[0] = rs->nn;
            }
            memcpy(lambda, t, (NROOTS + 1) * sizeof(t[0]));
        }
    }

    /* Convert lambda to index form and compute deg(lambda(x)) */
    deg_lambda = 0;
    for (i = 0;i < NROOTS + 1;i++) {
        lambda[i] = rs->index_of[lambda[i]];
        if (lambda[i] != rs->nn)
            deg_lambda = i;
    }
    /* Find roots of the error+erasure locator polynomial by Chien search */
    memcpy(&reg[1], &lambda[1], NROOTS * sizeof(reg[0]));
    count = 0;		/* Number of roots of lambda(x) */
    for (i = 1, k = rs->iprim - 1; i <= rs->nn; i++, k = MODNN(k + rs->iprim)) {
        q = 1; /* lambda[0] is always 0 */
        for (j = deg_lambda; j > 0; j--) {
            if (reg[j] != rs->nn) {
                reg[j] = MODNN(reg[j] + j);
                q ^= rs->alpha_to[reg[j]];
            }
        }
        if (q != 0)
            continue; /* Not a root */
        /* store root (index-form) and error location number */

        root[count] = i;
        loc[count] = k;
        /* If we've already found max possible roots,
         * abort the search to save time
         */
        if (++count == deg_lambda)
            break;
    }
    if (deg_lambda != count) {
        /*
         * deg(lambda) unequal to number of roots => uncorrectable
         * error detected
         */
        count = -1;
        goto finish;
    }
    /*
     * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
     * x**NROOTS). in index form. Also find deg(omega).
     */
    deg_omega = deg_lambda - 1;
    for (i = 0; i <= deg_omega;i++) {
        tmp = 0;
        for (j = i;j >= 0; j--) {
            if ((s[i - j] != rs->nn) && (lambda[j] != rs->nn))
                tmp ^= rs->alpha_to[MODNN(s[i - j] + lambda[j])];
        }
        omega[i] = rs->index_of[tmp];
    }

    /*
     * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
     * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
     */
    for (j = count - 1; j >= 0; j--) {
        num1 = 0;
        for (i = deg_omega; i >= 0; i--) {
            if (omega[i] != rs->nn)
                num1 ^= rs->alpha_to[MODNN(omega[i] + i * root[j])];
        }
        num2 = rs->alpha_to[MODNN(root[j] * (rs->fcr - 1) + rs->nn)];
        den = 0;

        /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
        for (i = MIN(deg_lambda, NROOTS - 1) & ~1; i >= 0; i -= 2) {
            if (lambda[i + 1] != rs->nn)
                den ^= rs->alpha_to[MODNN(lambda[i + 1] + i * root[j])];
        }
        /* Apply error to data */
        if (num1 != 0 && loc[j] >= 0) {
            data[loc[j] - 0] ^= rs->alpha_to[MODNN(rs->index_of[num1] + rs->index_of[num2] + rs->nn - rs->index_of[den])];
        }
    }
finish:
    if (eras_pos != NULL) {
        for (i = 0;i < count;i++)
            eras_pos[i] = loc[i];
    }
    *retval = count;
}
