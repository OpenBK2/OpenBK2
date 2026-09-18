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
	// Each modal picker has its own undo/redo history. Keep a separate record
	// of applied edits as well: clearing the visible history must not make
	// Cancel forget changes, and nested pickers must restore their parent.
	struct SEditOperation
	{
		CPtr<IController> pController;
		bool bApplied = true;
	};
	struct SEditSession
	{
		CControllerList savedUndo;
		CControllerList savedRedo;
		std::list<SEditOperation> operations;
	};
	std::list<SEditSession> editSessions;

	void BeginEditSession();
	bool EndEditSession( bool bAccept );
	void SetEditOperationApplied( IController *pOperation, bool bApplied );
	// The list under the Undo and Redo arrows, made the first time one is used.
	NMenuDropDown::IView *pMenuDropDown;

	// UndoArrow and RedoArrow, which differed only in which list and button.
	bool ShowOperationList( bool bUndo );

public:
	// Finish explicitly after the dialog closes; an exceptional exit cancels.
	class CEditSession
	{
		CControllerContainer *pContainer;

	public:
		explicit CEditSession( CControllerContainer *_pContainer );
		~CEditSession();
		CEditSession( const CEditSession& ) = delete;
		CEditSession& operator=( const CEditSession& ) = delete;
		bool Finish( bool bAccept );
	};

	CControllerContainer();
	~CControllerContainer();
	// IControllerContainer
	bool IsEditSessionActive() const override { return !editSessions.empty(); }
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
