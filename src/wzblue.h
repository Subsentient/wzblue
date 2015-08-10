/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/

#ifndef __WZBLUE_H__
#define __WZBLUE_H__

#define WZBLUE_VERSION "0.4"

typedef signed char Bool;
enum { false, true };
typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, ENDCOLOR = 0 } ConsoleColor;

#include <stdint.h>
typedef struct
{
	uint32_t StructVer;
	char GameName[64];
	
	struct
	{
		int32_t Size;
		int32_t Flags;
		char HostIP[40];
		int32_t MaxPlayers;
		int32_t CurPlayers;
		int32_t UserFlags[4];
	} NetSpecs;
	
	char SecondaryHosts[2][40];
	char Extra[159];
	char Map[40];
	char HostNick[40];
	char VersionString[64];
	char ModList[255];
	uint32_t MajorVer, MinorVer;
	uint32_t PrivateGame;
	uint32_t MapMod; /*PureGame in 2.3/Legacy, PureMap in 3.1.*/
	uint32_t Mods;
	
	uint32_t GameID;
	
	uint32_t Unused1;
	uint32_t Unused2;
	uint32_t Unused3;
} GameStruct;

/*main.c*/
void WZBlue_SetTextColor(ConsoleColor Color);

/*netcore.c*/
Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
Bool Net_Disconnect(int SockDescriptor);
Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, Bool TextStream);
Bool Net_Disconnect(int SockDescriptor);
Bool Net_Write(int SockDescriptor, const char *InMsg);

/*wz.c*/
Bool WZ_GetGamesList(const char *Server, unsigned short Port, uint32_t *GamesAvailable, GameStruct **Pointer);
void WZ_SendGamesList(const GameStruct *GamesList, uint32_t GamesAvailable);

#endif //__WZBLUE_H__
