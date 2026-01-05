#ifndef DECODE_RS_H
#define DECODE_RS_H

#include <string.h> // For memset, memmove, memcpy
#include <stdio.h>  // For printf (if DEBUG is enabled)
#include "char.h"

// Function declaration
void reed_solomon_decode(struct rs* rs, data_t* data, int* eras_pos, int no_eras, int* retval);

#endif