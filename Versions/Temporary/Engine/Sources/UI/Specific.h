#pragma once
#define CStructureSaver IBinSaver

#include "DBUserInterface.h"
#include "Input/GameMessage.h"
#include "Main/GameTimer.h"
#include "Misc/StrProc.h"
#include "SceneClassIDs.h"
#include "Sound/DBSound.h"
#include "UIFactory.h"

#include <algorithm>

#ifndef _FINALRELEASE
#define CONSOLE_BUFFER_LOG( n, s ) WriteToPipe( n, s )
#define CONSOLE_BUFFER_LOG1( n, s, c ) WriteToPipe( n, s, c )
#define CONSOLE_BUFFER_LOG2( n, s, c, b ) WriteToPipe( n, s, c, b )
#else
#define CONSOLE_BUFFER_LOG( n, s ) ;
#define CONSOLE_BUFFER_LOG1( n, s, c ) ;
#define CONSOLE_BUFFER_LOG2( n, s, c, b ) ;
#endif
