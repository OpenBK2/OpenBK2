#include "../Common_RTS_AI/AIObjectBase.h"
#include "../Common_RTS_AI/AIAngle.h"
#define CStructureSaver IBinSaver
#include "ValidObjectCheck.h"
#include "AICellsTiles.h"
#include "AITiles.h"
#include "AIMap.h"
#include "AITimer.h"
#include "../Common_RTS_AI/ChecksumSaver.h"
using namespace NDb;

#ifndef _FINALRELEASE
#define CONSOLE_BUFFER_LOG(n,s)	Singleton<IConsoleBuffer>()->WriteASCII( n, s )
#define CONSOLE_BUFFER_LOG1(n,s,c)	Singleton<IConsoleBuffer>()->WriteASCII( n, s, c )
#define CONSOLE_BUFFER_LOG2(n,s,c,b)	Singleton<IConsoleBuffer>()->WriteASCII( n, s, c, b )
#else
#define CONSOLE_BUFFER_LOG(n,s) ;
#define CONSOLE_BUFFER_LOG1(n,s,c) ;
#define CONSOLE_BUFFER_LOG2(n,s,c,b) ;
#endif

//DEBUG{ 
#ifndef _FINALRELEASE
void DrawWhiteCross( const CVec3 &vPoint );
#define DRAW_WHITE_CROSS(a) DrawWhiteCross(a)
void DrawWhiteCross( const CVec3 &vPoint );
#else
#define DRAW_WHITE_CROSS( a ) ;
#endif 
//DEBUG}

interface IAIScenarioTracker *GetScenarioTracker();


