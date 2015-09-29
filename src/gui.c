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
#include "icons.h"

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
static void GUI_LoadIcons(void);
static void GUI_LaunchGame(const char *IP);
static void GUI_DrawLaunchFailure(void);
static void GUI_DrawSettingsDialog(void);

static gboolean GUI_FindWZExecutable(char *const Out, unsigned OutMaxSize);


static void GTK_Destroy(GtkWidget *Widget, gpointer Stuff)
{
	Settings_SaveSettings();
	gtk_main_quit();
	exit(0);
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
	GtkWidget *Item_Preferences = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, AccelGroup);
	
	//Set up accelerator for Refresh (F5)
	gtk_widget_add_accelerator(Item_Refresh, "activate", AccelGroup, 0xffc2 ,0, GTK_ACCEL_VISIBLE);
	
	g_signal_connect(G_OBJECT(Item_About), "activate", (GCallback)GUI_DrawAboutDialog, NULL);
	g_signal_connect(G_OBJECT(Item_Preferences), "activate", (GCallback)GUI_DrawSettingsDialog, NULL);
	g_signal_connect(G_OBJECT(Item_Quit), "activate", (GCallback)GTK_Destroy, NULL);
	
	g_signal_connect_swapped(G_OBJECT(Item_Refresh), "activate", (GCallback)Main_LoopFunc, &False);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(HelpMenu), Item_About);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Refresh);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Preferences);
	gtk_menu_shell_append(GTK_MENU_SHELL(FileMenu), Item_Quit);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), FileTag);
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuBar), HelpTag);
	
	gtk_container_add((GtkContainer*)Align, MenuBar);
	gtk_table_attach(GTK_TABLE(GuiInfo.Table), Align, 0, 6, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);

	
	//gtk_widget_set_size_request(MenuBar, -1, 20);
	
	gtk_window_add_accel_group((GtkWindow*)GuiInfo.Win, AccelGroup);
	
	gtk_widget_show_all(GuiInfo.Table);
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
	
	gtk_container_add((GtkContainer*)Align, HBox);
	
	g_signal_connect_swapped(G_OBJECT(Button1), "clicked", (GCallback)Main_LoopFunc, &False);
	g_signal_connect_swapped(G_OBJECT(Button2), "clicked", (GCallback)GUI_LaunchGame, NULL);
	
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
	
	gtk_table_attach((GtkTable*)Table, Label, 0, 5, 0, 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, gtk_hseparator_new(), 0, 5, 1, 2, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	
	///Options
	//Choose Warzone binary
	GtkWidget *BinaryChooserLabel = gtk_label_new("Warzone 2100 binary");
	GtkWidget *BinaryChooser = gtk_file_chooser_button_new("Select Warzone 2100 binary", GTK_FILE_CHOOSER_ACTION_OPEN);
	
	char WZBinary[1024] = "file://";
	const gboolean AutoDetectedBinary = GUI_FindWZExecutable(WZBinary + sizeof "file://" - 1, sizeof WZBinary - (sizeof "file://" - 1));
	
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
	
	gtk_table_attach((GtkTable*)Table, BinaryChooserLabelAlign, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach((GtkTable*)Table, BinaryChooserSep, 1, 2, 2, 3, GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)Table, BinaryChooser, 2, 5, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	
	//Radio buttons
	GtkWidget *RadioButtons[3] = { NULL };
	const char *const RadioLabels[3] = { "Unspecified", "No", "Yes" };
	unsigned Row = 3;
	
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

	//Close button
	GtkWidget *CloseButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	GtkWidget *CloseButtonAlign = gtk_alignment_new(1.0, 1.0, 0.01, 0.01);
	
	gtk_container_add((GtkContainer*)CloseButtonAlign, CloseButton);
	g_signal_connect_swapped(G_OBJECT(CloseButton), "clicked", (GCallback)GTK_NukeWidget, Win);
	gtk_table_attach((GtkTable*)Table, CloseButtonAlign, 4, 5, Row, Row + 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

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

static void GUI_LaunchGame(const char *IP)
{ //Null specifies we want to host.
#ifndef WIN

	char WZBinary[2048];
    
    if (!GUI_FindWZExecutable(WZBinary, sizeof WZBinary) && !*Settings.WZBinary)
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
	
	execlp(*Settings.WZBinary ? Settings.WZBinary + (sizeof "file://" - 1) : WZBinary, "warzone2100", IPFormat, NULL);
	
	exit(1);

#else //WINDOWS CODE!!    
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
	
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    
    memset(&StartupInfo, 0, sizeof StartupInfo);
    memset(&ProcessInfo, 0, sizeof ProcessInfo);
    
    StartupInfo.cb = sizeof StartupInfo;
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
			return TRUE;
		}
	} while ((Worker = strchr(Worker, ':')) != NULL);
	
	*Out = '\0';
	
	return FALSE;
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

void GUI_NoGames(GtkWidget *ScrolledWindow)
{
	GtkWidget *VBox = gtk_vbox_new(FALSE, 1); //Because it wants a container.
	
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
		
		
		snprintf(OutBuf, sizeof OutBuf, "Name: <b><span foreground=\"#009bff\">%s</span></b> | "
									"Map: <b><span foreground=\"#00bb00\">%s</span></b>%s | "
									"Host: <b><span foreground=\"#ff8300\">%s</span></b>\n"
									"Players: <b>%d/%d</b> %s| IP: <b>%s</b> | Version: <b>%s</b>%s",
				GamesList[Inc].GameName, GamesList[Inc].Map, MapMod ? " <b><span foreground=\"red\">(map-mod)</span></b>" : "",
				GamesList[Inc].HostNick, GamesList[Inc].NetSpecs.CurPlayers, GamesList[Inc].NetSpecs.MaxPlayers,
				GamesList[Inc].PrivateGame ? "<b><span foreground=\"orange\">(private)</span></b> " : "", GamesList[Inc].NetSpecs.HostIP,
				GamesList[Inc].VersionString, ModString);
				
		GtkWidget *Icon = gtk_image_new_from_pixbuf(IconBuf);
		
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
		
		GtkWidget *HBox = gtk_hbox_new(FALSE, 4);
		GtkWidget *HBox2 = gtk_hbox_new(FALSE, 2);
		
		
		GtkWidget *VSep = gtk_vseparator_new(), *VSep2 = gtk_vseparator_new();
		
		gtk_box_pack_start(GTK_BOX(HBox2), Button, FALSE, FALSE, 0);
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
