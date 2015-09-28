/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2015.
See the included file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "wzblue.h"

#define CONFIGFILE "wzblueconfig.bin"
struct Settings Settings;

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
