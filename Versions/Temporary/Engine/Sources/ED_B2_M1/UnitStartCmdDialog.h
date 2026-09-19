#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The unit start command editor, behind a boundary that names no toolkit.
//
// This one is **modeless**, which is what makes it a different shape from every
// dialog migrated so far. It is created once when the palette's tab is entered,
// kept hidden, shown by Add Command and Edit Command, and hidden again when it
// is finished with -- so there is no Run that blocks and answers. It reports
// what the user did through IListener instead, which is the state, and the
// state reads and writes it through IDialog in between.
//
// That is also why the window stays alive across uses: CUnitStartCmdState holds
// the one instance for the life of the tab, and the dialog remembers the size
// and position it was left at, in Editor/ResizeDialogStyles/CEdUnitStartCmd.xml.
namespace NUnitStartCmdDialog
{
	// What the dialog shows, and what it answers with. Was
	// CEdUnitStartCmd::SDlgData, which is the same fields: the state fills one
	// in to open the dialog and reads one back when OK is pressed.
	struct SData
	{
		// true for a new command, false when an existing one is being edited.
		// It decides the title and is handed back untouched.
		bool bEditMode;
		int nSelectedCmdType;
		bool bSelectedCmdNeedTargetUnit;
		int nData;
		std::string szTarget;
		int nCommandIndex;

		SData()
		{
			Clear();
		}

		void Clear()
		{
			bEditMode = true;
			nSelectedCmdType = -1;
			nData = 0;
			szTarget = "";
			bSelectedCmdNeedTargetUnit = false;
			nCommandIndex = -1;
		}
	};


	// What the dialog tells the state. The two buttons hide the window as well;
	// Clear and the type change leave it up.
	enum EEvent
	{
		EV_OK,
		EV_CANCEL,
		EV_CLEAR,
		EV_TYPE_CHANGE,
	};


	// Implemented by the state. The dialog holds a borrowed pointer to it and
	// outlives nothing: the state owns the dialog and is what destroys it.
	class IListener
	{
	public:
		virtual ~IListener() {}
		virtual void OnUnitStartCmdDialogEvent( EEvent eEvent ) = 0;
	};


	// The window itself, as much of it as the state uses.
	class IDialog
	{
	public:
		virtual ~IDialog() {}

		// Show( false ) hides it, as both buttons do for themselves. The state
		// also hides it when it resets, which is how a tab change closes it.
		virtual void Show( bool bShow ) = 0;

		// Fill it in, and read it back. SetDialogData also decides the title and
		// which controls are enabled; see the implementations for the rules,
		// which are the MFC dialog's and are kept.
		virtual void SetDialogData( const SData *pData ) = 0;
		virtual void GetDialogData( SData *pData ) = 0;

		// The target unit box, which the state writes as the user picks units on
		// the map. It is read-only in the dialog itself.
		virtual void UpdateTarget( const std::string &rszTarget ) = 0;

		// The command type currently chosen, as its value rather than its index;
		// -1 for <UNKNOWN>. The state asks while the dialog is up, to know
		// whether what is being edited wants a target unit.
		virtual int GetSelectedCommandType() = 0;
	};


	// Makes the window, hidden, owned by pParent's frame. The caller owns the
	// result and destroys it with delete, which destroys the window with it.
	// Null if the window could not be created.
	IDialog* Create( IWidget *pParent, IListener *pListener );

	// Named so the dispatcher can reach them; not for anything else to call.
	IDialog* CreateMfc( IWidget *pParent, IListener *pListener );
	IDialog* CreateWx( IWidget *pParent, IListener *pListener );
}
