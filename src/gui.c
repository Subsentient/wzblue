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
#include "substrings/substrings.h"
#include "icons.h"

#ifdef WIN32
#include <windows.h>
#include <dirent.h>
#include <tchar.h>
#else
#include <sys/wait.h>
#endif //WIN

struct GooeyGuts GuiInfo;

static GameStruct HostGS = { .Internal_IsHost = true };
static GameStruct EmptyGS = { .Internal_IsEmpty = true };

char GameVersion[1024];

static void GTK_Destroy(GtkWidget *Widget, gpointer Stuff);
static void GTK_NukeContainerChildren(GtkContainer *Container);
static void GUI_LoadIcons(void);
static void GUI_LaunchGame(const GameStruct *GS);
static void GUI_DrawLaunchFailure(void);
static void GUI_DrawSettingsDialog(void);
static void GUI_OpenColorWheel(void);
static gboolean GUI_FindWZExecutable(char *const Out, unsigned OutMaxSize);
static void GUI_GetBinaryCWD(const char *In, char *Out, unsigned OutMax);

static void GTK_Destroy(GtkWidget *Widget, gpointer Stuff)
{
	Settings_SaveSettings();
	gtk_main_quit();
	exit(0);
}

static void GUI_OpenColorWheel(void)
{
	GtkWidget *ColorDialog = gtk_color_selection_dialog_new("Color wheel");
	g_signal_connect((GObject*)ColorDialog, "response", (GCallback)GTK_NukeWidget, ColorDialog);
	gtk_widget_show_all(ColorDialog);
}

gboolean GUI_CheckSlider(void)
{
	const unsigned NewRate = gtk_range_get_value((GtkRange*)GuiInfo.Slider);
	if (RefreshRate != NewRate)
	{
		RefreshRate = NewRate;
		return TRUE; //Value changed.
	}
	
	return FALSE;

}
	

static void GUI_LoadIcons(void)
{
	
	//Our logo
	GdkPixbufLoader *WMIconL = gdk_pixbuf_loader_new();
	
	gdk_pixbuf_loader_write(WMIconL, WMIcon, sizeof WMIcon, NULL);
	gdk_pixbuf_loader_close(WMIconL, NULL);
	
	GuiInfo.WMIcon_Pixbuf = gdk_pixbuf_loader_get_pixbuf(WMIconL);
	 
	//Full icon
	GdkPixbufLoader *FullLoader = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(FullLoader, Icon_Full, sizeof Icon_Full, NULL);
	gdk_pixbuf_loader_close(FullLoader, NULL);

	//Open icon
	GdkPixbufLoader *OpenLoader = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(OpenLoader, Icon_Open, sizeof Icon_Open, NULL);
	gdk_pixbuf_loader_close(OpenLoader, NULL);
	
	//Mod icon
	GdkPixbufLoader *ModLoader = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(ModLoader, Icon_Mod, sizeof Icon_Mod, NULL);
	gdk_pixbuf_loader_close(ModLoader, NULL);
	
	//Locked icon
	GdkPixbufLoader *LockedLoader = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(LockedLoader, Icon_Locked, sizeof Icon_Locked, NULL);
	gdk_pixbuf_loader_close(LockedLoader, NULL);
	

	GuiInfo.Icon_Full_Pixbuf = gdk_pixbuf_loader_get_pixbuf(FullLoader);
	GuiInfo.Icon_Open_Pixbuf = gdk_pixbuf_loader_get_pixbuf(OpenLoader);
	GuiInfo.Icon_Mod_Pixbuf = gdk_pixbuf_loader_get_pixbuf(ModLoader);
	GuiInfo.Icon_Locked_Pixbuf = gdk_pixbuf_loader_get_pixbuf(LockedLoader);
	
	gtk_window_set_icon((GtkWindow*)GuiInfo.Win, GuiInfo.WMIcon_Pixbuf);
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
	gtk_window_set_skip_taskbar_hint((GtkWindow*)AboutWin, TRUE);
	gtk_window_set_resizable((GtkWindow*)AboutWin, FALSE);
	
	gtk_window_set_title(GTK_WINDOW(AboutWin), "About WZBlue");

	gtk_container_set_border_width(GTK_CONTAINER(AboutWin), 10);
	
	g_signal_connect(G_OBJECT(AboutWin), "destroy", (GCallback)GTK_NukeWidget, NULL);

	GtkWidget *ButtonAlign = gtk_alignment_new(1.0, 1.0, 0.1, 1.0);
	
	GtkWidget *VBox = gtk_vbox_new(FALSE, 5);
	GtkWidget *Image = gtk_image_new_from_pixbuf(GuiInfo.WMIcon_Pixbuf);
	
	gtk_container_add(GTK_CONTAINER(AboutWin), VBox);
	char AboutString[2048];
	
	snprintf(AboutString, sizeof AboutString, "WZBlue Warzone 2100 Lobby Monitor version " WZBLUE_VERSION "\n"
										"Compiled against GTK %d.%d.%d\n\n"
										"By Subsentient. Game status icons by tmp500.", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
	
	GtkWidget *Button = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget *Label = gtk_label_new(AboutString);
	GtkWidget *LabelAlign = gtk_alignment_new(0.5, 0.5, 0.1, 0.1);
	gtk_container_add((GtkContainer*)LabelAlign, Label);
	
	gtk_container_add(GTK_CONTAINER(ButtonAlign), Button);
	
	gtk_box_pack_start((GtkBox*)VBox, Image, TRUE, TRUE, 5);
	gtk_box_pack_start((GtkBox*)VBox, LabelAlign, TRUE, TRUE, 10);

	gtk_box_pack_start((GtkBox*)VBox, ButtonAlign, TRUE, FALSE, 0);
	
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
	GtkWidget *Item_Launch = gtk_image_menu_item_new_with_mnemonic("_Launch Warzone");
	GtkWidget *Item_Launch_Image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
	GtkWidget *Item_Settings = gtk_image_menu_item_new_with_mnemonic("_Settings");
	GtkWidget *Item_Settings_Image = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
	
	g_object_set((GObject*)Item_Settings, "image", Item_Settings_Image, NULL);
	g_object_set((GObject*)Item_Launch, "image", Item_Launch_Image, NULL);
	
	
	//Set up accelerator for Refresh (F5)
	gtk_widget_add_accelerator(Item_Refresh, "activate", AccelGroup, 0xffc2 ,0, GTK_ACCEL_VISIBLE);
	
	g_signal_connect(G_OBJECT(Item_About), "activate", (GCallback)GUI_DrawAboutDialog, NULL);
	g_signal_connect_swapped(G_OBJECT(Item_Launch), "activate", (GCallback)GUI_LaunchGame, &EmptyGS);
	g_signal_connect(G_OBJECT(Item_Settings), "activate", (GCallback)GUI_DrawSettingsDialog, NULL);
	g_signal_connect_swapped(G_OBJECT(Item_Quit), "activate", (GCallback)GTK_Destroy, GuiInfo.Win);
	
	g_signal_connect_swapped(G_OBJECT(Item_Refresh), "activate", (GCallback)Main_LoopFunc, &False);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(HelpMenu), Item_About);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Refresh);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Launch);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Settings);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Quit);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), FileTag);
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), HelpTag);
	
	gtk_container_add((GtkContainer*)Align, MenuBar);
	gtk_table_attach(GTK_TABLE(GuiInfo.Table), Align, 0, 6, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);

	
	//gtk_widget_set_size_request(MenuBar, -1, 20);
	
	gtk_window_add_accel_group((GtkWindow*)GuiInfo.Win, AccelGroup);
	
	gtk_widget_show_all(GuiInfo.Table);
}

static void GUI_GetBinaryCWD(const char *In, char *Out, unsigned OutMax)
{
#ifdef WIN32
	const char *Delim = "\\";
#else
	const char *Delim = "/";
#endif
	char CWD[1024];
	SubStrings.Copy(CWD, In, sizeof CWD);
	
	const char *Worker = CWD;
	char *Final = NULL;
	
	while ( (Worker = SubStrings.CFind(*Delim, 1, Worker)) )
	{
		Final = (char*)Worker;
		if (*Worker == *Delim) ++Worker;
	}
	
	if (Final)
	{
		*Final = '\0';
		SubStrings.Copy(Out, CWD, OutMax);
	}
	else if (OutMax > 0) *Out = '\0';
}
	
	
GtkWidget *GUI_InitGUI()
{
	GtkWidget *Win = GuiInfo.Win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size((GtkWindow*)Win, 800, 500);
	GUI_LoadIcons();
	
	//Connect the destroy signal to allow quitting when we close the window.
	g_signal_connect(G_OBJECT(Win), "destroy", G_CALLBACK(GTK_Destroy), NULL);
	
	//Window title
	gtk_window_set_title(GTK_WINDOW(Win), "WZBlue");
	
	gtk_widget_set_size_request(Win, -1, -1);
	
	//gtk_window_set_resizable((GtkWindow*)Win, FALSE);

	//Create the main vertical box.
	GtkWidget *Table = GuiInfo.Table = gtk_table_new(6, 4, FALSE);
	
	//Create a scrolled window.
	GtkWidget *ScrolledWindow = GuiInfo.ScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy((GtkScrolledWindow*)ScrolledWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	//gtk_widget_set_size_request(ScrolledWindow, 800, 400);
	gtk_container_set_border_width((GtkContainer*)ScrolledWindow, 5);
	
	
	///Add to the main vertical box.
	GUI_DrawMenus();
	gtk_table_attach((GtkTable*)Table, ScrolledWindow, 0, 6, 1, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

	
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

	GtkWidget *CheckButton = gtk_check_button_new();

	g_signal_connect((GObject*)CheckButton, "toggled", (GCallback)Settings_SetHideIncompatible, NULL);

	gtk_toggle_button_set_active((GtkToggleButton*)CheckButton, Settings.HideIncompatibleGames == CHOICE_YES);
	
	gtk_box_pack_start((GtkBox*)HBox, gtk_vseparator_new(), FALSE, TRUE, 0);
	gtk_box_pack_start((GtkBox*)HBox, gtk_label_new("Hide incompatible games"), FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)HBox, CheckButton, FALSE, FALSE, 0);
	
	gtk_container_add((GtkContainer*)Align, HBox);
	
	g_signal_connect_swapped(G_OBJECT(Button1), "clicked", (GCallback)Main_LoopFunc, &False);
	g_signal_connect_swapped(G_OBJECT(Button2), "clicked", (GCallback)GUI_LaunchGame, &HostGS);
	
	gtk_table_attach((GtkTable*)Table, Align, 0, 3, 4, 5, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, StatusBar, 0, 6, 5, 6, GTK_FILL, GTK_SHRINK, 0, 0);
	
	gtk_container_add((GtkContainer*)Win, Table);
	gtk_widget_show_all(Win);
	return ScrolledWindow;
}


static void GUI_CreateRadioButtonGroup(const unsigned Count, const char *const Names[], GtkWidget *Out[])
{
	unsigned Inc = 0;
	
	void *Prev = NULL;
	
	for (; Inc < Count; ++Inc)
	{
		GtkWidget *(*Func)() = Inc == 0 ? (GtkWidget*(*)())gtk_radio_button_new_with_label : (GtkWidget*(*)())gtk_radio_button_new_with_label_from_widget;
		
		Out[Inc] = Prev = Func(Prev, Names[Inc]);
	}
}
		
static void GUI_DrawSettingsDialog(void)
{
	GtkWidget *Win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	//Connect the destroy signal
	g_signal_connect((GObject*)Win, "destroy", (GCallback)GTK_NukeWidget, NULL);

	//Window settings
	gtk_window_set_type_hint((GtkWindow*)Win, GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_skip_taskbar_hint((GtkWindow*)Win, TRUE);
	gtk_window_set_resizable((GtkWindow*)Win, FALSE);
	gtk_window_set_title((GtkWindow*)Win, "Settings");
	gtk_container_set_border_width((GtkContainer*)Win, 5);
	
	GtkWidget *Table = gtk_table_new(20, 5, FALSE);
	
	gtk_container_add((GtkContainer*)Win, Table);
	
	//Main window label and the separator.
	GtkWidget *Label = gtk_label_new("Game launch options");
	
	unsigned Row = 0;
	
	gtk_table_attach((GtkTable*)Table, Label, 0, 5, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	++Row;
	gtk_table_attach((GtkTable*)Table, gtk_hseparator_new(), 0, 5, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	++Row;
	
	///Options

	//Choose lobby server
	GtkWidget *LobbyURLLabel = gtk_label_new("Lobby server");
	GtkWidget *LobbyURLLabelAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	GtkWidget *LobbyURLSeparator = gtk_vseparator_new();
	GtkWidget *LobbyURLEntry = gtk_entry_new();
	GtkWidget *LobbyURLSaveButton = gtk_button_new_with_label("Save");
	
	gtk_entry_set_text((GtkEntry*)LobbyURLEntry, Settings.LobbyURL);

	g_signal_connect_swapped((GObject*)LobbyURLSaveButton, "clicked", (GCallback)Settings_LobbyURL_Save, LobbyURLEntry);
	
	gtk_container_add((GtkContainer*)LobbyURLLabelAlign, LobbyURLLabel);
	gtk_table_attach((GtkTable*)Table, LobbyURLLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, LobbyURLSeparator, 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, LobbyURLEntry, 2, 4, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, LobbyURLSaveButton, 4, 5, Row, Row + 1, GTK_EXPAND | GTK_SHRINK, GTK_SHRINK, 0, 0);
	++Row;
	
	//Choose Warzone binary
	GtkWidget *BinaryChooserLabel = gtk_label_new("Warzone 2100 binary");
	GtkWidget *BinaryChooser = gtk_file_chooser_button_new("Select Warzone 2100 binary", GTK_FILE_CHOOSER_ACTION_OPEN);
	
#ifdef WIN32
	char WZBinary[1024] = "file:///";
	const gboolean AutoDetectedBinary = GUI_FindWZExecutable(WZBinary + sizeof "file:///" - 1, sizeof WZBinary - (sizeof "file:///" - 1));
#else
	char WZBinary[1024] = "file://";
	const gboolean AutoDetectedBinary = GUI_FindWZExecutable(WZBinary + sizeof "file://" - 1, sizeof WZBinary - (sizeof "file://" - 1));
#endif

	char *TmpBuf = malloc(sizeof WZBinary * 2);
	SubStrings.Replace(WZBinary, TmpBuf, sizeof WZBinary, " ", "%20");
#ifdef WIN32
	SubStrings.Replace(WZBinary, TmpBuf, sizeof WZBinary, "\\", "/");
#endif
	free(TmpBuf);
	
	if (*Settings.WZBinary || AutoDetectedBinary)
	{ //Set default file.
		if (!gtk_file_chooser_set_uri((GtkFileChooser*)BinaryChooser, *Settings.WZBinary ? Settings.WZBinary : WZBinary))
		{
			fputs("BinaryChooser URI change failed.\n", stderr);
		}
	}
	
	g_signal_connect((GObject*)BinaryChooser, "file-set", (GCallback)Settings_SetBinary, NULL);
	
	GtkWidget *BinaryChooserSep = gtk_vseparator_new();
	GtkWidget *BinaryChooserLabelAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	gtk_container_add((GtkContainer*)BinaryChooserLabelAlign, BinaryChooserLabel);
	
	gtk_table_attach((GtkTable*)Table, BinaryChooserLabelAlign, 0, 1, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, BinaryChooserSep, 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, BinaryChooser, 2, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	
	++Row;
	
	//Resolution
	static GtkWidget *ResBox[2];
	ResBox[0] = gtk_entry_new();
	ResBox[1] = gtk_entry_new();
	
	
	if (Settings.Resolution.Width && Settings.Resolution.Height)
	{
		char W[64], H[64];
		snprintf(W, sizeof W, "%u", Settings.Resolution.Width);
		snprintf(H, sizeof H, "%u", Settings.Resolution.Height);
		gtk_entry_set_text((GtkEntry*)ResBox[0], W);
		gtk_entry_set_text((GtkEntry*)ResBox[1], H);
	}
	
	GtkWidget *ResXLabel = gtk_label_new("x");
	gtk_widget_set_size_request(ResBox[0], 45, -1);
	gtk_widget_set_size_request(ResBox[1], 45, -1);
	
	GtkWidget *ResHBox = gtk_hbox_new(FALSE, 5);
	GtkWidget *ResHBoxAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	GtkWidget *ResButton = gtk_button_new_with_label("Save");
	GtkWidget *ResButton2 = gtk_button_new_with_label("Clear");
	
	g_signal_connect((GObject*)ResButton, "clicked", (GCallback)Settings_SetResolution, ResBox);
	g_signal_connect((GObject*)ResButton2, "clicked", (GCallback)Settings_ClearResolution, ResBox);
	
	gtk_container_add((GtkContainer*)ResHBoxAlign, ResHBox);
	
	gtk_box_pack_start((GtkBox*)ResHBox, ResBox[0], FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)ResHBox, ResXLabel, FALSE, FALSE, 3);
	gtk_box_pack_start((GtkBox*)ResHBox, ResBox[1], FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)ResHBox, ResButton, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)ResHBox, ResButton2, FALSE, FALSE, 0);
	
	GtkWidget *ResLabel = gtk_label_new("Resolution");
	GtkWidget *ResLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	gtk_container_add((GtkContainer*)ResLabelAlign, ResLabel);
	
	gtk_table_attach((GtkTable*)Table, ResLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, ResHBoxAlign, 2, 3, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	++Row;
	//Radio buttons
	GtkWidget *RadioButtons[3] = { NULL };
	const char *const RadioLabels[3] = { "Unspecified", "No", "Yes" };
	
	//Fullscreen
	GtkWidget *FullscreenLabel = gtk_label_new("Display mode");
	GtkWidget *FullscreenLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	gtk_container_add((GtkContainer*)FullscreenLabelAlign, FullscreenLabel);
	
	const char *const ScreenRadioLabels[3] = { "Unspecified", "Windowed", "Fullscreen" };
	GUI_CreateRadioButtonGroup(3, ScreenRadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.Fullscreen);
	
	gtk_table_attach((GtkTable*)Table, FullscreenLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetFullscreen, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetFullscreen, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetFullscreen, DefaultChoices + 2);
	
	++Row;
	
	//Sound
	GtkWidget *SoundLabel = gtk_label_new("Enable sound");
	GtkWidget *SoundLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	gtk_container_add((GtkContainer*)SoundLabelAlign, SoundLabel);
	
	GUI_CreateRadioButtonGroup(3, RadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.Sound);
	
	gtk_table_attach((GtkTable*)Table, SoundLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetSound, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetSound, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetSound, DefaultChoices + 2);
	
	++Row;
	
	//Shadows
	GtkWidget *ShadowLabel = gtk_label_new("Enable shadows");
	GtkWidget *ShadowLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	gtk_container_add((GtkContainer*)ShadowLabelAlign, ShadowLabel);
	
	GUI_CreateRadioButtonGroup(3, RadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.Shadows);
	
	gtk_table_attach((GtkTable*)Table, ShadowLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetShadows, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetShadows, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetShadows, DefaultChoices + 2);

	++Row;
	
	//Texture compression
	GtkWidget *TextureCompressLabel = gtk_label_new("Enable texture compression");
	GtkWidget *TextureCompressLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	
	gtk_container_add((GtkContainer*)TextureCompressLabelAlign, TextureCompressLabel);
	
	GUI_CreateRadioButtonGroup(3, RadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.TextureCompression);
	
	gtk_table_attach((GtkTable*)Table, TextureCompressLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	

	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetTextureCompress, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetTextureCompress, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetTextureCompress, DefaultChoices + 2);

	++Row;
	
	//Shaders
	GtkWidget *ShadersLabel = gtk_label_new("Enable shaders");
	GtkWidget *ShadersLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	
	gtk_container_add((GtkContainer*)ShadersLabelAlign, ShadersLabel);
	
	GUI_CreateRadioButtonGroup(3, RadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.Shaders);
	
	gtk_table_attach((GtkTable*)Table, ShadersLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	//Disable the yes button
	gtk_widget_set_sensitive(RadioButtons[2], FALSE);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetShaders, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetShaders, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetShaders, DefaultChoices + 2);
	
	++Row;
	
	//VBOS
	GtkWidget *VBOSLabel = gtk_label_new("Enable OpenGL VBOS");
	GtkWidget *VBOSLabelAlign = gtk_alignment_new(0.0, 0.5, 0.1, 0.1);
	
	gtk_container_add((GtkContainer*)VBOSLabelAlign, VBOSLabel);
	
	GUI_CreateRadioButtonGroup(3, RadioLabels, RadioButtons);
	
	Settings_RadioButtonInit(RadioButtons, Settings.VBOS);
	
	gtk_table_attach((GtkTable*)Table, VBOSLabelAlign, 0, 1, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	gtk_table_attach((GtkTable*)Table, gtk_vseparator_new(), 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[0], 2, 3, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[1], 3, 4, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, RadioButtons[2], 4, 5, Row, Row + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	//Disable the yes button
	gtk_widget_set_sensitive(RadioButtons[2], FALSE);
	
	g_signal_connect((GObject*)RadioButtons[0], "toggled", (GCallback)Settings_SetVBOS, DefaultChoices);
	g_signal_connect((GObject*)RadioButtons[1], "toggled", (GCallback)Settings_SetVBOS, DefaultChoices + 1);
	g_signal_connect((GObject*)RadioButtons[2], "toggled", (GCallback)Settings_SetVBOS, DefaultChoices + 2);
	
	++Row;
	
	
	///Color options
	
	gtk_table_attach((GtkTable*)Table, gtk_hseparator_new(), 0, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	++Row;
	gtk_table_attach((GtkTable*)Table, gtk_label_new("Color options"), 0, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	++Row;
	gtk_table_attach((GtkTable*)Table, gtk_hseparator_new(), 0, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	++Row;
	
	
	//Name color
	GtkWidget *NameLabel = gtk_label_new("Color for game name");
	GtkWidget *NameLabelAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	GtkWidget *NameSep = gtk_vseparator_new();
	GtkWidget *NameColor = gtk_entry_new();
	gtk_entry_set_text((GtkEntry*)NameColor, Settings.Colors.Name);
	
	gtk_widget_set_size_request(NameColor, 70, -1);
	
	GtkWidget *NameHBox = gtk_hbox_new(FALSE, 2);
	GtkWidget *NameColorAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	gtk_container_add((GtkContainer*)NameColorAlign, NameHBox);

	GtkWidget *NameButton = gtk_button_new_with_label("Save");
	g_signal_connect_swapped((GObject*)NameButton, "clicked", (GCallback)Settings_Color_SetNameColor, NameColor);
	gtk_box_pack_start((GtkBox*)NameHBox, NameColor, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)NameHBox, NameButton, FALSE, FALSE, 0);
	
	// *** COLOR PICKER PACKED NEXT TO NAME ***
	GtkWidget *ColorPickerAlign = gtk_alignment_new(1.0, 0.5, 0.01, 0.01);

	GtkWidget *ColorPickerButton = gtk_button_new();
	GtkWidget *ColorPickerImage = gtk_image_new_from_stock(GTK_STOCK_SELECT_COLOR, GTK_ICON_SIZE_BUTTON);
	g_signal_connect((GObject*)ColorPickerButton, "clicked", (GCallback)GUI_OpenColorWheel, NULL);
	
	g_object_set((GObject*)ColorPickerButton, "image", ColorPickerImage, NULL);
	gtk_container_add((GtkContainer*)ColorPickerAlign, ColorPickerButton);

	
	gtk_container_add((GtkContainer*)NameLabelAlign, NameLabel);
	
	gtk_table_attach((GtkTable*)Table, NameLabelAlign, 0, 1, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, NameSep, 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, NameColorAlign, 2, 4, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, ColorPickerAlign, 4, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	++Row;
	//Map color
	GtkWidget *MapColor = gtk_entry_new();
	gtk_entry_set_text((GtkEntry*)MapColor, Settings.Colors.Map);
	
	gtk_widget_set_size_request(MapColor, 70, -1);
	GtkWidget *MapHBox = gtk_hbox_new(FALSE, 2);
	GtkWidget *MapColorAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	gtk_container_add((GtkContainer*)MapColorAlign, MapHBox);

	GtkWidget *MapLabel = gtk_label_new("Color for map name");
	GtkWidget *MapLabelAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	GtkWidget *MapSep = gtk_vseparator_new();
	GtkWidget *MapButton = gtk_button_new_with_label("Save");
	g_signal_connect_swapped((GObject*)MapButton, "clicked", (GCallback)Settings_Color_SetMapColor, MapColor);
	gtk_box_pack_start((GtkBox*)MapHBox, MapColor, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)MapHBox, MapButton, FALSE, FALSE, 0);
	
	gtk_container_add((GtkContainer*)MapLabelAlign, MapLabel);
	
	gtk_table_attach((GtkTable*)Table, MapLabelAlign, 0, 1, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, MapSep, 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, MapColorAlign, 2, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	
	++Row;
	//Host name color
	GtkWidget *HostColor = gtk_entry_new();
	gtk_entry_set_text((GtkEntry*)HostColor, Settings.Colors.Host);
	
	GtkWidget *HostHBox = gtk_hbox_new(FALSE, 2);
	gtk_widget_set_size_request(HostColor, 70, -1);
	
	GtkWidget *HostColorAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	gtk_container_add((GtkContainer*)HostColorAlign, HostHBox);

	GtkWidget *HostLabel = gtk_label_new("Color for the host");
	GtkWidget *HostLabelAlign = gtk_alignment_new(0.0, 0.5, 0.01, 0.01);
	GtkWidget *HostSep = gtk_vseparator_new();
	GtkWidget *HostButton = gtk_button_new_with_label("Save");
	
	g_signal_connect_swapped((GObject*)HostButton, "clicked", (GCallback)Settings_Color_SetHostColor, HostColor);
	gtk_box_pack_start((GtkBox*)HostHBox, HostColor, FALSE, FALSE, 0);
	gtk_box_pack_start((GtkBox*)HostHBox, HostButton, FALSE, FALSE, 0);
	
	gtk_container_add((GtkContainer*)HostLabelAlign, HostLabel);
	
	gtk_table_attach((GtkTable*)Table, HostLabelAlign, 0, 1, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, HostSep, 1, 2, Row, Row + 1, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, HostColorAlign, 2, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	++Row;
	
	//Close button
	gtk_table_attach((GtkTable*)Table, gtk_hseparator_new(), 0, 5, Row, Row + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	++Row;
	GtkWidget *CloseButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	GtkWidget *CloseButtonAlign = gtk_alignment_new(1.0, 1.0, 0.01, 0.01);
	
	gtk_container_add((GtkContainer*)CloseButtonAlign, CloseButton);
	g_signal_connect_swapped(G_OBJECT(CloseButton), "clicked", (GCallback)GTK_NukeWidget, Win);
	gtk_table_attach((GtkTable*)Table, CloseButtonAlign, 4, 5, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

	///Draw it all
	gtk_widget_show_all(Win);
	return;
}
//Empty the scrolled window so we can rebuild it.
void GUI_ClearGames(GtkWidget *ScrolledWindow)
{
	if (!ScrolledWindow) return;

	GTK_NukeContainerChildren((GtkContainer*)ScrolledWindow);
}

bool GUI_GetGameVersion(char *OutBuf, const size_t Capacity)
{
	char WZBinary[2048] = { 0 };

	if (*Settings.WZBinary)
	{
#ifdef WIN32
		SubStrings.Copy(WZBinary, Settings.WZBinary + (sizeof "file:///" - 1), sizeof WZBinary);
#else
		SubStrings.Copy(WZBinary, Settings.WZBinary + (sizeof "file://" - 1), sizeof WZBinary);
#endif
	}
	else if (!GUI_FindWZExecutable(WZBinary, sizeof WZBinary))
	{
		return false;
	}

	char CmdBuf[4096] = { 0 };

	char *TempBuf = calloc(sizeof WZBinary * 2, 1);
	
	SubStrings.Replace(WZBinary, TempBuf, sizeof WZBinary, "%20", " ");
#ifdef WIN32
	SubStrings.Replace(WZBinary, TempBuf, sizeof WZBinary, "/", "\\");
#endif

	free(TempBuf);
	
	snprintf(CmdBuf, sizeof CmdBuf, "\"%s\" --version > .wzv", WZBinary);
	
	system(CmdBuf);

	struct stat FileStat;

	memset(&FileStat, 0, sizeof FileStat);

	if (stat(".wzv", &FileStat) != 0) return false;

	
	FILE *Desc = fopen(".wzv", "rb");
	if (!Desc) return false;

	char *DirtyVersion = calloc(FileStat.st_size + 1, 1);

	fread(DirtyVersion, 1, FileStat.st_size, Desc);
	fclose(Desc);

	remove(".wzv");

	if (!*DirtyVersion)
	{
		free(DirtyVersion);
		return false;
	}

	const char *Ptr = strstr(DirtyVersion, "Version: ");

	if (!Ptr)
	{
		free(DirtyVersion);
		return false;
	}

	Ptr += sizeof "Version: " - 1;

	SubStrings.CopyUntilC(OutBuf, Capacity, &Ptr, "- ,", true);

	free(DirtyVersion);

	return true;
}
	

static void GUI_LaunchGame(const GameStruct *GS)
{ //Null specifies we want to host.
#ifndef WIN32

	char ExtraOptions[4096] = { '\0' };
	char WZBinary[1024];
	
    if (!GUI_FindWZExecutable(WZBinary, sizeof WZBinary) && !*Settings.WZBinary)
    {
		GUI_DrawLaunchFailure();
		return;
    }
	
	Settings_AppendOptionsToLaunch(ExtraOptions, sizeof ExtraOptions);
	
	pid_t PID = fork();
	
	if (PID == -1)
	{
		GUI_DrawLaunchFailure();
		return;
	}
	
	
	///Parent code
	if (PID != 0)
	{
		signal(SIGCHLD, SIG_IGN);
		return;
	}
	
	
	///Child code
	//Bigger than we'll ever need.
	char **Argv = calloc(64, sizeof(char*));
	
	unsigned Inc = 0;
	
	//THe binary and the host/join option need to be allocated by us.
	Argv[0] = calloc(256, 1);
	
	if (!GS->Internal_IsEmpty)
	{
		Argv[1] = calloc(256, 1);
		Argv[2] = calloc(256, 1);
	}
	
	char IPFormat[128] = "--join=";
	char PortFormat[128] = "--gameport=";
	
	if (GS->Internal_IsHost)
	{
		SubStrings.Copy(IPFormat, "--host", sizeof IPFormat);
		*PortFormat = '\0';
	}
	else if (GS->Internal_IsSpec)
	{
		snprintf(IPFormat, sizeof(IPFormat), "--spectate=%s", GS->NetSpecs.HostIP);		
		snprintf(PortFormat, sizeof(PortFormat), "--gameport=%hu", GS->HostPort);
	}
	else if (GS->Internal_IsEmpty)
	{
		*IPFormat = '\0';
		*PortFormat = '\0';
	}
	else
	{
		SubStrings.Cat(IPFormat, GS->NetSpecs.HostIP, sizeof IPFormat);
		
		snprintf(PortFormat, sizeof(PortFormat), "--gameport=%hu", GS->HostPort);
	}
	
	//Copy in the binary path.
	SubStrings.Copy(Argv[0], *Settings.WZBinary ? Settings.WZBinary + (sizeof "file://" - 1) : WZBinary, 256);
	
	if (!GS->Internal_IsEmpty)//Copy in the host or join parameter,
	{
		SubStrings.Copy(Argv[1], IPFormat, 256);
		SubStrings.Copy(Argv[2], PortFormat, 256);
	}
	
	char CWD[1024];
	GUI_GetBinaryCWD(*Argv, CWD, sizeof CWD);
	
	chdir(CWD);
	
	const char *Iter = ExtraOptions;

	char Temp[256];
	//Break ExtraOptions up into argv format.
	Inc = GS->Internal_IsEmpty ? 1 : (GS->Internal_IsHost ? 2 : 3);
	
	for (; SubStrings.CopyUntilC(Temp, sizeof Temp, &Iter, " ", TRUE); ++Inc)
	{
		Argv[Inc] = strdup(Temp);
	}
	///We don't bother freeing anything because we're going to exec() it away anyways
	
	//Do the exec
	execvp(*Argv, Argv);
	
	exit(1);

#else //WINDOWS CODE!!    
    char WZString[4096];
    
    if (!GUI_FindWZExecutable(WZString, sizeof WZString) && !*Settings.WZBinary)
    {
		GUI_DrawLaunchFailure();
		return;
    }
    
    if (*Settings.WZBinary)
    {
		SubStrings.Copy(WZString,  Settings.WZBinary + sizeof "file:///" - 1, sizeof WZString);
		char *TempBuf = malloc(sizeof WZString * 2);
		
		SubStrings.Replace(WZString, TempBuf, sizeof WZString, "%20", " ");
		SubStrings.Replace(WZString, TempBuf, sizeof WZString, "/", "\\");
		free(TempBuf);
	}
	
	char BinaryWD[1024];
	GUI_GetBinaryCWD(WZString, BinaryWD, sizeof BinaryWD);
	
	char CWD[1024];
	getcwd(CWD, sizeof CWD);
	
	char IPFormat[128] = "--join=";
	char PortFormat[128] = "--gameport=";
	
	if (GS->Internal_IsHost)
	{
		SubStrings.Copy(IPFormat, "--host", sizeof IPFormat);
		*PortFormat = '\0';
	}
	else if (GS->Internal_IsSpec)
	{
		snprintf(IPFormat, sizeof(IPFormat), "--spectate=%s", GS->NetSpecs.HostIP);		
		snprintf(PortFormat, sizeof(PortFormat), "--gameport=%hu", GS->HostPort);
	}
	else if (GS->Internal_IsEmpty)
	{
		*IPFormat = '\0';
		*PortFormat = '\0';
	}
	else
	{
		SubStrings.Cat(IPFormat, GS->NetSpecs.HostIP, sizeof IPFormat);
		
		snprintf(PortFormat, sizeof(PortFormat), "--gameport=%hu", GS->HostPort);
	}
	
	SubStrings.Cat(WZString, " ", sizeof WZString);
	
	if (*IPFormat)
	{
		SubStrings.Cat(WZString, IPFormat, sizeof WZString);
		SubStrings.Cat(WZString, " ", sizeof WZString);
	}
	
	if (*PortFormat)
	{
		SubStrings.Cat(WZString, PortFormat, sizeof WZString);
		SubStrings.Cat(WZString, " ", sizeof WZString);
	}

	Settings_AppendOptionsToLaunch(WZString, sizeof WZString);
	
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    
    memset(&StartupInfo, 0, sizeof StartupInfo);
    memset(&ProcessInfo, 0, sizeof ProcessInfo);
    
    StartupInfo.cb = sizeof StartupInfo;
	//Launch it
	
	chdir(BinaryWD);
	
	const gboolean Worked = CreateProcess(NULL, WZString, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
	
	chdir(CWD);
	
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
	
	gtk_window_set_resizable((GtkWindow*)Win, FALSE);
	
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
#ifndef WIN32
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
			return TRUE;
		}
	} while ((Worker = strchr(Worker, ':')) != NULL);
	
	*Out = '\0';
	
	return FALSE;
#else
	const char *ProgDir = getenv("programfiles");

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
				return TRUE;
			}
		}
	}
	
	closedir(Folder);
	*Out = '\0';
	return FALSE;

#endif //WIN
}

static void GTK_NukeContainerChildren(GtkContainer *Container)
{
	GList *Children = gtk_container_get_children(Container);
	GList *Worker = Children;
	
	if (!GTK_IS_CONTAINER(Container))
	{
		return;
	}
	
	if (GTK_IS_FILE_CHOOSER_BUTTON(Container))
	{
		gtk_widget_destroy((GtkWidget*)Container);
		return;
	}
	
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

void GUI_ConnErr(GtkWidget *ScrolledWindow)
{
	GtkWidget *VBox = gtk_vbox_new(FALSE, 1); //Because it wants a container.
	
	GtkWidget *Label = gtk_label_new("Connection error. Will retry at next interval.");
	gtk_box_pack_start(GTK_BOX(VBox), Label, FALSE, FALSE, 20);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ScrolledWindow), VBox);
	gtk_widget_show_all(ScrolledWindow);
}

void GUI_NoGames(GtkWidget *ScrolledWindow)
{
	GtkWidget *VBox = gtk_vbox_new(FALSE, 1); //Because it wants a container.
	
	GtkWidget *Label = gtk_label_new("No games available.");
	gtk_box_pack_start(GTK_BOX(VBox), Label, FALSE, FALSE, 20);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ScrolledWindow), VBox);
	gtk_widget_show_all(ScrolledWindow);
}

static void SpecLaunch(GameStruct *GS)
{
	GS->Internal_IsSpec = true;
	GUI_LaunchGame(GS);
}
static void JoinLaunch(GameStruct *GS)
{
	GS->Internal_IsSpec = false;
	GUI_LaunchGame(GS);
}
void GUI_RenderGames(GtkWidget *ScrolledWindow, GameStruct *GamesList, uint32_t GamesAvailable)
{
	static GameStruct *FreeAfterRender[64];
	
	//We want it static so that it persists when this function returns, but we still need to wipe it each call.
	for (int Inc = 0; Inc < 64 && FreeAfterRender[Inc] != NULL; ++Inc)
	{
		free(FreeAfterRender[Inc]);
	}
	
	memset(FreeAfterRender, 0, sizeof FreeAfterRender);
	
	if (!ScrolledWindow || !GamesList || !GamesAvailable) return;
	
	GtkWidget *VBox = gtk_vbox_new(FALSE, (GamesAvailable * 2) - 1);
	
	unsigned Inc = 0;
	char OutBuf[2048];
	GdkPixbuf *IconBuf = NULL;
	gtk_box_pack_start(GTK_BOX(VBox), gtk_hseparator_new(), FALSE, FALSE, 0);
	
	for (; Inc < GamesAvailable; ++Inc)
	{
		char ModString[384] = { '\0' };
		const gboolean MapMod = GamesList[Inc].MapMod;
		
		if (GamesList[Inc].NetSpecs.CurPlayers >= GamesList[Inc].NetSpecs.MaxPlayers)
		{ /*Game is full.*/
			IconBuf = GuiInfo.Icon_Full_Pixbuf;
		}
		else if (GamesList[Inc].PrivateGame)
		{ /*Private game.*/
			IconBuf = GuiInfo.Icon_Locked_Pixbuf;
		}
		else if (*GamesList[Inc].ModList != '\0')
		{ /*It has mods.*/
			IconBuf = GuiInfo.Icon_Mod_Pixbuf;
		}
		else
		{ /*Normal, joinable game.*/
			IconBuf = GuiInfo.Icon_Open_Pixbuf;
		}
		
		/*Add mod warning even if we don't get the color for that.*/
		if (GamesList[Inc].ModList[0] != '\0')
		{
			snprintf(ModString, sizeof ModString, " <b><span foreground=\"red\">(mods: %s)</span></b>",  GamesList[Inc].ModList);
		}
		
		
		snprintf(OutBuf, sizeof OutBuf,
				"Name: <b><span foreground=\"%s\">%s</span></b> | "
				"Map: <b><span foreground=\"%s\">%s</span></b>%s | "
				"Host: <b><span foreground=\"%s\">%s</span></b>\n"
				"Players: <b>%d/%d</b> %s| IP: <b>%s:%hu</b> | Version: <b>%s</b>%s", Settings.Colors.Name,
				GamesList[Inc].GameName, Settings.Colors.Map, GamesList[Inc].Map, MapMod ? " <b><span foreground=\"red\">(map-mod)</span></b>" : "",
				Settings.Colors.Host, GamesList[Inc].HostNick, GamesList[Inc].NetSpecs.CurPlayers, GamesList[Inc].NetSpecs.MaxPlayers,
				GamesList[Inc].PrivateGame ? "<b><span foreground=\"orange\">(private)</span></b> " : "", GamesList[Inc].NetSpecs.HostIP,
				GamesList[Inc].HostPort, GamesList[Inc].VersionString, ModString);
				
		GtkWidget *Icon = gtk_image_new_from_pixbuf(IconBuf);
		
		GtkWidget *Label = gtk_label_new("");
		gtk_label_set_selectable((GtkLabel*)Label, true);
		gtk_label_set_markup((GtkLabel*)Label, OutBuf);
		
		GtkWidget *Button = gtk_button_new_with_label("Join");
		GtkWidget *SpecButton = gtk_button_new_with_label("Watch");
		
		gtk_widget_set_tooltip_text(SpecButton, "Spectate this game");
		gtk_widget_set_tooltip_text(Button, "Join this game");
		
		//Add a temporary pointer to our temporary pointer list
		void *Ptr = NULL;
		for (guint Inc = 0; Inc < 64; ++Inc)
		{
			if (FreeAfterRender[Inc] == NULL)
			{
				Ptr = malloc(sizeof(GameStruct));
				
				memcpy(Ptr, &GamesList[Inc], sizeof(GameStruct));
				
				FreeAfterRender[Inc] = Ptr;
				break;
			}
		}
		
		g_signal_connect_swapped(G_OBJECT(Button), "clicked", (GCallback)JoinLaunch, Ptr);
		g_signal_connect_swapped(G_OBJECT(SpecButton), "clicked", (GCallback)SpecLaunch, Ptr);
		
		if (GamesList[Inc].NetSpecs.CurPlayers == GamesList[Inc].NetSpecs.MaxPlayers)
		{
			gtk_widget_set_sensitive(Button, FALSE);
			gtk_button_set_label((GtkButton*)Button, "Game Full");
		}
		
		GtkWidget *HBox = gtk_hbox_new(FALSE, 4);
		GtkWidget *HBox2 = gtk_hbox_new(FALSE, 2);
		
		
		GtkWidget *VSep = gtk_vseparator_new(), *VSep2 = gtk_vseparator_new();
		
		gtk_box_pack_start(GTK_BOX(HBox2), Button, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox2), SpecButton, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox2), VSep2, FALSE, FALSE, 0);
		
		gtk_box_pack_start(GTK_BOX(HBox), Icon, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), VSep, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), HBox2, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(HBox), Label, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(VBox), HBox, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(VBox), gtk_hseparator_new(), FALSE, FALSE, 0);

	}
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ScrolledWindow), VBox);
	
	gtk_widget_show_all(ScrolledWindow);
}
