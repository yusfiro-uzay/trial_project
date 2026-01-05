/*
 * utils.h
 *
 *  Created on: 12 A�u 2020
 *      Author: fatihileri
 */


#include "getopt.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "common_types.h"

#define MAX_IP_STRING_LENGTH	16
#define MAKE_BIG16(n)     		((((n) << 8) & 0xFF00) | (((n) >> 8) & 0x00FF) )
#define MAKE_BIG32(n)     		((((n) << 24) & 0xFF000000) | (((n) << 8) & 0x00FF0000) | (((n) >> 8) & 0x0000FF00) | (((n) >> 24) & 0x000000FF))

int getOptions(int argc, char* argv[], char*, uint16*, char*, int32*, int32*, int32*, uint16*, uint16*, uint16*);
int32 generateCheckSum(uint8*, uint16);
int32 readBigEnd32(uint8*);
void writeBigEnd32(uint8*, int32);
