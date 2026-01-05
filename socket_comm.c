/*
 * socket_comm.c
 *
 *  Created on: 11 Agu 2020
 *      Author:
 */

#include "socket_comm.h"

SOCKET socket_tcFrame_desc_fromRouterProxy;
SOCKADDR_IN source_routerProxy_UDP_tcFrame;

SOCKET socket_tmFrame_desc_fromFsw;
SOCKADDR_IN source_Fsw_UDP_tmFrame;

SOCKET socket_tmFrame_desc_toModemProxy;
SOCKADDR_IN dest_modemProxy_UDP_tmFrame;

SOCKET socket_sdr_gain;
SOCKADDR_IN dest_sdr_gain;

SOCKET socket_tmFrame_desc_toLockServer;
SOCKADDR_IN dest_modemProxy_UDP_LockServer;

SOCKET socket_CLTU_desc_SDR;
SOCKADDR_IN dest_SDR_TCP_CLTU;

SOCKET socket_status_desc_LockServer;
SOCKADDR_IN dest_lock_status_desc_LockServer;
/*
uint8 createUdpSender(int8* desc, char* ipAddress, struct sockaddr_in* destination, uint16 port)
{
	*desc = socket(AF_INET, SOCK_DGRAM, 0);

	if ((*desc) < 0)
	{
		return FALSE;
	}

	bzero(destination, sizeof(struct sockaddr_in));
	destination->sin_family = AF_INET;
	destination->sin_addr.s_addr = inet_addr(ipAddress);
	destination->sin_port = htons(port);
	memset(destination->sin_zero, '\0', sizeof(destination->sin_zero));

	return TRUE;
}

uint8 createUdpReceiver(int8* desc, struct sockaddr_in* source, char* ipAddress, uint16 port)
{
	*desc = socket(AF_INET, SOCK_DGRAM, 0);

	if ((*desc) < 0)
	{
		return FALSE;
	}

	bzero(source, sizeof(struct sockaddr_in));
	source->sin_family = AF_INET;
	source->sin_addr.s_addr = inet_addr(ipAddress);
	source->sin_port = htons(port);
	memset(source->sin_zero, '\0', sizeof(source->sin_zero));

	if (bind(*desc, (struct sockaddr *)source, sizeof(struct sockaddr_in)) != 0)
	{
		return FALSE;
	}

	return TRUE;
}
*/


#ifdef _WIN32
void initializeWinSock() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(EXIT_FAILURE);
	}
}

void cleanupWinSock() {
	WSACleanup();
}
#endif

uint8 createUdpSender(SOCKET* desc, char* ipAddress, SOCKADDR_IN *destination, uint16 port) {
	*desc = socket(AF_INET, SOCK_DGRAM, 0);

	if (*desc < 0) {
#ifdef _WIN32
		fprintf(stderr, "Socket creation failed with error: %d\n", WSAGetLastError());
#else
		perror("Socket creation failed");
#endif
		return FALSE;
	}

	memset(destination, 0, sizeof(struct sockaddr_in));
	destination->sin_family = AF_INET;

	if (inet_pton(AF_INET, ipAddress, &destination->sin_addr) <= 0) {
#ifdef _WIN32
		fprintf(stderr, "Invalid address: %s\n", ipAddress);
#else
		perror("Invalid address");
#endif
		return FALSE;
	}

	destination->sin_port = htons(port);
	return TRUE;
}
uint8 createUdpReceiver(SOCKET *desc, SOCKADDR_IN *source, char *ipAddress, uint16 port) {
	
	// 1. Create a UDP socket
	*desc = socket(AF_INET, SOCK_DGRAM, 0);
	if (*desc == INVALID_SOCKET) {
		fprintf(stderr, "Socket creation failed with error: %d\n", WSAGetLastError());
		return FALSE;
	}
	printf("1");

	// 2. Set socket options (SO_REUSEADDR)
	int opt = 1;
	if (setsockopt(*desc, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
		fprintf(stderr, "setsockopt failed with error: %d\n", WSAGetLastError());
		closesocket(*desc); // Close socket on error
		return FALSE;
	}
	printf("1");
	// 3. Initialize the source structure
	memset(source, 0, sizeof(struct sockaddr_in));
	source->sin_family = AF_INET;
	source->sin_port = htons(port);
	printf("1");
	printf("dadsasa\n");
	printf("inet_pton(AF_INET, ipAddress, &source->sin_addr) : %d\n",inet_pton(AF_INET, ipAddress, &source->sin_addr));
	// 4. Convert IP address from text to binary form
	if (inet_pton(AF_INET, ipAddress, &source->sin_addr) <= 0) {
		fprintf(stderr, "Invalid IP address: %s\n", ipAddress);
		closesocket(*desc); // Close socket on error
		return FALSE;
	}
	printf("1");
	// 5. Bind the socket to the specified address and port
	if (bind(*desc, (struct sockaddr*)source, sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "Bind failed with error: %d\n", WSAGetLastError());
		closesocket(*desc); // Close socket on error
		return FALSE;
	}
	printf("1");
	// 6. Success
	printf("Socket created and bound successfully.\n");
	return TRUE;
}



//SDR dan CLTU gondermek icin.
void setupCLTUComm(char* SDR_Hw_ip, uint16 SDR_Hw_tcPort)
{
	uint8 result;

	do
	{
		result = createUdpSender(&socket_CLTU_desc_SDR, SDR_Hw_ip, &dest_SDR_TCP_CLTU, SDR_Hw_tcPort);

		if (result == TRUE)
		{
			LOG_ON_CONSOLE(0, C_GREEN, "Connection established for CLTU.\n");
		}
		else
		{
			LOG_ON_CONSOLE(0, C_YELLOW, "Attempting to establish UDP connection for CLTU.\n");
			sleep(3);
		}
	} while (!result);
}

/*

SOCKET socket_status_desc_LockServer;
SOCKADDR_IN dest_lock_status_desc_LockServer;
*/
// SDR dan CLTU gondermek icin.
void setupLockServerComm(const char* LockServerIp, uint16 LockStatusPort) {
  uint8 result;

  do {
    result = createUdpSender(&socket_status_desc_LockServer, LockServerIp,
                             &dest_lock_status_desc_LockServer, LockStatusPort);

    if (result == TRUE) {
      LOG_ON_CONSOLE(0, C_GREEN, "Connection established for lock server.\n");
    } else {
      LOG_ON_CONSOLE(0, C_YELLOW,
                     "Attempting to establish UDP connection for lock server.\n");
      sleep(3);
    }
  } while (!result);
}

//FSW'den gelen TM frame'leri dinlemek icin.
void setupCADUComm(const char* Fsw_ip, uint16 tmFramePort_fromFsw)
{
	uint8 result;

	do
	{
		result = createUdpReceiver(&socket_tmFrame_desc_fromFsw, &source_Fsw_UDP_tmFrame, Fsw_ip, tmFramePort_fromFsw);

		if (!result)
		{
			LOG_ON_CONSOLE(0, C_YELLOW, "Attempting to create UDP socket for collecting CADU.\n");
			sleep(1);
		}
		else
		{
			LOG_ON_CONSOLE(0, C_GREEN, "UDP socket created for collecting CADU\n");
		}
	} while (!result);
}

//Modem Proxy'ye, Fsw'den gelen TM Frame'leri iletmek icin.
void setupTmFrameComm(char* ipAddr_modemProxy, uint16 udpPort_tmFrameToModemProxy)
{
	uint8 result;

	do
	{
		result = createUdpSender(&socket_tmFrame_desc_toModemProxy, (char*)ipAddr_modemProxy, &dest_modemProxy_UDP_tmFrame, udpPort_tmFrameToModemProxy);

		if (!result)
		{
			LOG_ON_CONSOLE(0, C_YELLOW, "Attempting to create UDP socket for TM Frame.\n");
			sleep(1);
		}
		else
		{
			LOG_ON_CONSOLE(0, C_GREEN, "UDP socket created for TM Frame.\n");
		}
	} while (!result);
}
void setupSdrComm(char* ipAddr_modemProxy,
                      uint16 udpPort_tmFrameToModemProxy) {
  uint8 result;

  do {
    result = createUdpSender(
        &socket_sdr_gain, (char*)ipAddr_modemProxy,
        &dest_sdr_gain, udpPort_tmFrameToModemProxy);

    if (!result) {
      LOG_ON_CONSOLE(0, C_YELLOW,
                     "Attempting to create UDP socket for Gain.\n");
      sleep(1);
    } else {
      LOG_ON_CONSOLE(0, C_GREEN, "UDP socket created for Gain.\n");
    }
  } while (!result);
}

//Router Proxy'den gelen TC frame'leri dinlemek icin.
void setupTcFrameComm(char* routerProxy_ip, uint16 tcFramePort_fromRouterProxy)
{
	uint8 result;

	do
	
	{
		result = createUdpReceiver(&socket_tcFrame_desc_fromRouterProxy, &source_routerProxy_UDP_tcFrame, routerProxy_ip, tcFramePort_fromRouterProxy);

		if (!result)
		{
			LOG_ON_CONSOLE(0, C_YELLOW, "Attempting to create UDP socket for collecting TC frames.\n");
			sleep(1);
		}
		else
		{
			LOG_ON_CONSOLE(0, C_GREEN, "UDP socket created for collecting TC frames.\n");
		}
	} while (!result);
}


int8 receiveTcFrame(uint8* inBuffer, int32* frameSize)
{
	*frameSize = recvfrom(socket_tcFrame_desc_fromRouterProxy, inBuffer, MAX_TC_FRAME_SIZE, 0, NULL, NULL);

	printf("\nTC frame received. Size: %d\n", (*frameSize));

	if ((*frameSize) == -1)
	{
    	LOG_ON_CONSOLE(0, C_RED, "Either connection closed or error: TC frame flow out.\n");
        return FALSE;
	}

	return TRUE;
}

int8 sendTmFrame(uint8* outBuffer, int32 msgSize)
{
	int32 retVal;
	
	retVal = sendto(socket_tmFrame_desc_toModemProxy, outBuffer, msgSize, 0,  (struct sockaddr *)&dest_modemProxy_UDP_tmFrame, sizeof(struct sockaddr_in));

	if (retVal == -1)
	{
		LOG_ON_CONSOLE(0, C_RED, "Failure: TM Frame could not be sent.\n");

	}
	return retVal;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Windows'ta: #include <winsock2.h> ve #pragma comment(lib, "Ws2_32.lib")

int8 sendGainValue(float gainValue) {
  int32 retVal;

  // Float'ı stringe çevir (ör: "25.000000")
  char outBuffer[32];
  int msgSize = snprintf(outBuffer, sizeof(outBuffer), "%.6f", gainValue);
  if (msgSize < 0) {
    fprintf(stderr, "Gain değeri stringe çevrilemedi.\n");
    return -1;
  }

  retVal = sendto(socket_sdr_gain, outBuffer, msgSize, 0,
                  (struct sockaddr*)&dest_sdr_gain, sizeof(struct sockaddr_in));

  if (retVal == -1) {
    LOG_ON_CONSOLE(0, C_RED, "Failure: Gain Value could not be sent.\n");
  } else {
    LOG_ON_CONSOLE(1, C_GREEN, "Gain value sent: %s\n", outBuffer);
  }

  return retVal;
}


/*

SOCKET socket_status_desc_LockServer;
SOCKADDR_IN dest_lock_status_desc_LockServer;
*/
int8 sendLockStatus(uint8 outBuffer, int32 msgSize) {
  int32 retVal;

  retVal = sendto(socket_status_desc_LockServer, &outBuffer, msgSize, 0,
                  (struct sockaddr*)&dest_lock_status_desc_LockServer,
                  sizeof(struct sockaddr_in));

  if (retVal == -1) {
    LOG_ON_CONSOLE(0, C_RED,"Failure: Lock Status could not be sent.\n");
  }
  return retVal;
}

int8 receiveCADU(unsigned char* inBuffer, int* frameSize)
{
	SOCKADDR clienAddr;
	int clientAddrSize = sizeof(clienAddr);
	

	do {
		*frameSize = recvfrom(socket_tmFrame_desc_fromFsw, inBuffer, 40928/*TODO:check*/, 0, &clienAddr, &clientAddrSize);

		//printf("frameSize : %d \n", *frameSize);
		if ((*frameSize) == -1)
		{
			LOG_ON_CONSOLE(0, C_RED, "Either connection closed or error: CADU flow out.\n");
			return FALSE;
		}
		else {
			return TRUE;
		}
		
	} while (1);
	return TRUE;
}


int8 sendCLTU(uint8* outBuffer, int32 msgSize)
{
	int8 retVal = sendto(socket_CLTU_desc_SDR, outBuffer, msgSize, 0,  (struct sockaddr *)&dest_SDR_TCP_CLTU, sizeof(struct sockaddr_in));

    if (retVal == -1)
    {
    	LOG_ON_CONSOLE(0, C_RED, "Failure: CLTU could not be sent.\n");
    }
    else{
		
		LOG_ON_CONSOLE(0, C_GREEN, "CLTU sent.\n");

		
		}

    return retVal;
}

void printBuffer(uint8* startAddress, uint16 dataLength, uint8 startFromHighByte)
{
	uint16 i;
	uint8* ptr = startAddress;
	if (startFromHighByte)
	{
		ptr += (dataLength - 1);
	}

	for (i = 0; i < dataLength; i++)
	{
	    if((i%32) == 0 && (i != 0))
		    LOG_ON_CONSOLE(0, C_RESET, "\n");
		    
		LOG_ON_CONSOLE(0, C_RESET, "%02X ", *ptr);
		if (startFromHighByte)
		{
			ptr--;
		}
		else
		{
			ptr++;
		}	

	}
	LOG_ON_CONSOLE(0, C_RESET, "\n");
}

