/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/

#include "wzblue.h"

int main(int argc, char **argv)
{
	WZ_GetGamesList("lobby.wz2100.net", 9990, false);
	return 0;
}
