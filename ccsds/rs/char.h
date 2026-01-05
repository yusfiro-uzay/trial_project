/* Stuff specific to the 8-bit symbol version of the general purpose RS codecs
 *
 * Copyright 2003, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */
#include "rs-common.h"


#define MODNN(x) modnn(rs,x)

#define MM (rs->mm)
#define NN (rs->nn)
#define ALPHA_TO (rs->alpha_to) 
#define INDEX_OF (rs->index_of)
#define GENPOLY 0x15F
#define NROOTS 32
#define FCR (rs->fcr)
#define PRIM (rs->prim)
#define IPRIM (rs->iprim)
#define PAD 0
#define A0 (NN)




