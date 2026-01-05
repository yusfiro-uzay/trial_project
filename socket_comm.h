/*
 * socket_comm.h
 *
 *  Created on: 11 Aðu 2020
 *      Author:
 */

#ifndef SOCKET_COMM_H_
#define SOCKET_COMM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "unistd_win.h"
#include "common_types.h"


#define MAX_TC_FRAME_SIZE						2558
#define MAX_CLTU_SIZE							3000

#define TM_FRAME_SIZE						    2558

void setupTcFrameComm(char*, uint16);

#ifdef __cplusplus
extern "C" {
#endif

void setupCADUComm(const char* Fsw_ip, uint16 tmFramePort_fromFsw);
void setupLockServerComm(const char* LockServerIp, uint16 LockStatusPort);
int8 receiveCADU(unsigned char*, int*);
int8 sendTmFrame(uint8*, int32);
int8 sendGainValue(float gainValue);
int8 sendLockStatus(uint8 outBuffer, int32 msgSize);
void setupTmFrameComm(const char*, uint16);
void setupSdrComm(const char*, uint16);


#ifdef __cplusplus
}
#endif
int8 receiveTcFrame(uint8*, int32*);
void printBuffer(uint8*, uint16, uint8);

int8 sendCLTU(uint8*, int32);
void setupCLTUComm(char*, uint16);

#endif /* SOCKET_COMM_H_ */
