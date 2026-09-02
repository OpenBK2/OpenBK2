#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/DefaultInputState.h"
#include "MapEditorLib/ObjectController.h"

#include <cstdint>

#include "ED_Common_export.h"

struct SEditorState
{
	SObjectSet objectSet;
	CDefaultInputState *pState;
};


class CWindowSimpleSharedController : public CObjectBaseController
{
	OBJECT_NOCOPY_METHODS( CWindowSimpleSharedController );
public:
	CObjectController * GetInternalController() { return &internalController; }
	static const std::string GetTemporaryLabel()     { return "CWindowSimpleSharedController::TEMPORARY_LABEL"; }
	//
	// IController
	virtual bool IsEmpty() const { return internalController.IsEmpty(); }
	virtual void GetDescription( CString *pstrDescription ) const
	{
		internalController.GetDescription( pstrDescription );
	}
	//
	void SetChildDesc( const std::string & szTypeName, const CDBID &rDBID )
	{
		szChildTypeName = szTypeName;
		dbid = rDBID;
	}
	inline const std::string & GetChildTypeName() const { return szChildTypeName; }
	inline const CDBID& GetChildID() const { return dbid; }
	//
private:
	CObjectController internalController;
	std::string szChildTypeName;
	CDBID dbid;
};


class CWindowSimpleSharedEditorSettings
{
public:
	CDBID templateScreenDBID;
	CDBID templateWindowDBID;

	// serializing...
	int operator&( IXmlSaver &xs );
};


class ED_COMMON_EXPORT CWindowSimpleSharedEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	//friend class CWindowSimpleSharedState;
	OBJECT_NOCOPY_METHODS( CWindowSimpleSharedEditor );
	//Данные специфичные для данного редактрора
	
	// Данные общего назначения 
	std::vector<SEditorState> states;
	CPtr<CWindowSimpleSharedController> pUndoController;
	CPtr<IManipulator> pUndoManipulator;
	bool bUOResult;
	//
	CWindowSimpleSharedEditor();
	//
public:
	CWindowSimpleSharedEditorSettings editorSettings;

	//IEditor
	void GetTemporaryLabel( std::string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this;  }
	IInputState* GetInputState() { return states.back().pState; }
	void GetChildFrameType( std::string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
	void CreateControls() {}
	void PostCreateControls() {}
	void PreDestroyControls() {}
	void DestroyControls() { 	Destroy(); }
	void Create();
	void Destroy();

	//IView
	void RemoveViewManipulator();

	// Создание Undo Operation
	CWindowSimpleSharedController* CreateUndoController() 
	{ 
		return CDefaultView::CreateController<CWindowSimpleSharedController>( static_cast<CWindowSimpleSharedController*>( 0 ) ); 
	}

	//CDefaultView
	void Undo( IController* pController );
	void Redo( IController* pController );
	
	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) { return false; }
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) { return false; }

	// methods
	bool UOBegin( IManipulator *pManipulator, const std::string &rObjectTypeName, const CDBID &rDBID );
	bool UOEnd();
	bool UOSetValue( const std::string & szName, const CVariant &newValue );
	bool UOInsertNode( const std::string &szName, const std::string &szChildTypeName, const CDBID &rDBID );
	bool UORemoveNode( const std::string &szName, int nIndex, const std::string &szChildTypeName, const CDBID &rDBID );
	bool InsertObject( IManipulator *pManipulator, const std::string &szName );
	bool RemoveObject( IManipulator *pManipulator, const std::string &szName );

	void PushState( const SObjectSet & _objectSet, CDefaultInputState * pState, bool bCallEnterLeave = true );
	void PopState();
	bool HasPushedStates() const { return !states.empty(); }
	bool HasMoreThanOnePushedStates() const { return states.size() > 1; }
	bool SetupState( const SObjectSet & _objectSet );

	void PushRunModeState( const std::string &rszEditorTypeName, const CDBID &rDBID );
	void PopRunModeState();
};


