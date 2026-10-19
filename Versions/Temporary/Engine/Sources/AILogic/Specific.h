#include "Common_RTS_AI/AIAngle.h"
#include "Common_RTS_AI/AIObjectBase.h"
#define CStructureSaver IBinSaver
#include "AICellsTiles.h"
#include "AIMap.h"
#include "AITiles.h"
#include "AITimer.h"
#include "Common_RTS_AI/ChecksumSaver.h"
#include "ValidObjectCheck.h"
// This file is AILogic's prelude and nothing outside AILogic includes it, so
// the namespace it opens does not reach another module, which is the leak
// -Wheader-hygiene exists to report. Every unit in this project names the
// database records unqualified, and dropping the directive and compiling all
// 213 units textually measures what that is worth: 1,928 sites, 283 of them in
// AILogic's own headers, over only 73 distinct names, of which twelve carry 88
// per cent of the references. So the cheaper shape of this is 73
// using-declarations rather than 1,928 qualifications, and either way it is a
// mechanical pass over lockstep simulation code that wants an oracle stronger
// than "it still compiles". That has not been done, so the directive stays for
// now. The suppression is kept to this one directive so the flag stays on for
// the rest of the tree; see cmake/warnings.cmake. GCC has no diagnostic of
// this class, so the pragma is clang's and sits behind __clang__ rather than
// being spelled for both.
#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
#endif
using namespace NDb;
#if defined( __clang__ )
#pragma clang diagnostic pop
#endif

#ifndef _FINALRELEASE
#define CONSOLE_BUFFER_LOG( n, s ) Singleton<IConsoleBuffer>()->WriteASCII( n, s )
#define CONSOLE_BUFFER_LOG1( n, s, c ) Singleton<IConsoleBuffer>()->WriteASCII( n, s, c )
#define CONSOLE_BUFFER_LOG2( n, s, c, b ) Singleton<IConsoleBuffer>()->WriteASCII( n, s, c, b )
#else
#define CONSOLE_BUFFER_LOG( n, s ) ;
#define CONSOLE_BUFFER_LOG1( n, s, c ) ;
#define CONSOLE_BUFFER_LOG2( n, s, c, b ) ;
#endif

//DEBUG{
#ifndef _FINALRELEASE
void DrawWhiteCross( const CVec3 &vPoint );
#define DRAW_WHITE_CROSS( a ) DrawWhiteCross( a )
void DrawWhiteCross( const CVec3 &vPoint );
#else
#define DRAW_WHITE_CROSS( a ) ;
#endif
//DEBUG}

struct IAIScenarioTracker *GetScenarioTracker();
