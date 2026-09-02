#pragma once

#include "MDDLDialog.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

#define UNDO_BUFFER_SIZE 25

class CControllerContainer : public IControllerContainer, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CControllerContainer );
	//
	typedef std::list<CPtr<IController> > CControllerList;
	CControllerList controllerList;
	CControllerList redoOperationList;
	CMDDLDialog wndMDDLDialog;

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



