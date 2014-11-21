/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain. By Subsentient, 2014.
*/

typedef signed char Bool;
enum { false, true };
typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, ENDCOLOR = 0 } ConsoleColor;

/*main.c*/
void WZBlue_SetTextColor(ConsoleColor Color);

/*netcore.c*/
Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
Bool Net_Disconnect(int SockDescriptor);
Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, Bool TextStream);
Bool Net_Disconnect(int SockDescriptor);
Bool Net_Write(int SockDescriptor, const char *InMsg);

/*lobbycomm.c*/
Bool WZ_GetGamesList(const char *Server, unsigned short Port, Bool WZLegacy);
