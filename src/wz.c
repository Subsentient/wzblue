/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/

/**This file is nasty, mostly because it's designed to be compatible with Warzone 2100.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <gtk/gtk.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "wzblue.h"
#include "substrings/substrings.h"

static gboolean WZ_RecvGameStruct(int SockDescriptor, void *OutStruct);

static gboolean WZ_RecvGameStruct(int SockDescriptor, void *OutStruct)
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
	
	if (!Net_Read(SockDescriptor, SuperBuffer, sizeof SuperBuffer, FALSE)) return FALSE;
	
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
	
	return TRUE;
}

gboolean WZ_GetGamesList(const char *Server, unsigned short Port, uint32_t *GamesAvailable, GameStruct **Pointer)
{
	GameStruct *GamesList = NULL;
	static GameStruct *PrevList;
	static uint32_t PrevAvailable;
	int WZSocket = 0;
	uint32_t Inc = 0;

	
	if (!Net_Connect(Server, Port, &WZSocket))
	{
		puts("Unable to connect to lobby server!");
		return FALSE;
	}
	
	if (!Net_Write(WZSocket, "list\r\n"))
	{
		puts("Unable to write LIST command to lobby server!");
		return FALSE;
	}
	
	/*Get number of available games.*/
	if (!Net_Read(WZSocket, GamesAvailable, sizeof(uint32_t), FALSE))
	{
		puts("Unable to read data from connection to lobby server!");
		return FALSE;
	}
	
	*GamesAvailable = ntohl(*GamesAvailable);
	
	/*Allocate space for them.*/
	*Pointer = GamesList = calloc(*GamesAvailable + 1, sizeof(GameStruct));

	unsigned Usable = 0;
	for (unsigned Inc = 0; Inc < *GamesAvailable; ++Inc)
	{ /*Receive the listings.*/
		GameStruct Temp = { 0 };
		
		if (!WZ_RecvGameStruct(WZSocket, &Temp))
		{
			if (PrevList) free(PrevList);
			PrevList = NULL;
			free(GamesList);
			return FALSE;
		}

		if (Settings.HideIncompatibleGames != CHOICE_YES || SubStrings.Compare(GameVersion, Temp.VersionString) || !*GameVersion)
		{
			GamesList[Usable++] = Temp;
		}
	}

	*GamesAvailable = Usable;
	
	Net_Disconnect(WZSocket);
	
	gboolean RetVal = FALSE;
	if (PrevList)
	{
		//Compare the lists to see if the new one is identical to the old.
		if (PrevAvailable != *GamesAvailable)
		{
			RetVal = TRUE;
		}
		else
		{
			for (Inc = 0; Inc < *GamesAvailable && PrevList[Inc].GameName[0] != '\0'; ++Inc)
			{
				if (memcmp(GamesList + Inc, PrevList + Inc, sizeof(GameStruct)) != 0)
				{
					RetVal = TRUE;
					break;
				}
			}
		}
	}
	else
	{
		RetVal = TRUE;
	}
	PrevAvailable = *GamesAvailable;
	if (PrevList) free(PrevList);
	PrevList = GamesList;

	return RetVal;
	
}

