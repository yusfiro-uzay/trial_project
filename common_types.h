/*
 * common_types.h
 *
 *  Created on: 26 Þub 2018
 *      Author: fatihileri
 */

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <stdio.h>

typedef unsigned long long uint64;
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;

typedef long long int64;
typedef int   int32;
typedef short int16;
typedef char  int8;

#define TRUE 	1
#define FALSE 	0

#define C_GREEN       "\033[0m\x1B[1;32m"
#define C_YELLOW      "\033[0m\x1B[1;33m"
#define C_RED         "\033[0m\x1B[1;31m"
#define C_RESET       "\033[0m\x1B[1;37m"

#ifdef _MSC_VER
#define LOG_ON_CONSOLE(tmp, color, fmt, ...) do { \
        printf(fmt); \
        fflush(0); \
    } while (0)
#else
#define LOG_ON_CONSOLE(tmp, color, fmt, args...) do { \
        printf(color fmt C_RESET, ## args); \
        fflush(0); \
    } while (0)
#endif

#endif /* COMMON_TYPES_H_ */
