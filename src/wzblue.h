/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/

#ifndef __WZBLUE_H__
#define __WZBLUE_H__

#define WZBLUE_VERSION "1.1"

#include <gtk/gtk.h>

#define NUMELEMENTS(x) (sizeof x / sizeof *x)

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

struct GooeyGuts
{
	//Widgets
	GtkWidget *StatusBar;
	GtkWidget *Win;
	GtkWidget *ScrolledWindow;
	GtkWidget *Table;
	GtkWidget *Slider;
	
	//Icon pixbufs
	GdkPixbuf *WMIcon_Pixbuf;
	GdkPixbuf *Icon_Open_Pixbuf;
	GdkPixbuf *Icon_Full_Pixbuf;
	GdkPixbuf *Icon_Locked_Pixbuf;
	GdkPixbuf *Icon_Mod_Pixbuf;
	
	//Widget properties
	guint StatusBarContextID;

};


//Globals
extern unsigned RefreshRate;
extern struct GooeyGuts GuiInfo;
extern gboolean True, False;
/*main.c*/
gboolean Main_LoopFunc(gboolean *ViaLoop);

/*netcore.c*/
gboolean Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
gboolean Net_Disconnect(int SockDescriptor);
gboolean Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, gboolean TextStream);
gboolean Net_Disconnect(int SockDescriptor);
gboolean Net_Write(int SockDescriptor, const char *InMsg);

/*wz.c*/
gboolean WZ_GetGamesList(const char *Server, unsigned short Port, uint32_t *GamesAvailable, GameStruct **Pointer);
void WZ_SendGamesList(const GameStruct *GamesList, uint32_t GamesAvailable, GtkWidget *ScrolledWindow);

/*gui.c*/
GtkWidget *GUI_InitGUI();
void GUI_RenderGames(GtkWidget *ScrolledWindow, GameStruct *GamesList, uint32_t GamesAvailable);
void GUI_ClearGames(GtkWidget *ScrolledWindow);
void GUI_NoGames(GtkWidget *ScrolledWindow);
void GUI_SetStatusBar(const char *Text);
void GUI_SetStatusBar_GameCount(const uint32_t GamesAvailable);
void GUI_Flush(void);
gboolean GUI_CheckSlider(void);

#endif //__WZBLUE_H__
