#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "Misc/Geom.h"

#include <string>

struct IManipulator;
class CVariant;

// Selection Properties -- the property grid in its docking pane -- behind a
// boundary that names no toolkit.
//
// CDWPropertyBrowser is the pane, and stays MFC. What is inside it used to be a
// CPCDialog held by value: a CPCMainTreeControl, the Stingray tree that lists an
// object's fields with an inline editor over the value cell, and a status line.
// Nothing outside the pane talks to that dialog directly. Every editor state
// reaches it through the command handler registered as CHID_PC_DIALOG, with
// four commands -- ID_PC_DIALOG_GET_VIEW, GET_COMMAND_HANDLER, CREATE_TREE and
// UPDATE_VALUES -- and then through the IView it hands back. So an
// implementation is whatever registers as CHID_PC_DIALOG and answers those.
//
// The wx one is a wxPropertyGrid, and is being built in slices: this first one
// shows the tree and does not edit it.
struct IPropertyPane
{
	virtual ~IPropertyPane() {}

	// Builds the contents inside the pane and registers them as CHID_PC_DIALOG.
	// rszOptionsLabel names the Editor/ResizeDialogStyles file the column widths
	// are kept in. pParentPane is used only during this call.
	virtual bool Create( IWidget *pParentPane, const std::string &rszOptionsLabel ) = 0;
	virtual bool IsCreated() const = 0;

	// Position within the pane, in the pane's client coordinates.
	virtual void SetBounds( const CTRect<int> &rBounds ) = 0;
	virtual void Show( bool bShow ) = 0;

	// The editor's read-only mode: everything grey, nothing editable.
	virtual void EnableEdit( bool bEnable ) = 0;
};


namespace NPropertyPane
{
	// Owned by the caller. Chosen by OBK2_WX_DIALOGS, like every migrated view.
	IPropertyPane* Create();

	// Named so the dispatcher can reach them; not for anything else to call.
	IPropertyPane* CreateMfc();
#ifdef OBK2_WITH_WX
	IPropertyPane* CreateWx();
#endif


	// ---- the part that is not drawing ----
	//
	// Property names are paths: "Weather.WindForce", "Players.[0].Name". These
	// answer for a name, from the manipulator, what CPCMainTreeControl answered
	// for a tree item by walking up from it.

	// "Weather.WindForce" -> "Weather"; empty at the top.
	std::string ParentName( const std::string &rszName );

	// The value the grid shows. A vec3_color is three float fields shown as one
	// colour, which is why this is not simply the manipulator's GetValue.
	bool GetValue( IManipulator *pManipulator, const std::string &rszName, CVariant *pValue );

	// The value column's text, formatted as the property's editor formats it.
	// False for a node, which has no value of its own, and for a name the
	// manipulator does not know.
	bool GetValueText( IManipulator *pManipulator, const std::string &rszName, std::string *pszText );

	// Read-only when the property, or any property above it, is marked so.
	bool IsReadOnly( IManipulator *pManipulator, const std::string &rszName );
}
