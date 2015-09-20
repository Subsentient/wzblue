/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include "wzblue.h"
#include "icon.h"

#ifdef WIN
#include <windows.h>
#include <dirent.h>
#include <tchar.h>
#else
#include <sys/wait.h>
#endif //WIN

struct GooeyGuts GuiInfo;

static void GTK_Destroy(GtkWidget *Widget, gpointer Stuff);
static void GTK_NukeContainerChildren(GtkContainer *Container);
static void GUI_LoadIcon(void);
static void GUI_LaunchGame(const char *IP);
static void GUI_DrawLaunchFailure(void);

static gboolean GUI_FindWZExecutable(char *const Out, unsigned OutMaxSize);


static void GTK_Destroy(GtkWidget *Widget, gpointer Stuff)
{
	gtk_main_quit();
	exit(0);
}

gboolean GUI_CheckSlider(void)
{
	const unsigned NewRate = gtk_range_get_value((GtkRange*)GuiInfo.Slider);
	if (RefreshRate != NewRate)
	{
		RefreshRate = NewRate;
		return true; //Value changed.
	}
	
	return false;

}
	

static void GUI_LoadIcon(void)
{
	GdkPixbufLoader *Loader = gdk_pixbuf_loader_new();
	
	gdk_pixbuf_loader_write(Loader, WZBlueIcon_Data, sizeof WZBlueIcon_Data, NULL);
	GuiInfo.IconPixbuf = gdk_pixbuf_loader_get_pixbuf(Loader);
	
	gtk_window_set_icon((GtkWindow*)GuiInfo.Win, GuiInfo.IconPixbuf);
}
	
void GTK_NukeWidget(GtkWidget *Widgy)
{
	GTK_NukeContainerChildren((GtkContainer*)Widgy);
	gtk_widget_destroy(Widgy);
}

void GUI_Flush(void)
{ //So if we are doing something that locks us up, we can draw any of our notifications before that happens.
	while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
}

void GUI_SetStatusBar(const char *Text)
{
	gtk_statusbar_remove_all((GtkStatusbar*)GuiInfo.StatusBar, GuiInfo.StatusBarContextID);
	
	
	//Nothing to set it to? We're done.
	if (!Text) return;

	gtk_statusbar_push((GtkStatusbar*)GuiInfo.StatusBar, GuiInfo.StatusBarContextID, Text);
}

void GUI_SetStatusBar_GameCount(const uint32_t GamesAvailable)
{
	gtk_statusbar_remove_all((GtkStatusbar*)GuiInfo.StatusBar, GuiInfo.StatusBarContextID);
	
	char Buf[256];
	
	snprintf(Buf, sizeof Buf, "%u game%s available.", (unsigned)GamesAvailable, GamesAvailable == 1 ? "" : "s");
	
	gtk_statusbar_push((GtkStatusbar*)GuiInfo.StatusBar, GuiInfo.StatusBarContextID, Buf);
}

void GUI_DrawAboutDialog()
{
	GtkWidget *AboutWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_type_hint((GtkWindow*)AboutWin, GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_skip_taskbar_hint((GtkWindow*)AboutWin, true);
	gtk_window_set_resizable((GtkWindow*)AboutWin, false);
	
	gtk_window_set_title(GTK_WINDOW(AboutWin), "About WZBlue");

	gtk_container_set_border_width(GTK_CONTAINER(AboutWin), 10);
	
	g_signal_connect(G_OBJECT(AboutWin), "destroy", (GCallback)GTK_NukeWidget, NULL);

	GtkWidget *Align = gtk_alignment_new(1.0, 1.0, 0.1, 1.0);
	
	GtkWidget *VBox = gtk_vbox_new(FALSE, 5);
	GtkWidget *Image = gtk_image_new_from_pixbuf(GuiInfo.IconPixbuf);
	
	gtk_container_add(GTK_CONTAINER(AboutWin), VBox);
	
	GtkWidget *Label1 = gtk_label_new("WZBlue Warzone 2100 Lobby Monitor version " WZBLUE_VERSION);
	char GTKVersion[64];
	snprintf(GTKVersion, sizeof GTKVersion, "Compiled against GTK %d.%d", GTK_MAJOR_VERSION, GTK_MINOR_VERSION);
	
	GtkWidget *Label2 = gtk_label_new(GTKVersion);
	GtkWidget *Label3 = gtk_label_new("By Subsentient.");
	
	GtkWidget *Button = gtk_button_new_from_stock(GTK_STOCK_OK);
	
	gtk_container_add(GTK_CONTAINER(Align), Button);
	
	gtk_box_pack_start((GtkBox*)VBox, Image, TRUE, TRUE, 5);
	gtk_box_pack_start((GtkBox*)VBox, Label1, TRUE, TRUE, 20);
	gtk_box_pack_start((GtkBox*)VBox, Label2, TRUE, TRUE, 0);
	gtk_box_pack_start((GtkBox*)VBox, Label3, TRUE, TRUE, 10);
	gtk_box_pack_start((GtkBox*)VBox, Align, TRUE, FALSE, 0);
	
	g_signal_connect_swapped(G_OBJECT(Button), "clicked", (GCallback)GTK_NukeWidget, AboutWin);
	
	gtk_widget_show_all(AboutWin);
}


void GUI_DrawMenus()
{
	//Main menu bar
	GtkWidget *Align = gtk_alignment_new(0.0, 0.0, 1.0, 0.01);
	GtkWidget *MenuBar = gtk_menu_bar_new();
	GtkAccelGroup *AccelGroup = gtk_accel_group_new();
	
	//File and help tags
	GtkWidget *FileTag = gtk_menu_item_new_with_mnemonic("_File");
	GtkWidget *HelpTag = gtk_menu_item_new_with_mnemonic("_Help");
	
	//The submenus for file and help.
	GtkWidget *FileMenu = gtk_menu_new();
	GtkWidget *HelpMenu = gtk_menu_new();
	
	//Now *set* them as the submenus.
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(FileTag), FileMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(HelpTag), HelpMenu);
	
	//Items for the help menu
	GtkWidget *Item_About = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, AccelGroup);
	
	//Items for the file menu
	GtkWidget *Item_Quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, AccelGroup);
	GtkWidget *Item_Refresh = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, AccelGroup);
	
	//Set up accelerator for Refresh (F5)
	gtk_widget_add_accelerator(Item_Refresh, "activate", AccelGroup, 0xffc2 ,0, GTK_ACCEL_VISIBLE);
	
	g_signal_connect(G_OBJECT(Item_About), "activate", (GCallback)GUI_DrawAboutDialog, NULL);
	g_signal_connect(G_OBJECT(Item_Quit), "activate", (GCallback)GTK_Destroy, NULL);
	
	g_signal_connect_swapped(G_OBJECT(Item_Refresh), "activate", (GCallback)Main_LoopFunc, &False);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(HelpMenu), Item_About);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Refresh);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Quit);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), FileTag);
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), HelpTag);
	
	gtk_container_add((GtkContainer*)Align, MenuBar);
	gtk_container_add((GtkContainer*)GuiInfo.VBox, Align);
	
	//gtk_widget_set_size_request(MenuBar, -1, 20);
	
	gtk_window_add_accel_group((GtkWindow*)GuiInfo.Win, AccelGroup);
	
	gtk_widget_show_all(GuiInfo.VBox);
}

GtkWidget *GUI_InitGUI()
{
	GtkWidget *Win = GuiInfo.Win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GUI_LoadIcon();
	
	//Connect the destroy signal to allow quitting when we close the window.
	g_signal_connect(G_OBJECT(Win), "destroy", G_CALLBACK(GTK_Destroy), NULL);
	
	//Window title
	gtk_window_set_title(GTK_WINDOW(Win), "WZBlue");
	
	gtk_widget_set_size_request(Win, -1, -1);
	
	gtk_window_set_resizable((GtkWindow*)Win, false);

	//Create the main vertical box.
	GtkWidget *VBox = GuiInfo.VBox = gtk_vbox_new(FALSE, 6); //One for menu, one for status, one for games list.
	gtk_container_add((GtkContainer*)Win, VBox);
	
	//Create a scrolled window.
	GtkWidget *ScrolledWindow = GuiInfo.ScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_widget_set_size_request(ScrolledWindow, 800, 400);
	gtk_container_set_border_width(GTK_CONTAINER(ScrolledWindow), 5);
	
	
	///Add to the main vertical box.
	GUI_DrawMenus();
	gtk_box_pack_start((GtkBox*)VBox, ScrolledWindow, FALSE, FALSE, 0);
	
	//Status bar
	GtkWidget *StatusBar = GuiInfo.StatusBar = gtk_statusbar_new();
	GuiInfo.StatusBarContextID = gtk_statusbar_get_context_id((GtkStatusbar*)StatusBar, "global");
	
	
	//Mostly here because I wanna make sure it works
	GUI_SetStatusBar(NULL);
	
	//The refresh button, aligned to the right.
	GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.5, 0.01);
	GtkWidget *Button1 = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	GtkWidget *Button2 = gtk_button_new_with_mnemonic("H_ost Game");
	GtkWidget *B2Image = gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_BUTTON);
	
	gtk_button_set_image((GtkButton*)Button2, (void*)B2Image);
	
	//The refresh delay slider
	GtkWidget *RefreshSlider = GuiInfo.Slider = gtk_hscale_new_with_range(5.0, 120.0, 1.0);
	gtk_scale_set_digits((GtkScale*)RefreshSlider, 0);
	gtk_scale_set_value_pos((GtkScale*)RefreshSlider, GTK_POS_LEFT);
	gtk_range_set_value((GtkRange*)RefreshSlider, RefreshRate);
	
	GtkWidget *HBox = gtk_hbox_new(FALSE, 4);
	
	//Baby vbox
	GtkWidget *BabyVBox = gtk_vbox_new(FALSE, 3);
	GtkWidget *BabyLabel = gtk_label_new("Auto-Refresh delay in seconds");
	GtkWidget *BabySep = gtk_hseparator_new();
	
	gtk_box_pack_start((GtkBox*)BabyVBox, RefreshSlider, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)BabyVBox, BabySep, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)BabyVBox, BabyLabel, FALSE, FALSE, 0);
	

	gtk_box_pack_start((GtkBox*)HBox, Button1, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)HBox, Button2, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)HBox, BabyVBox, TRUE, TRUE, 0);
	
	gtk_container_add((GtkContainer*)Align, HBox);
	
	g_signal_connect_swapped(G_OBJECT(Button1), "clicked", (GCallback)Main_LoopFunc, &False);
	g_signal_connect_swapped(G_OBJECT(Button2), "clicked", (GCallback)GUI_LaunchGame, NULL);
	
	gtk_box_pack_start((GtkBox*)VBox, Align, TRUE, TRUE, 0);
	gtk_box_pack_start((GtkBox*)VBox, StatusBar, FALSE, FALSE, 0);
	
	gtk_widget_show_all(Win);
	
	return ScrolledWindow;
}


//Empty the scrolled window so we can rebuild it.
void GUI_ClearGames(GtkWidget *ScrolledWindow)
{
	if (!ScrolledWindow) return;
	
	GTK_NukeContainerChildren((GtkContainer*)ScrolledWindow);
}

static void GUI_LaunchGame(const char *IP)
{ //Null specifies we want to host.
#ifndef WIN

	char WZExecutable[2048];
    
    if (!GUI_FindWZExecutable(WZExecutable, sizeof WZExecutable))
    {
		GUI_DrawLaunchFailure();
		return;
    }
	
	pid_t PID = fork();
	
	if (PID == -1)
	{
		GUI_DrawLaunchFailure();
		return;
	}
	
	if (PID != 0)
	{
		signal(SIGCHLD, SIG_IGN);
		return;
	}

	char IPFormat[128] = "--join=";
	if (IP == NULL)
	{
		strcpy(IPFormat, "--host");
	}
	else
	{
		strcat(IPFormat, IP);
	}
	
	execlp(WZExecutable, "warzone2100", IPFormat, NULL);
	
	exit(1);

#else //WINDOWS CODE!!
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    
    memset(&StartupInfo, 0, sizeof StartupInfo);
    memset(&ProcessInfo, 0, sizeof ProcessInfo);
    
    StartupInfo.cb = sizeof StartupInfo;
    
    char IPFormat[128] = "--join=";
    char WZExecutable[2048];
    
    if (!GUI_FindWZExecutable(WZExecutable, sizeof WZExecutable))
    {
		GUI_DrawLaunchFailure();
		return;
    }
	if (IP == NULL)
	{
		strcpy(IPFormat, "--host");
	}
	else
	{
		strcat(IPFormat, IP);
	}
	
	strcat(WZExecutable, " ");
	strcat(WZExecutable, IPFormat);
	
	//Launch it
	const gboolean Worked = CreateProcess(NULL, WZExecutable, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
	
	CloseHandle(ProcessInfo.hProcess);
	CloseHandle(ProcessInfo.hThread);
    
	if (!Worked) GUI_DrawLaunchFailure();
#endif //WIN
	return;
	
}

static void GUI_DrawLaunchFailure(void)
{
	GtkWidget *Win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title((GtkWindow*)Win, "Failed to launch Warzone");
	
	gtk_widget_set_size_request(Win, -1, -1);
	
	gtk_window_set_resizable((GtkWindow*)Win, false);
	
	GtkWidget *Table = gtk_table_new(2, 2, FALSE);
	
	gtk_container_add((GtkContainer*)Win, Table);
	
	GtkWidget *Label = gtk_label_new("Failed to launch Warzone 2100! Make sure it's installed?");
	GtkWidget *FailIcon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	GtkWidget *OkButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	
	g_signal_connect_swapped(G_OBJECT(OkButton), "clicked", (GCallback)GTK_NukeWidget, Win);
	
	gtk_table_attach(GTK_TABLE(Table), Label, 1, 2, 0, 1, GTK_EXPAND, GTK_SHRINK, 10, 20);
	gtk_table_attach(GTK_TABLE(Table), FailIcon, 0, 1, 0, 1, GTK_EXPAND, GTK_SHRINK, 10, 20);
	gtk_table_attach(GTK_TABLE(Table), OkButton, 0, 2, 1, 2, GTK_EXPAND, GTK_SHRINK, 0, 10);
	
	gtk_widget_show_all(Win);
}

static gboolean GUI_FindWZExecutable(char *const Out, unsigned OutMaxSize)
{
#ifndef WIN
	const char *Path = getenv("PATH");
	
	if (!Path) exit(1);
	
	char Name[1024];
	const char *Worker = Path;
	
	do
	{
		unsigned Inc = 0;
		
		if (*Worker == ':') ++Worker;
		
		for (; Inc < (sizeof Name - 1 - (sizeof "/warzone2100" - 1)) &&
				Worker[Inc] != ':' && Worker[Inc] != '\0'; ++Inc)
		{
			Name[Inc] = Worker[Inc];
		}
		Name[Inc] = '\0';
		
		strcat(Name, "/warzone2100");
		
		struct stat FileStat;
		//Check if the executable exists.
		
		if (stat(Name, &FileStat) == 0)
		{
			strncpy(Out, Name, OutMaxSize - 1);
			Out[OutMaxSize - 1] = '\0';
			return true;
		}
	} while ((Worker = strchr(Worker, ':')) != NULL);
	
	*Out = '\0';
	
	return false;
#else
	const char *ProgDir = getenv("programfiles");
	puts(ProgDir);
	DIR *Folder = opendir(ProgDir);
	struct dirent *File = NULL;
	
	while ((File = readdir(Folder)))
	{
		char Filename[256];
		
		strncpy(Filename, File->d_name, sizeof Filename - 1);
		Filename[sizeof Filename - 1] = '\0';
		*Filename = tolower(*Filename);
		if (!strncmp(Filename, "warzone", sizeof "warzone" - 1))
		{
			//We found the folder! Build our new pathname!
			char NewPath[1024];
			
			snprintf(NewPath, sizeof NewPath, "%s\\%s\\warzone2100.exe", ProgDir, File->d_name);
			
			struct stat Stat;
			
			if (stat(NewPath, &Stat) == 0)
			{
				strncpy(Out, NewPath, OutMaxSize - 1);
				Out[OutMaxSize - 1] = '\0';
				
				closedir(Folder);
				return true;
			}
		}
	}
	
	closedir(Folder);
	*Out = '\0';
	return false;

#endif //WIN
}

static void GTK_NukeContainerChildren(GtkContainer *Container)
{
	GList *Children = gtk_container_get_children(Container);
	GList *Worker = Children;
	
	if (!GTK_IS_CONTAINER(Container)) return;
	
	for (; Worker; Worker = g_list_next(Worker))
	{
		if (GTK_IS_CONTAINER((GtkWidget*)Worker->data))
		{
			GTK_NukeContainerChildren(Worker->data);
		}
		
		gtk_widget_destroy((GtkWidget*)Worker->data);
	}
	
	g_list_free(Children);
}	

void GUI_NoGames(GtkWidget *ScrolledWindow)
{
	GtkWidget *VBox = gtk_vbox_new(false, 1); //Because it wants a container.
	
	GtkWidget *Label = gtk_label_new("No games available.");
	gtk_box_pack_start(GTK_BOX(VBox), Label, FALSE, FALSE, 20);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ScrolledWindow), VBox);
	gtk_widget_show_all(ScrolledWindow);
}

void GUI_RenderGames(GtkWidget *ScrolledWindow, GameStruct *GamesList, uint32_t GamesAvailable)
{
	static void *FreeAfterRender[64];
	
	//We want it static so that it persists when this function returns, but we still need to wipe it each call.
	for (int Inc = 0; Inc < 64 && FreeAfterRender[Inc] != NULL; ++Inc)
	{
		free(FreeAfterRender[Inc]);
	}
	
	memset(FreeAfterRender, 0, sizeof FreeAfterRender);
	
	if (!ScrolledWindow || !GamesList || !GamesAvailable) return;
	
	GtkWidget *VBox = gtk_vbox_new(false, (GamesAvailable * 2) - 1);
	
	unsigned Inc = 0;
	char OutBuf[2048];
	const char *StockID = NULL;
	
	gtk_box_pack_start(GTK_BOX(VBox), gtk_hseparator_new(), FALSE, FALSE, 0);
	
	for (; Inc < GamesAvailable; ++Inc)
	{
		char ModString[384] = { '\0' };
		const Bool MapMod = GamesList[Inc].MapMod;
		
		if (GamesList[Inc].NetSpecs.CurPlayers >= GamesList[Inc].NetSpecs.MaxPlayers)
		{ /*Game is full.*/
			StockID = GTK_STOCK_NO;
		}
		else if (GamesList[Inc].PrivateGame)
		{ /*Private game.*/
			StockID = GTK_STOCK_DIALOG_AUTHENTICATION;
		}
		else if (*GamesList[Inc].ModList != '\0')
		{ /*It has mods.*/
			StockID = GTK_STOCK_CONVERT;
		}
		else
		{ /*Normal, joinable game.*/
			StockID = GTK_STOCK_YES;
		}
		
		/*Add mod warning even if we don't get the color for that.*/
		if (GamesList[Inc].ModList[0] != '\0')
		{
			snprintf(ModString, sizeof ModString, " <b><span foreground=\"red\">(mods: %s)</span></b>",  GamesList[Inc].ModList);
		}
		
		
		snprintf(OutBuf, sizeof OutBuf, "Name: <b><span foreground=\"blue\">%s</span></b> | "
									"Map: <b><span foreground=\"#00bb00\">%s</span></b>%s | "
									"Host: <b><span foreground=\"#cc00cc\">%s</span></b>\n"
									"Players: <b>%d/%d</b> %s| IP: <b>%s</b> | Version: <b>%s</b>%s",
				GamesList[Inc].GameName, GamesList[Inc].Map, MapMod ? " <b><span foreground=\"red\">(map-mod)</span></b>" : "",
				GamesList[Inc].HostNick, GamesList[Inc].NetSpecs.CurPlayers, GamesList[Inc].NetSpecs.MaxPlayers,
				GamesList[Inc].PrivateGame ? "<b><span foreground=\"#cece00\">(private)</span></b> " : "", GamesList[Inc].NetSpecs.HostIP,
				GamesList[Inc].VersionString, ModString);
				
		GtkWidget *Icon = gtk_image_new_from_stock(StockID, GTK_ICON_SIZE_BUTTON);
		
		GtkWidget *Label = gtk_label_new("");
		gtk_label_set_markup((GtkLabel*)Label, OutBuf);
		
		GtkWidget *Button = gtk_button_new_with_label("Join");
		
		
		//Add a temporary pointer to our temporary pointer list
		void *Ptr = NULL;
		for (guint Inc = 0; Inc < 64; ++Inc)
		{
			if (FreeAfterRender[Inc] == NULL)
			{
				FreeAfterRender[Inc] = Ptr = strdup(GamesList[Inc].NetSpecs.HostIP);
				break;
			}
		}
		
		if (Ptr == NULL) Ptr = "";
		
		g_signal_connect_swapped(G_OBJECT(Button), "clicked", (GCallback)GUI_LaunchGame, Ptr);
		
		if (GamesList[Inc].NetSpecs.CurPlayers == GamesList[Inc].NetSpecs.MaxPlayers)
		{
			gtk_widget_set_sensitive(Button, FALSE);
			gtk_button_set_label((GtkButton*)Button, "Game Full");
		}
		
		GtkWidget *HBox = gtk_hbox_new(false, 4);
		GtkWidget *HBox2 = gtk_hbox_new(false, 2);
		
		
		GtkWidget *VSep = gtk_vseparator_new(), *VSep2 = gtk_vseparator_new();
		
		gtk_box_pack_start(GTK_BOX(HBox2), Button, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox2), VSep2, FALSE, FALSE, 0);
		
		gtk_box_pack_start(GTK_BOX(HBox), Icon, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), VSep, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), HBox2, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), Label, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(VBox), HBox, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(VBox), gtk_hseparator_new(), FALSE, FALSE, 0);

	}
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ScrolledWindow), VBox);
	
	gtk_widget_show_all(ScrolledWindow);
}
