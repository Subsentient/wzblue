/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "wzblue.h"
#ifdef WIN
#include <windows.h>
#endif
int main(int argc, char **argv)
{
	int Inc = 1;
	char Server[512] = "lobby.wz2100.net";
	int Refresh = 7;
	int Port = 9990;
	time_t Clock = 0;
	struct tm *TimeStruct = NULL;
	char TimeBuf[256];
	Bool WZLegacy = false;
#ifdef WIN /*Fire up winsock2.*/
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    { /*Initialize winsock*/
        fprintf(stderr, "Unable to initialize WinSock2!");
        exit(1);
	}
#endif

	if (argc > 1)
	{
		for (; Inc < argc; ++Inc)
		{
			if (!strncmp(argv[Inc], "--server=", sizeof "--server=" - 1))
			{
				strncpy(Server, argv[Inc] + (sizeof "--server=" - 1), sizeof Server - 1);
				Server[sizeof Server - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--refresh=", sizeof "--refresh=" - 1))
			{
				Refresh = atoi(argv[Inc] + sizeof "--refresh=" - 1);
				
				if (Refresh < 3)
				{
					fprintf(stderr, "Minimum refresh time is 3 seconds.\nPlease specify a value greater or equal to 3.\n");
					exit(1);
				}
				
			}
			else if (!strncmp(argv[Inc], "--port=", sizeof "--port=" - 1))
			{
				Port = atoi(argv[Inc] + (sizeof "--port=" - 1));
				if (!Port)
				{
					fprintf(stderr, "You specified port 0 (zero). That's not valid. Default is 9990.\n");
					exit(1);
				}
			}
			else if (!strcmp(argv[Inc], "--wzlegacy"))
			{
				puts("Treating server as Warzone 2100 Legacy lobby server.");
				WZLegacy = true;
			}
			else
			{
				fprintf(stderr, "Unknown command switch.\n");
				exit(1);
			}
		}
	}
	
	while (1)
	{
		
	/*lazy way to clear the screen.*/
#ifdef WIN
		system("cls");
#else
		system("clear");
#endif
		Clock = time(NULL);
		
		TimeStruct = localtime(&Clock);
		strftime(TimeBuf, sizeof TimeBuf, "%Y-%m-%d %I:%M:%S %p", TimeStruct);
		
		printf("Last update: %s\n", TimeBuf);
		
		WZ_GetGamesList(Server, Port, WZLegacy);
		
#ifdef WIN
		Sleep(Refresh * 1000);
#else
		usleep(Refresh * 1000000);
#endif
	}
	return 0;
}
