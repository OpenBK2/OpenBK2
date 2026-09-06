#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

// What the start-camera-positions palette holds, and how the editor asks for
// it. Both are toolkit-neutral and both used to live in CameraPositionWindow.h
// beside the MFC window; they are here so the wx palette can share them rather
// than restate them.

struct SCameraPositionWindowData
{
	int nPlayerIndex;
	int nPlayerCount;
	bool bAllParams;
	//
	SCameraPositionWindowData() :
		nPlayerIndex( -1 ),
		nPlayerCount( 0 ),
		bAllParams( false )
	{
	}
	//
	void Clear()
	{
		nPlayerIndex = -1;
		nPlayerCount = 0;
		bAllParams = false;
	}
};


// The two commands CMapInfoState drives the palette with -- read your controls
// into this struct, fill your controls from this struct -- and the dispatch
// that turns a command id and a uintptr_t back into a call.
//
// That dispatch is identical whichever toolkit draws the palette, so it is
// written once and both implementations derive from this. What differs between
// them is only how the controls are read and written, which is what the two
// pure virtuals are.
class CCameraPositionCommands : public ICommandHandler
{
public:
	virtual void GetDialogData( SCameraPositionWindowData *pData ) = 0;
	virtual void SetDialogData( const SCameraPositionWindowData *pData ) = 0;

	//	ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};
