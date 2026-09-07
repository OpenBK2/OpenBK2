#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

// How a palette in this editor talks to the state behind it: two commands,
// "read your controls into this struct" and "fill your controls from this
// struct", arriving as a command id and a uintptr_t that is really a pointer to
// the palette's own data type.
//
// There are two families of that, not one, and they differ only in which pair
// of command ids they answer to and what the struct is. CPaletteCommands is the
// dialog-data family; CEditParameterCommands below is the edit-parameters one.
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


// The same thing for the other family. Five palettes -- field, heights, map
// objects, models and VSO -- do not hand over a whole dialog struct. They
// exchange an SEditParameters whose nFlags says which of its fields this
// particular exchange is about, under ID_GET_EDIT_PARAMETERS and
// ID_SET_EDIT_PARAMETERS. Everything else is the same switch over a uintptr_t
// that is really a pointer, written out five times.
//
// Two differences from CPaletteCommands, both of them the family's and not a
// choice made here. The halves answer bool rather than void, because the state
// asks them whether the exchange happened. And SetEditParameters takes a
// reference rather than a pointer, which is what all five already declare.
//
// Three of the five answer commands of their own as well. HandleCommand and
// UpdateCommand stay virtual for them: a palette with more to say overrides
// them and falls through to here for the pair it does not handle itself.
template<class TEditParameters>
class CEditParameterCommands : public ICommandHandler
{
public:
	virtual bool GetEditParameters( TEditParameters *pEditParameters ) = 0;
	virtual bool SetEditParameters( const TEditParameters &rEditParameters ) = 0;

	//	ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
	{
		// Every one of the five checked this before dereferencing, and rightly:
		// a null here means the caller passed no struct, and there is nothing to
		// read or fill. Not NI_ASSERT, which does not evaluate its argument in
		// this build and so would document the rule without enforcing it.
		if ( dwData == 0 )
		{
			return false;
		}
		switch ( nCommandID )
		{
			case ID_GET_EDIT_PARAMETERS:
				return GetEditParameters( reinterpret_cast<TEditParameters*>( dwData ) );
			//
			case ID_SET_EDIT_PARAMETERS:
				return SetEditParameters( *reinterpret_cast<const TEditParameters*>( dwData ) );
		}
		return false;
	}

	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
	{
		NI_ASSERT( pbEnable != 0, "CEditParameterCommands::UpdateCommand(), pbEnable == 0" );
		NI_ASSERT( pbCheck != 0, "CEditParameterCommands::UpdateCommand(), pbCheck == 0" );
		//
		switch ( nCommandID )
		{
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
		}
	}
};
