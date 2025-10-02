#pragma once

#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "MapEditorLib/EditorBase.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/DefaultInputState.h"
#include "MapEditorLib/ObjectController.h"


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
	static const string GetTemporaryLabel()     { return "CWindowSimpleSharedController::TEMPORARY_LABEL"; }
	//
	// IController
	virtual bool IsEmpty() const { return internalController.IsEmpty(); }
	virtual void GetDescription( CString *pstrDescription ) const
	{
		internalController.GetDescription( pstrDescription );
	}
	//
	void SetChildDesc( const string & szTypeName, const CDBID &rDBID )
	{
		szChildTypeName = szTypeName;
		dbid = rDBID;
	}
	inline const string & GetChildTypeName() const { return szChildTypeName; }
	inline const CDBID& GetChildID() const { return dbid; }
	//
private:
	CObjectController internalController;
	string szChildTypeName;
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


class CWindowSimpleSharedEditor : public CEditorBase, public CDefaultView, public ICommandHandler
{
	//friend class CWindowSimpleSharedState;
	OBJECT_NOCOPY_METHODS( CWindowSimpleSharedEditor );
	//Данные специфичные для данного редактрора
	
	// Данные общего назначения 
	vector<SEditorState> states;
	CPtr<CWindowSimpleSharedController> pUndoController;
	CPtr<IManipulator> pUndoManipulator;
	bool bUOResult;
	//
	CWindowSimpleSharedEditor();
	//
public:
	CWindowSimpleSharedEditorSettings editorSettings;

	//IEditor
	void GetTemporaryLabel( string *pszTemporaryLabel ) { pszTemporaryLabel->clear(); }
	IView* GetView() { return this;  }
	IInputState* GetInputState() { return states.back().pState; }
	void GetChildFrameType( string *pszChildFrameTypeName ) { ( *pszChildFrameTypeName ) = "__CHILD_FRAME_DX_SCENE_LABEL__"; }
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
	bool HandleCommand( UINT nCommandID, DWORD dwData ) { return false; }
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck ) { return false; }

	// methods
	bool UOBegin( IManipulator *pManipulator, const string &rObjectTypeName, const CDBID &rDBID );
	bool UOEnd();
	bool UOSetValue( const string & szName, const CVariant &newValue );
	bool UOInsertNode( const string &szName, const string &szChildTypeName, const CDBID &rDBID );
	bool UORemoveNode( const string &szName, int nIndex, const string &szChildTypeName, const CDBID &rDBID );
	bool InsertObject( IManipulator *pManipulator, const string &szName );
	bool RemoveObject( IManipulator *pManipulator, const string &szName );

	void PushState( const SObjectSet & _objectSet, CDefaultInputState * pState, bool bCallEnterLeave = true );
	void PopState();
	bool HasPushedStates() const { return !states.empty(); }
	bool HasMoreThanOnePushedStates() const { return states.size() > 1; }
	bool SetupState( const SObjectSet & _objectSet );

	void PushRunModeState( const string &rszEditorTypeName, const CDBID &rDBID );
	void PopRunModeState();
};


