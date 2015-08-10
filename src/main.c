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

static unsigned RefreshRate = 15; /*15 secs lobby refresh delay.*/

int main(int argc, char **argv)
{
	int Inc = 1;
	char Server[512] = "lobby.wz2100.net";
	int Port = 9990;
	time_t Clock = 0;
	struct tm *TimeStruct = NULL;
	char TimeBuf[256];
	Bool WZLegacy = false;
	Bool EnterKey = false;
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
			else if (!strncmp(argv[Inc], "--port=", sizeof "--port=" - 1))
			{
				Port = atoi(argv[Inc] + (sizeof "--port=" - 1));
				if (!Port)
				{
					fprintf(stderr, "You specified port 0 (zero). That's not valid. Default is 9990.\n");
					exit(1);
				}
			}
			else if (!strncmp(argv[Inc], "--refresh=", sizeof "--refresh=" - 1))
			{
				unsigned NewRefresh = atoi(argv[Inc] + (sizeof "--refresh=" - 1));
				
				RefreshRate = NewRefresh;
			}
			else if (!strcmp(argv[Inc], "--wzlegacy"))
			{
				puts("Treating server as Warzone 2100 Legacy lobby server.");
				WZLegacy = true;
			}
			else if (!strcmp(argv[Inc], "--help") || !strcmp(argv[Inc], "--version"))
			{
				puts("WZBlue version " WZBLUE_VERSION);
				puts("Arguments are --server=myserver.com, --port=9990, --wzlegacy, --enterkey, --refresh=30.");
				exit(0);
			}
			else if (!strcmp(argv[Inc], "--enterkey"))
			{
				EnterKey = true;
			}
			else
			{
				fprintf(stderr, "Unknown command switch. Try --help.\n");
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
		
		printf("Last update was %s. ", TimeBuf); fflush(NULL); /*We don't leave a newline on purpose.*/
		
		WZ_GetGamesList(Server, Port, WZLegacy);
		
		if (EnterKey)
		{
			
			WZBlue_SetTextColor(GREEN);
			puts("Strike enter to refresh.");
			WZBlue_SetTextColor(ENDCOLOR);
			getchar();
		}
		else
		{	
#ifdef WIN
			Sleep(RefreshRate * 1000);
#else
			usleep(RefreshRate * 1000000);
#endif
		}
	}
	return 0;
}

void WZBlue_SetTextColor(ConsoleColor Color)
{
#ifndef WIN
	printf("\033[%dm", Color);
#else
	BOOL (WINAPI *DoSetConsoleTextAttribute)(HANDLE, WORD) = NULL;
	HANDLE WDescriptor = GetStdHandle(STD_OUTPUT_HANDLE);
	static HMODULE kernel32 = (HMODULE)0xffffffff;
	WORD WinColor = 0;
	
	if (kernel32 == 0)
	{
		return;
	}
	else if(kernel32 == (HMODULE)0xffffffff)
	{
		kernel32 = LoadLibrary("kernel32.dll");
		
		if (kernel32 == 0)
		{
			return;
		}
	}
	
	/*Not supported.*/
	if (!(DoSetConsoleTextAttribute = (BOOL (WINAPI *)(HANDLE, WORD))GetProcAddress(kernel32, "SetConsoleTextAttribute"))) return;
	
	
	switch (Color)
	{
		case BLACK:
			WinColor = 0;
			break;
		case BLUE:
			WinColor = 1;
			break;
		case GREEN:
			WinColor = 2;
			break;
		case CYAN:
			WinColor = 3;
			break;
		case RED:
			WinColor = 4;
			break;
		case MAGENTA:
			WinColor = 5;
			break;
		case YELLOW:
			WinColor = 6;
			break;
		default: /*Everything else just becomes white.*/
			WinColor = 7;
			break;
	}
			
	DoSetConsoleTextAttribute(WDescriptor, WinColor);
#endif
}
