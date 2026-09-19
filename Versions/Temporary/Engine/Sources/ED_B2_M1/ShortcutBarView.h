#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <functional>
#include <string>

class CPaletteList;
class CWnd;

// The shortcut bar, behind a boundary that names no toolkit.
//
// The map, building and squad editors each dock one: a column of bars --
// Terrain, Objects, Gameplay, Script for the map editor -- of which one is open
// at a time, the open one showing a row of tabs over a palette per tab. They
// held a CDefaultShortcutBar by value and built it through Stingray's calls; this
// is those calls, and nothing reads the bar back afterwards but the commands it
// sends.
//
// A palette is still made by its own Create function, which takes the MFC tab
// window it is made in and registers itself in that window's list. The wx bar
// runs the same functions: it gives each of its bars a CDefault3DTabWindow that
// never becomes a window, as the list, and while a palette is made it points
// CWxHostWindow at the notebook page the palette goes in. Both bars exist only
// under the flag that makes the palettes' Create pick their wx half, so a wx bar
// always gets wx palettes.
namespace NShortcutBar
{
	// Makes one palette in a tab window, registers it in the window's list, and
	// returns it: the shape of every palette's Create.
	typedef std::function<IWidget*( CPaletteList*, IWidget* )> TPaletteFactory;

	class IView
	{
	public:
		virtual ~IView() {}

		// Makes the bar in pPane, the editor's docking pane. nControlID is the
		// Stingray bar's child id.
		virtual bool Create( IWidget *pPane, unsigned nControlID ) = 0;
		// Takes the bar down, palettes and all.
		virtual void Destroy() = 0;
		// What the pane is given as its contents.
		virtual IWidget* GetWidget() = 0;
		virtual void Show( bool bShow ) = 0;

		// A new bar, not yet in the column; its index. A tab handler, when one
		// is named, hears this bar's tab changes on its own, as
		// CDefault3DTabWindow::SetCommandHandlerID -- the building editor's
		// point lists use it.
		virtual int BeginBar( unsigned nTabCommandHandlerID, unsigned nTabCommandID ) = 0;
		// A tab on the bar, with the palette rFactory makes. False when the
		// factory made none, and no tab is added.
		virtual bool AddTab( int nBar, const std::string &rszLabel, const TPaletteFactory &rFactory ) = 0;
		virtual void ActivateTab( int nBar, int nTab ) = 0;
		// Puts the bar in the column under rszLabel.
		virtual void EndBar( int nBar, const std::string &rszLabel ) = 0;

		// Opens a bar.
		virtual void SelectBar( int nBar ) = 0;
		// Where a bar being opened, and a tab changed in the open bar, are
		// reported: MAKELONG( tab, bar ), when the command is enabled.
		virtual void SetCommandHandlerID( unsigned nCommandHandlerID, unsigned nCommandID ) = 0;
	};


	// Owned by the caller.
	IView* Create();
}
