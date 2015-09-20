/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "wzblue.h"
#ifdef WIN
#include <windows.h>
#include <commctrl.h>
#endif

unsigned RefreshRate = 10; /*10 sec refresh by default*/
static char Server[128] = "lobby.wz2100.net"; //The lobby server hostname.
static int Port = 9990;
static unsigned Counter = 0;
gboolean True = true, False = false;
//Prototypes

static void Main_CheckLoop(void);

gboolean Main_LoopFunc(gboolean *ViaLoop)
{
	uint32_t GamesAvailable = 0;
	GameStruct *GamesList = NULL;
	gboolean RetVal = true;
	
	if (!*ViaLoop) Counter = 0;
	
	GUI_SetStatusBar("Refreshing...");
	GUI_Flush();
	Bool Changed = WZ_GetGamesList(Server, Port, &GamesAvailable, &GamesList);
	
	if (GamesAvailable)
	{
		if (!Changed)
		{
			GUI_SetStatusBar_GameCount(GamesAvailable);
			return RetVal;
		}
	}
	
	GUI_ClearGames(GuiInfo.ScrolledWindow);
	
	if (GamesAvailable)
	{
		GUI_RenderGames(GuiInfo.ScrolledWindow, GamesList, GamesAvailable);
	}
	else
	{
		GUI_NoGames(GuiInfo.ScrolledWindow);
	}
	
	GUI_SetStatusBar_GameCount(GamesAvailable);
	
	return RetVal;
}
	
	
int main(int argc, char **argv)
{
	int Inc = 1;
#ifdef WIN /*Fire up winsock2.*/
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    { /*Initialize winsock*/
        fprintf(stderr, "Unable to initialize WinSock2!");
        exit(1);
	}
	
	INITCOMMONCONTROLSEX ICC;
	ICC.dwSize = sizeof(ICC);
	ICC.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&ICC);
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
			else if (!strcmp(argv[Inc], "--help") || !strcmp(argv[Inc], "--version"))
			{
				puts("WZBlue version " WZBLUE_VERSION);
				puts("Arguments are --server=myserver.com, --port=9990");
				exit(0);
			}
			else
			{
				fprintf(stderr, "Unknown command switch. Try --help.\n");
				exit(1);
			}
		}
	}
	
	//Start the GUI
	gtk_init(&argc, &argv);
	
	GUI_InitGUI();
	
	//Every second
	g_timeout_add(1000, (GSourceFunc)Main_CheckLoop, NULL);
	
	//Run it once.
	Main_LoopFunc(&False);
	
	gtk_main();
	
	return 0;
}

static void Main_CheckLoop(void)
{	
	GUI_CheckSlider();
	
	if (Counter >= RefreshRate)
	{
		Counter = 0;
		Main_LoopFunc(&True);
	}
	
	++Counter;
}

	
	
	
