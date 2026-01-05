#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#ifdef _WIN32
#include "getopt.h" // Include custom getopt implementation for Windows
#else
#include <getopt.h>
#endif

#define MAX_IP_STRING_LENGTH 64
#define MAKE_BIG32(x) htonl(x) // Ensure compatibility with big-endian systems

const char* short_opt = "i:p:h:m:s:c:r:k:d";

struct option long_opt[] = {
    {"router-prox-ip", required_argument, NULL, 'i'},
    {"tc-frm-port", required_argument, NULL, 'p'},
    {"grtmtc-ip", required_argument, NULL, 'h'},
    {"tc-psr-enable", required_argument, NULL, 'm'},
    {"cadu-size", required_argument, NULL, 's'},
    {"cadu-rs-byte-count", required_argument, NULL, 'c'},
    {"cadu-port", required_argument, NULL, 'r'},
    {"tm-frm-port", required_argument, NULL, 'k'},
    {"cltu-port", required_argument, NULL, 'd'},
    {NULL, 0, NULL, 0}
};

int getOptions(int argc, char* argv[], char* routerProxIp, uint16* tcFramePort, char* grtmtcIp,
    int32* tcPsrEnable, int32* caduSize, int32* caduRsByteCount, uint16* caduPort,
    uint16* tmFrmPort, uint16* cltuPort) {
    int c;
    int optCnt = 0;
    int retVal = 0;

    while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
        optCnt++;
        switch (c) {
        case 'i':
            strncpy_s(routerProxIp, optarg, MAX_IP_STRING_LENGTH - 1, _TRUNCATE);
            routerProxIp[MAX_IP_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
            break;
        case 'p':
            *tcFramePort = (uint16)atoi(optarg);
            break;
        case 'h':
            strncpy_s(grtmtcIp, optarg, MAX_IP_STRING_LENGTH - 1, _TRUNCATE);
            grtmtcIp[MAX_IP_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
            break;
        case 'm':
            *tcPsrEnable = atoi(optarg);
            break;
        case 's':
            *caduSize = atoi(optarg);
            break;
        case 'c':
            *caduRsByteCount = atoi(optarg);
            break;
        case 'r':
            *caduPort = (uint16)atoi(optarg);
            break;
        case 'k':
            *tmFrmPort = (uint16)atoi(optarg);
            break;
        case 'd':
            *cltuPort = (uint16)atoi(optarg);
            break;
        default:
            fprintf(stderr, "Invalid opasdasd: %c\n", c);
            retVal = -1;
            break;
        }
    }

    return retVal;
}

int32 generateCheckSum(uint8* message, uint16 messageSize) {
    uint16 n = 0;
    uint8* ptr = message;
    int32 checkSum = 0;

    for (n = 0; n < messageSize; n += sizeof(int32)) {
        checkSum += readBigEnd32(ptr);
        ptr += sizeof(int32);
    }

    checkSum = -checkSum;
    return checkSum;
}

int32 readBigEnd32(uint8* source) {
    int32 dest;
    memcpy(&dest, source, sizeof(int32));
    return MAKE_BIG32(dest);
}

void writeBigEnd32(uint8* destination, int32 value) {
    value = MAKE_BIG32(value);
    memcpy(destination, &value, sizeof(int32));
}
