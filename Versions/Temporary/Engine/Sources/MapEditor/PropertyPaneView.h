#pragma once

#include "MapEditorLib/Interface_PCItemEditor.h"
#include "MapEditorLib/Interface_Widget.h"
#include "Misc/Geom.h"

#include <functional>
#include <string>
#include <vector>

class CDefaultView;
class CObjectBaseController;

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
// The wx one is a wxPropertyGrid, built in slices: the tree, then editing in
// place, then the buttons beside a value, which run NPropertyButton.
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
	// manipulator does not know. A long string is cut at its first line break
	// for the column; bMultiline keeps all of it, for the editor its button
	// opens.
	bool GetValueText( IManipulator *pManipulator, const std::string &rszName, std::string *pszText, bool bMultiline = false );

	// Read-only when the property, or any property above it, is marked so.
	bool IsReadOnly( IManipulator *pManipulator, const std::string &rszName );

	// The values a combo row offers, as its MFC editor lists them: the int and
	// float combos' "values:" ranges, a string combo's descriptor values, the
	// objects a combo ref may point at after "null", and true and false. False
	// for a type that is not a list, and for a list the MFC editor would have
	// refused to open.
	bool GetChoices( const SPropertyDesc *pDesc, EPCIEType nType, std::vector<std::string> *pChoices );

	// Text as typed or chosen, turned into a value by the type's rules. False
	// when it does not parse, in which case the row goes back to what is stored.
	// A path that is not a valid file name does not parse, as the file and
	// folder editors put their stored value back over one.
	bool ParseValueText( IManipulator *pManipulator, const std::string &rszName, const std::string &rszText, CVariant *pValue );

	// Writes a value through the undo list, as CPCMainTreeControl's
	// UpdateValueFromPCItemEditor does: nothing when it equals what is stored
	// or the manipulator refuses it, otherwise a change controller, redone into
	// every view on the object and added to the controller container. True when
	// something was written.
	bool CommitValue( CDefaultView *pView, const std::string &rszName, const CVariant &rNewValue );

	// CPCMainTreeControl::AddChangeOperation: one change for a field, and three,
	// one per float, for a vec3_color.
	bool AddChangeOperation( CObjectBaseController *pController, IManipulator *pManipulator,
													 const std::string &rszName, const CVariant &rValue );

	// "Players.[3]" -> 3: an array element's index, from its last segment. False
	// for a name that is not an array element.
	bool GetNodeIndex( const std::string &rszName, int *pnIndex );

	// An element put into, or taken out of, the array rszArrayName through the
	// undo list, as AddNode, InsertNode, DeleteNode and DeleteAllNodes wrote them
	// and redone into every view on the object. nIndex is where: NODE_ADD_INDEX
	// appends, NODE_REMOVEALL_INDEX empties the array. True when written.
	bool InsertNode( CDefaultView *pView, const std::string &rszArrayName, int nIndex );
	bool RemoveNode( CDefaultView *pView, const std::string &rszArrayName, int nIndex );

	// CopySelection's half that is not the clipboard: the value of every field
	// among rNames, kept in SUserData::pcSelection for Paste. Nodes are skipped.
	void CopyValues( IManipulator *pManipulator, const std::vector<std::string> &rNames );

	// PasteSelection: every copied value whose field rShown says the view shows,
	// as one controller on the undo list. False when nothing had been copied.
	bool PasteValues( CDefaultView *pView, const std::function<bool( const std::string& )> &rShown );
}
