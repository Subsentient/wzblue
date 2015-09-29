/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "wzblue.h"
#include "substrings/substrings.h"

#define CONFIGFILE "wzblueconfig.bin"

//Globals
struct Settings Settings;
enum SettingsChoice DefaultChoices[] = { CHOICE_UNSPECIFIED, CHOICE_NO, CHOICE_YES };

//Prototypes
static int Settings_GetIndiceFromChoice(enum SettingsChoice Setting);

//Functions
gboolean Settings_ReadSettings(void)
{
	struct stat Stat;
	
	if (stat(CONFIGFILE, &Stat) != 0)
	{
		return FALSE;
	}
	
	FILE *Descriptor = fopen(CONFIGFILE, "rb");
	
	if (!Descriptor) return FALSE;
	
	fread(&Settings, 1, sizeof(struct Settings), Descriptor);
	fclose(Descriptor);
	
	return TRUE;
}

gboolean Settings_SaveSettings(void)
{
	FILE *Desc = fopen(CONFIGFILE, "wb");
	
	if (!Desc) return FALSE;
	
	fwrite(&Settings, 1, sizeof(struct Settings), Desc);
	fclose(Desc);
	
	return TRUE;
}

void Settings_SetBinary(GtkFileChooserButton *Button)
{
	strncpy(Settings.WZBinary, gtk_file_chooser_get_uri((GtkFileChooser*)Button), sizeof Settings.WZBinary - 1);
	Settings.WZBinary[sizeof Settings.WZBinary - 1] = '\0';
}

void Settings_SetSound(GtkRadioButton *Button, enum SettingsChoice *Choice)
{
	if (gtk_toggle_button_get_active((GtkToggleButton*)Button))
	{
		Settings.Sound = *Choice;
	}
}

void Settings_SetShadows(GtkRadioButton *Button, enum SettingsChoice *Choice)
{
	if (gtk_toggle_button_get_active((GtkToggleButton*)Button))
	{
		Settings.Shadows = *Choice;
	}
}

void Settings_SetTextureCompress(GtkRadioButton *Button, enum SettingsChoice *Choice)
{
	if (gtk_toggle_button_get_active((GtkToggleButton*)Button))
	{
		Settings.TextureCompression = *Choice;
	}
}

void Settings_SetShaders(GtkRadioButton *Button, enum SettingsChoice *Choice)
{
	if (gtk_toggle_button_get_active((GtkToggleButton*)Button))
	{
		Settings.Shaders = *Choice;
	}
}

void Settings_SetVBOS(GtkRadioButton *Button, enum SettingsChoice *Choice)
{
	if (gtk_toggle_button_get_active((GtkToggleButton*)Button))
	{
		Settings.VBOS = *Choice;
	}
}

static int Settings_GetIndiceFromChoice(enum SettingsChoice Setting)
{ //Gets the value of *Setting, and enumerates it properly.
	switch (Setting)
	{
		case CHOICE_UNSPECIFIED:
			return 0;
		case CHOICE_NO:
			return 1;
		case CHOICE_YES:
			return 2;
	}
	
	return 0; //If all else fails we assume unspecified.
}

void Settings_RadioButtonInit(GtkWidget *RadioButtons[3], const enum SettingsChoice Setting)
{
	gtk_toggle_button_set_active((GtkToggleButton*)RadioButtons[Settings_GetIndiceFromChoice(Setting)], TRUE);
}

void Settings_AppendOptionsToLaunch(char *const Out, unsigned OutMax)
{
	switch (Settings.Sound)
	{
		case CHOICE_NO:
		{
			SubStrings.Cat(Out, "--nosound ", OutMax);
			break;
		}
		case CHOICE_YES:
		{
			SubStrings.Cat(Out, "--sound ", OutMax);
			break;
		}
		default:
			break;
	}
	
	switch (Settings.Shadows)
	{
		case CHOICE_NO:
		{
			SubStrings.Cat(Out, "--noshadows ", OutMax);
			break;
		}
		case CHOICE_YES:
		{
			SubStrings.Cat(Out, "--shadows ", OutMax);
			break;
		}
		default:
			break;
	}
	
	switch (Settings.Fullscreen)
	{
		case CHOICE_NO:
		{
			SubStrings.Cat(Out, "--window ", OutMax);
			break;
		}
		case CHOICE_YES:
		{
			SubStrings.Cat(Out, "--fullscreen ", OutMax);
			break;
		}
		default:
			break;
	}
	
	switch (Settings.TextureCompression)
	{
		case CHOICE_NO:
		{
			SubStrings.Cat(Out, "--texturecompression ", OutMax);
			break;
		}
		case CHOICE_YES:
		{
			SubStrings.Cat(Out, "--notexturecompression ", OutMax);
			break;
		}
		default:
			break;
	}
	
	if (Settings.Shaders == CHOICE_NO)
	{
		SubStrings.Cat(Out, "--noshaders ", OutMax);
	}
	
	if (Settings.VBOS == CHOICE_NO)
	{
		SubStrings.Cat(Out, "--novbos ", OutMax);
	}
	
	if (Settings.Resolution.Width && Settings.Resolution.Height)
	{
		char Append[256];
		
		snprintf(Append, sizeof Append, "--resolution=%ux%u ", Settings.Resolution.Width, Settings.Resolution.Height);
		SubStrings.Cat(Out, Append, OutMax);
	}
	
}
