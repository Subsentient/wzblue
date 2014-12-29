/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/

/**This file is nasty, mostly because it's designed to be compatible with Warzone 2100.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef WIN
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "wzblue.h"

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

static Bool WZ_RecvGameStruct(int SockDescriptor, void *OutStruct);

static Bool WZ_RecvGameStruct(int SockDescriptor, void *OutStruct)
{
	uint32_t Inc = 0;
	GameStruct RV = { 0 };
	unsigned char SuperBuffer[sizeof RV.StructVer +
							sizeof RV.GameName +
							sizeof(int32_t) * 8 +
							sizeof RV.NetSpecs.HostIP + 
							sizeof RV.SecondaryHosts + 
							sizeof RV.Extra + 
							sizeof RV.Map + 
							sizeof RV.HostNick + 
							sizeof RV.VersionString + 
							sizeof RV.ModList + 
							sizeof(uint32_t) * 9] = { 0 }, *Worker = SuperBuffer;
	
	if (!Net_Read(SockDescriptor, SuperBuffer, sizeof SuperBuffer, false)) return false;
	
	memcpy(&RV.StructVer, Worker, sizeof RV.StructVer);
	RV.StructVer = ntohl(RV.StructVer);
	Worker += sizeof RV.StructVer;
	
	memcpy(RV.GameName, Worker, sizeof RV.GameName);
	Worker += sizeof RV.GameName;
	
	memcpy(&RV.NetSpecs.Size, Worker, sizeof RV.NetSpecs.Size);
	RV.NetSpecs.Size = ntohl(RV.NetSpecs.Size);
	Worker += sizeof RV.NetSpecs.Size;
	
	memcpy(&RV.NetSpecs.Flags, Worker, sizeof RV.NetSpecs.Flags);
	RV.NetSpecs.Flags = ntohl(RV.NetSpecs.Flags);
	Worker += sizeof RV.NetSpecs.Flags;
	
	memcpy(RV.NetSpecs.HostIP, Worker, sizeof RV.NetSpecs.HostIP);
	Worker += sizeof RV.NetSpecs.HostIP;
	
	memcpy(&RV.NetSpecs.MaxPlayers, Worker, sizeof(uint32_t));
	RV.NetSpecs.MaxPlayers = ntohl(RV.NetSpecs.MaxPlayers);
	Worker += sizeof(uint32_t);
	
	memcpy(&RV.NetSpecs.CurPlayers, Worker, sizeof(uint32_t));
	RV.NetSpecs.CurPlayers = ntohl(RV.NetSpecs.CurPlayers);
	Worker += sizeof(uint32_t);
	
	memcpy(RV.NetSpecs.UserFlags, Worker, sizeof(uint32_t) * 4);
	for (Inc = 0; Inc < 4; ++Inc)
	{
		RV.NetSpecs.UserFlags[Inc] = ntohl(RV.NetSpecs.UserFlags[Inc]);
	}
	Worker += sizeof(uint32_t) * 4;
	
	memcpy(RV.SecondaryHosts, Worker, sizeof RV.SecondaryHosts);
	Worker += sizeof RV.SecondaryHosts;
	/*Extra Map Host*/
	memcpy(RV.Extra, Worker, sizeof RV.Extra);
	Worker += sizeof RV.Extra;
	
	memcpy(RV.Map, Worker, sizeof RV.Map);
	Worker += sizeof RV.Map;
	
	memcpy(RV.HostNick, Worker, sizeof RV.HostNick);
	Worker += sizeof RV.HostNick;
	
	memcpy(RV.VersionString, Worker, sizeof RV.VersionString);
	Worker += sizeof RV.VersionString;
	
	memcpy(RV.ModList, Worker, sizeof RV.ModList);
	Worker += sizeof RV.ModList;
	
	/*Fuck alignment. This is aligned and I don't wanna hear anything else.*/
	memcpy(&RV.MajorVer, Worker, sizeof(uint32_t) * 9);
	for (Inc = 0; Inc < 9; ++Inc)
	{
		(&RV.MajorVer)[Inc] = ntohl((&RV.MajorVer)[Inc]);
	}
	Worker += sizeof(uint32_t) * 9;
	
	memcpy(OutStruct, &RV, sizeof RV);
	
	return true;
}

Bool WZ_GetGamesList(const char *Server, unsigned short Port, Bool WZLegacy)
{
	GameStruct *GamesList = NULL;
	int WZSocket = 0;
	char OutBuf[2048];
	uint32_t GamesAvailable = 0, Inc = 0;
	uint32_t LastHosted = 0;
	ConsoleColor LabelColor = ENDCOLOR;
	char MOTD[512] = { '\0' };
	
	if (!Net_Connect(Server, Port, &WZSocket))
	{
		puts("Unable to connect to lobby server!");
		return false;
	}
	
	if (!Net_Write(WZSocket, "list\r\n"))
	{
		puts("Unable to write LIST command to lobby server!");
		return false;
	}
	
	/*Get number of available games.*/
	if (!Net_Read(WZSocket, &GamesAvailable, sizeof(uint32_t), false))
	{
		puts("Unable to read data from connection to lobby server!");
		return false;
	}
	
	GamesAvailable = ntohl(GamesAvailable);
	
	/*Allocate space for them.*/
	GamesList = malloc(sizeof(GameStruct) * GamesAvailable);

	for (; Inc < GamesAvailable; ++Inc)
	{ /*Receive the listings.*/
		if (!WZ_RecvGameStruct(WZSocket, GamesList + Inc))
		{
			free(GamesList);
			return false;
		}
	}
	
	/*If we're Legacy protocol, retrieve the time since last hosted.*/
	if (WZLegacy)
	{
		if (!Net_Read(WZSocket, &LastHosted, sizeof(uint32_t), false))
		{
			free(GamesList);
			return false;
		}
		
		LastHosted = ntohl(LastHosted);
	}
	else
	{ /*3.1 protocol.*/
		uint32_t CodeBuf[2];
		
		if (!Net_Read(WZSocket, CodeBuf, sizeof(uint32_t) * 2, false) ||
			!Net_Read(WZSocket, MOTD, CodeBuf[1], false))
		{
			free(GamesList);
			return false;
		}
	}
		
	
	Net_Disconnect(WZSocket);
	
	if (WZLegacy)
	{ /*Warzone 2100 Legacy-style lobby server.*/
		if (LastHosted > 60)
		{ /*Show in minutes.*/
			printf("%u game%s available.\nLast game hosted %d minute%s ago.\n", (unsigned)GamesAvailable,
					GamesAvailable == 1 ? "" : "s", LastHosted / 60, LastHosted / 60 == 1 ? "" : "s");
		}
		else
		{ /*Show in seconds.*/
			printf("%u game%s available.\nLast game hosted %d second%s ago.\n", (unsigned)GamesAvailable,
					GamesAvailable == 1 ? "" : "s", LastHosted, LastHosted == 1 ? "" : "s");
		}
	}
	else
	{ /*Standard Warzone protocol.*/
		printf("%u game%s available.\nMOTD: ", (unsigned)GamesAvailable, GamesAvailable == 1 ? "" : "s");
		WZBlue_SetTextColor(CYAN);
		printf("\"%s\"\n", MOTD);
		WZBlue_SetTextColor(ENDCOLOR);
	}
	fflush(NULL);
	
	/*Now send them to the user.*/
	for (Inc = 0; Inc < GamesAvailable; ++Inc)
	{
		char ModString[384] = { '\0' };
		const Bool MapMod = !WZLegacy && GamesList[Inc].MapMod;
		
		if (GamesList[Inc].NetSpecs.CurPlayers >= GamesList[Inc].NetSpecs.MaxPlayers)
		{ /*Game is full.*/
			LabelColor = RED;
		}
		else if (GamesList[Inc].PrivateGame)
		{ /*Private game.*/
			LabelColor = YELLOW;
		}
		else if (*GamesList[Inc].ModList != '\0')
		{ /*It has mods.*/
			LabelColor = MAGENTA;
		}
		else
		{ /*Normal, joinable game.*/
			LabelColor = GREEN;
		}
		
		/*Add mod warning even if we don't get the color for that.*/
		if (GamesList[Inc].ModList[0] != '\0')
		{
			snprintf(ModString, sizeof ModString, " (mods: %s)",  GamesList[Inc].ModList);
		}
		
		WZBlue_SetTextColor(LabelColor); /*Set the game color.*/
		printf("\n[%d]", Inc + 1); /*Print our colored label.*/
		WZBlue_SetTextColor(ENDCOLOR); /*Turn off colors.*/
		
		
		snprintf(OutBuf, sizeof OutBuf, " Name: %s | Map: %s%s | Host: %s\n"
				"Players: %d/%d %s| IP: %s | Version: %s%s",
				GamesList[Inc].GameName, GamesList[Inc].Map, MapMod ? " (map-mod)" : "",
				GamesList[Inc].HostNick, GamesList[Inc].NetSpecs.CurPlayers, GamesList[Inc].NetSpecs.MaxPlayers,
				GamesList[Inc].PrivateGame ? "(private) " : "", GamesList[Inc].NetSpecs.HostIP,
				GamesList[Inc].VersionString, ModString);
				
		puts(OutBuf);
		
	}
	
	if (GamesList) free(GamesList);
		
	
	return true;
	
}