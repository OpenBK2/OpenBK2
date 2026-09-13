#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Controller.h"
// MapEditor's resource ids. This header used to bring them through MDDLDialog.h,
// and EditorContainer.cpp and others still rely on getting them here.
#include "ResourceDefines.h"

#include <cstdint>

#define UNDO_BUFFER_SIZE 25

namespace NMenuDropDown
{
	class IView;
}

class CControllerContainer : public IControllerContainer, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CControllerContainer );
	//
	typedef std::list<CPtr<IController> > CControllerList;
	CControllerList controllerList;
	CControllerList redoOperationList;
	// The list under the Undo and Redo arrows, made the first time one is used.
	NMenuDropDown::IView *pMenuDropDown;

	// UndoArrow and RedoArrow, which differed only in which list and button.
	bool ShowOperationList( bool bUndo );

public:
	CControllerContainer();
	~CControllerContainer();
	// IControllerContainer
	void Add( IController *pOperation );
	void Clear();
	bool CanUndo() const;
	bool CanRedo() const;
	bool Undo( int nCount );
	bool Redo( int nCount );
	bool UndoArrow();
	bool RedoArrow();
	int GetDescriptionList( CDescriptionList *pDescriptionList, bool bUndoList ) const;
	int RemoveTemporaryControllers( const std::string &rszTemporaryLabel );

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};
