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
#include "cJSON/cJSON.h"

static gboolean GetStringMemberFromObject(const cJSON *Obj, const char *MemberName, char *Out, const size_t OutCapacity)
{
	cJSON *SubObj = cJSON_GetObjectItemCaseSensitive(Obj, MemberName);
	
	if (!cJSON_IsString(SubObj) || !SubObj->valuestring)
	{
		return FALSE;
	}
	
	SubStrings.Copy(Out, SubObj->valuestring, OutCapacity);
	
	return TRUE;
}

static gboolean GetIntegerMemberFromObject(const cJSON *Obj, const char *MemberName, int32_t *Out)
{
	cJSON *SubObj = cJSON_GetObjectItemCaseSensitive(Obj, MemberName);
	
	if (!SubObj) return FALSE;
	
	*Out = SubObj->valueint;
	
	return TRUE;
}

gboolean WZ_DecodeToGameStruct(const char *LineData, GameStruct *Out)
{
	
	cJSON *Object = cJSON_Parse(LineData);
	
	if (!Object)
	{
		cJSON_Delete(Object);
		return FALSE;
	}
	
	//Only real games objects (not the motd) will have an "id" field

	GameStruct RV = { 0 };
	
	if (!GetStringMemberFromObject(Object, "id", RV.GameID, sizeof RV.GameID))
	{
		cJSON_Delete(Object);
		return FALSE;
	}
	
	if (!GetStringMemberFromObject(Object, "v", RV.VersionString, sizeof RV.VersionString))
	{
		cJSON_Delete(Object);
		return FALSE;
	}
	
	if (!GetStringMemberFromObject(Object, "name", RV.GameName, sizeof RV.GameName))
	{
		cJSON_Delete(Object);
		return FALSE;
	}	
	
	cJSON *PlayersObject = cJSON_GetObjectItemCaseSensitive(Object, "players");
	
	
	if (!PlayersObject)
	{
		cJSON_Delete(Object);
		return FALSE;
	}
	
	//Might be an empty value;
		
	GetIntegerMemberFromObject(PlayersObject, "cur", &RV.CurPlayers); //Might not exist, we don't care
	
	if (!GetIntegerMemberFromObject(PlayersObject, "max", &RV.MaxPlayers))
	{ //We always expect MaxPlayers to be populated
		cJSON_Delete(Object);
		return FALSE;
	}
	
	cJSON *MapObject = cJSON_GetObjectItemCaseSensitive(Object, "map");
	
	if (!MapObject->child)
	{
		cJSON_Delete(Object);

		return FALSE;
	}
	
	SubStrings.Copy(RV.Map, MapObject->child->valuestring, sizeof RV.Map);
	
	cJSON *HostObject = cJSON_GetObjectItemCaseSensitive(Object, "host");
	
	if (!HostObject)
	{
		cJSON_Delete(Object);

		return FALSE;
	}
	
	if (!GetStringMemberFromObject(HostObject, "name", RV.HostNick, sizeof RV.HostNick))
	{ //No host object
		cJSON_Delete(Object);

		return FALSE;
	}
	
	memcpy(Out, &RV, sizeof RV);

	cJSON_Delete(Object);
	
	return TRUE;
}

static size_t CountNewlines(const char *String)
{
	const char *Worker = String;
	
	size_t NumNewlines = 0u;
	while ((Worker = strchr(Worker, '\n')) != NULL)
	{
		++Worker;
		++NumNewlines;
	}
	
	return NumNewlines;
}

gboolean WZ_GetGamesList(uint32_t *GamesAvailable, GameStruct **Pointer)
{

#define LOBBY_JSON_BUFFER_SIZE 65536
	puts("Entered WZ_GetGamesList()");
	GameStruct *GamesList = NULL;
	
	puts("Curling URL " LOBBY_URL);
	
	char *HTTPBuffer = calloc(LOBBY_JSON_BUFFER_SIZE, 1);
	
	if (!Curl_GetLobbyHTTP(Settings.LobbyURL, HTTPBuffer, LOBBY_JSON_BUFFER_SIZE - 1))
	{
		fprintf(stderr, "Failed to curl address \"%s\"\n", LOBBY_URL);
		free(HTTPBuffer);
		return FALSE;
	}

	//One line per game
	const size_t NumGamesCapacity = CountNewlines(HTTPBuffer);
	
	/*Allocate space for them.*/
	*Pointer = GamesList = calloc(NumGamesCapacity + 1, sizeof(GameStruct));
	
	char LineBuf[4096] = "";
	
	const char *Iter = HTTPBuffer;
	
	size_t Inc = 0u, LineNum = 0u;
	
	printf("HTTP BUFFER !!!\n%s\n", HTTPBuffer);
	
	while (SubStrings.CopyUntilC(LineBuf, sizeof LineBuf, &Iter, "\r\n", TRUE))
	{ /*Receive the listings.*/
		if (!*LineBuf) break;
		
		++LineNum;
		
		GameStruct Temp = { 0 };
		
		printf("GOT LINEBUF %s\n", LineBuf);
		
		if (!WZ_DecodeToGameStruct(LineBuf, &Temp))
		{
			fprintf(stderr, "\n\n\nERROR: Failed to decode JSON line: \"\"\"%s\"\"\"\n\n\n", LineBuf);
			memset(LineBuf, 0, sizeof LineBuf); //Wipe it

			continue;
		}
		
		printf("Got game name \"%s\" at LineNum %u\n", Temp.GameName, (unsigned)LineNum);
		
		printf("Got game JSON:\n%s\n", LineBuf);
		memset(LineBuf, 0, sizeof LineBuf); //Wipe it

		if (Settings.HideIncompatibleGames == CHOICE_YES && !SubStrings.Compare(GameVersion, Temp.VersionString))
		{
			continue;
		}

		if (Settings.HideEmptyGames == CHOICE_YES && !Temp.CurPlayers)
		{
			continue;
		}
		memset(LineBuf, 0, sizeof LineBuf); //Wipe it

		GamesList[Inc++] = Temp;
		
	}

	*GamesAvailable = Inc;
	
	printf("Returning with GamesAvailable %u", (unsigned)Inc);
	free(HTTPBuffer);
	
	return TRUE;
}

