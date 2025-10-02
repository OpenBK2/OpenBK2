#include "..\Main\GameTimer.h"
#define CStructureSaver IBinSaver
#ifndef _FINALRELEASE
#define CONSOLE_BUFFER_LOG(n,s)	Singleton<IConsoleBuffer>()->WriteASCII( n, s )
#define CONSOLE_BUFFER_LOG1(n,s,c)	Singleton<IConsoleBuffer>()->WriteASCII( n, s, c )
#define CONSOLE_BUFFER_LOG2(n,s,c,b)	Singleton<IConsoleBuffer>()->WriteASCII( n, s, c, b )
#else
#define CONSOLE_BUFFER_LOG(n,s) ;
#define CONSOLE_BUFFER_LOG1(n,s,c) ;
#define CONSOLE_BUFFER_LOG2(n,s,c,b) ;
#endif
