/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef WIN
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "wzblue.h"

Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_)
{

	char *FailMsg = "Failed to establish a connection to the server:";
	struct sockaddr_in SocketStruct;
	struct hostent *HostnameStruct;
	
	memset(&SocketStruct, 0, sizeof(SocketStruct));
	
	if ((HostnameStruct = gethostbyname(InHost)) == NULL)
	{
		
		fprintf(stderr, "Failed to resolve hostname \"%s\".\n", InHost);
		
		return 0;
	}
	
	if ((*SocketDescriptor_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror(FailMsg);
		return false;
	}
	
	memcpy(&SocketStruct.sin_addr, HostnameStruct->h_addr_list[0], HostnameStruct->h_length);
	
	SocketStruct.sin_family = AF_INET;
	SocketStruct.sin_port = htons(PortNum);
	
	if (connect(*SocketDescriptor_, (void*)&SocketStruct, sizeof SocketStruct) != 0)
	{
		
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
		*SocketDescriptor_ = 0;
		return false;
	}

	return true;
}

Bool Net_Write(int SockDescriptor, const char *InMsg)
{
	size_t StringSize = strlen(InMsg);
	unsigned Transferred = 0, TotalTransferred = 0;

	do
	{
		Transferred = send(SockDescriptor, InMsg + TotalTransferred, (StringSize - TotalTransferred), 0);
		
		if (Transferred == -1) /*This is ugly I know, but it's converted implicitly, so shut up.*/
		{
			return false;
		}
		
		TotalTransferred += Transferred;
	} while (StringSize > TotalTransferred);
	
	
	return true;
}

Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, Bool TextStream)
{
	int Status = 0;
	unsigned char Byte = 0;
	unsigned char *OutStream = OutStream_;
	unsigned Inc = 0;
	
	*OutStream = '\0';
	
	do
	{
		Status = recv(SockDescriptor, (void*)&Byte, 1, 0);
		if (TextStream && Byte == '\n') break;
		
		*OutStream++ = Byte;
	} while (++Inc, Status > 0 && Inc < (TextStream ? MaxLength - 1 : MaxLength));
	
	if (TextStream)
	{
		*OutStream = '\0';
		if (*(OutStream - 1) == '\r') *(OutStream - 1) = '\0';
	}
	
	if (Status == -1) return false;
	
	return true;
}

Bool Net_Disconnect(int SockDescriptor)
{
	if (!SockDescriptor) return false;
	
#ifdef WIN
	return !closesocket(SockDescriptor);
#else
	return !close(SockDescriptor);
#endif
}
