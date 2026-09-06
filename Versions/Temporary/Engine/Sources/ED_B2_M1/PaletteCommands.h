#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

// How every palette in this editor talks to the state behind it: two commands,
// "read your controls into this struct" and "fill your controls from this
// struct", arriving as a command id and a uintptr_t that is really a pointer to
// the palette's own data type.
//
// Each palette used to write that dispatch out itself -- the same switch, the
// same reinterpret_cast, the same two asserts, differing only in the struct
// named. There are fifteen of them. Written once here, a palette says what its
// data type is and implements the two halves that genuinely differ:
//
//     class CFormationWindow : public CResizeDialog,
//                              public CPaletteCommands<SFormationWindowDialogData>
//
// The point is not the twenty lines saved. It is that a palette moving to wx
// has two implementations of it for a while, and the dispatch is exactly the
// kind of thing that would be copied into the second one and then drift.
template<class TDialogData>
class CPaletteCommands : public ICommandHandler
{
public:
	virtual void GetDialogData( TDialogData *pData ) = 0;
	virtual void SetDialogData( const TDialogData *pData ) = 0;

	//	ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
	{
		TDialogData *pData = reinterpret_cast<TDialogData*>( dwData );
		// The unit start commands palette guarded this and the others did not,
		// which is the sort of difference that survives only because nobody
		// compared them. Guarded for all of them now: a null here means the
		// caller passed no struct, and there is nothing to read or fill.
		//
		// Not NI_ASSERT: it does not evaluate its argument in this build, so it
		// would document the rule without enforcing it.
		if ( pData == 0 )
		{
			return false;
		}
		switch ( nCommandID )
		{
			case ID_WINDOW_GET_DIALOG_DATA:
				GetDialogData( pData );
				return true;
			//
			case ID_WINDOW_SET_DIALOG_DATA:
				SetDialogData( pData );
				return true;
		}
		return false;
	}

	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
	{
		NI_ASSERT( pbEnable != 0, "CPaletteCommands::UpdateCommand(), pbEnable == 0" );
		NI_ASSERT( pbCheck != 0, "CPaletteCommands::UpdateCommand(), pbCheck == 0" );
		//
		switch ( nCommandID )
		{
		case ID_WINDOW_SET_DIALOG_DATA:
		case ID_WINDOW_GET_DIALOG_DATA:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
		}
	}
};
