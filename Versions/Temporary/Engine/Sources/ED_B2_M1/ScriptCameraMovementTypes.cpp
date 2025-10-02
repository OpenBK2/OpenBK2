#include "stdafx.h"
#include "ScriptCameraMovementTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CScriptCameraRunTypeMnemonics::CScriptCameraRunTypeMnemonics() : 
CMnemonicsCollector<int>( NDb::SCRT_DIRECT_MOVE, "SCRT_DIRECT_MOVE" )
{
	Insert( NDb::SCRT_DIRECT_MOVE , "SCRT_DIRECT_MOVE" );
	Insert( NDb::SCRT_DIRECT_ROTATE , "SCRT_DIRECT_ROTATE" );
	Insert( NDb::SCRT_DIRECT_FOLLOW , "SCRT_DIRECT_FOLLOW" );
	Insert( NDb::SCRT_SPLINE , "SCRT_SPLINE" );
}

CScriptCameraRunTypeMnemonics typeScriptCameraRunTypeMnemonics;

