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
#include "substrings/substrings.h"
#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#endif

unsigned RefreshRate = 10; /*10 sec refresh by default*/
static unsigned Counter = 0;
gboolean True = TRUE, False = FALSE;
//Prototypes

static void Main_CheckLoop(void);

gboolean Main_LoopFunc(gboolean *ViaLoop)
{
	uint32_t GamesAvailable = 0;
	GameStruct *GamesList = NULL;
	gboolean RetVal = TRUE;
	
	if (!*ViaLoop) Counter = 0;
	
	GUI_SetStatusBar("Refreshing...");
	GUI_Flush();

	char Server[1024] = { 0 };
	char PortText[16] = { 0 };
	
	SubStrings.Split(Server, PortText, ":", Settings.LobbyURL, SPLIT_NOKEEP);
	const unsigned short Port = atoi(PortText);

	gboolean ConnErr = FALSE;
	
	gboolean Changed = WZ_GetGamesList(Server, Port, &GamesAvailable, &GamesList, &ConnErr);
	
	if (GamesAvailable)
	{
		if (!Changed)
		{
			GUI_SetStatusBar_GameCount(GamesAvailable);
			return RetVal;
		}
	}
	
	GUI_ClearGames(GuiInfo.ScrolledWindow);
	
	if (ConnErr)
	{
		GUI_ConnErr(GuiInfo.ScrolledWindow);
	}
	else if (GamesAvailable)
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
	putenv((char*)"GTK_CSD=0");
		
#ifdef WIN32 /*Fire up winsock2.*/
	putenv((char*)"XDG_DATA_DIRS=.");

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
		for (int Inc = 0; Inc < argc; ++Inc)
		{
			if (!strcmp(argv[Inc], "--help") || !strcmp(argv[Inc], "--version"))
			{
				puts("WZBlue version " WZBLUE_VERSION);
				puts("Options are available in the configuration dialog.");
				exit(0);
			}
			else
			{
				fprintf(stderr, "Unknown command switch. Try --help.\n");
				exit(1);
			}
		}
	}
	
	//Read in settings, if present.
	Settings_ReadSettings();

	//Get version of Warzone binary.
	GUI_GetGameVersion(GameVersion, sizeof GameVersion);
	
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

	
	
	
