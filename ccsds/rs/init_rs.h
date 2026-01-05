#ifndef RS_INIT_H
#define RS_INIT_H

#include <stdlib.h>
#include "rs-common.h" // Assuming rs-common.h contains struct rs definition and modnn function

struct rs* INIT_RS(int symsize, int gfpoly, int fcr, int prim, int nroots, int pad);

#endif // RS_INIT_H
