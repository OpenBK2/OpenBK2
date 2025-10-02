#include "StdAfx.h"

#include "..\MapEditorLib\EditorFactory.h"
#include "..\MapEditorLib\Tools_HashSet.h"
#include "..\MapEditorLib\Interface_UserData.h"
#include "../libdb/ResourceManager.h"
//#include "EditorMethods.h"
#include "WindowSimpleSharedEditor.h"
#include "WindowSimpleSharedState.h"
#include "UIRunModeState.h"
//#include "UIScene.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EDITOR_IN_DLL( WindowSimpleShared, CWindowSimpleSharedEditor )

#define REGISTER_EDITOR_LOCAL( name ) \
	REGISTER_EDITOR_IN_DLL( name, CWindowSimpleSharedEditor )

REGISTER_EDITOR_LOCAL( WindowScreen )
REGISTER_EDITOR_LOCAL( WindowSimple )
REGISTER_EDITOR_LOCAL( WindowMSButton )
REGISTER_EDITOR_LOCAL( WindowTextView )
REGISTER_EDITOR_LOCAL( WindowEditLine )
REGISTER_EDITOR_LOCAL( WindowConsoleOutput )
REGISTER_EDITOR_LOCAL( WindowScrollBar )
REGISTER_EDITOR_LOCAL( WindowScrollableContainer )
REGISTER_EDITOR_LOCAL( WindowSlider )
REGISTER_EDITOR_LOCAL( WindowSelection )
REGISTER_EDITOR_LOCAL( WindowListCtrl )
REGISTER_EDITOR_LOCAL( WindowListItem )
REGISTER_EDITOR_LOCAL( WindowListHeader )
REGISTER_EDITOR_LOCAL( Window1LvlTreeControl )
REGISTER_EDITOR_LOCAL( WindowTabControl )
REGISTER_EDITOR_LOCAL( WindowComboBox )
REGISTER_EDITOR_LOCAL( WindowProgressBar )
REGISTER_EDITOR_LOCAL( WindowMultiTextureProgressBar )
REGISTER_EDITOR_LOCAL( WindowConsole )
REGISTER_EDITOR_LOCAL( WindowPlayer )
REGISTER_EDITOR_LOCAL( WindowStatsSystem )
REGISTER_EDITOR_LOCAL( WindowTooltip )

#undef REGISTER_EDITOR_LOCAL

CWindowSimpleSharedEditor::CWindowSimpleSharedEditor()
{
}


void CWindowSimpleSharedEditor::Create()
{
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "WindowSimpleShared", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	}
	PushState( GetObjectSet(), new CWindowSimpleSharedState( this ), false );
}


void CWindowSimpleSharedEditor::Destroy()
{
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "WindowSimpleShared", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
	}

	// call PopState for last state only
	if ( HasPushedStates() )
		PopState();

	// just remove other states
	while ( HasPushedStates() )
	{
		delete states.back().pState;
		states.pop_back();
	}
}


void CWindowSimpleSharedEditor::PushState( const SObjectSet & _objectSet, CDefaultInputState * pState, bool bCallEnterLeave )
{
	if ( bCallEnterLeave )
	{
		states.back().objectSet = GetObjectSet(); //BUGFIX
		states.back().pState->Leave();
		SetupState( _objectSet );
	}

	states.push_back( SEditorState() );
	states.back().objectSet = _objectSet;
	states.back().pState = pState;

	if ( bCallEnterLeave )
	{
		states.back().pState->Enter();
	}
}


void CWindowSimpleSharedEditor::PopState()
{
	states.back().pState->Leave();

	delete states.back().pState;
	states.pop_back();

	if ( HasPushedStates() )
	{
		SetupState( states.back().objectSet );
		states.back().pState->Enter();
	}
}


bool CWindowSimpleSharedEditor::SetupState( const SObjectSet & _objectSet )
{
	const string szTypeName = _objectSet.szObjectTypeName;
	if ( CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szTypeName, _objectSet.objectNameSet.begin()->first ) )
	{
		string tempLabel;
		GetTemporaryLabel( &tempLabel );

		SetViewManipulator( pManipulator, _objectSet, tempLabel );
		return true;
	}
	return false;
}


bool CWindowSimpleSharedEditor::UOBegin( IManipulator *pManipulator, const string &rObjectTypeName, const CDBID &rDBID )
{
	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedEditor::BeginUndo: pTableManipulator == 0" );

	pUndoManipulator = pManipulator;
	pUndoController = CreateUndoController();

	if ( pUndoController )
	{
		SObjectSet _objectSet;
		_objectSet.szObjectTypeName = rObjectTypeName;
		InsertHashSetElement( &( _objectSet.objectNameSet ), rDBID );
		
		pUndoController->GetInternalController()->SetObjectSet( _objectSet );
		pUndoController->SetTemporaryLabel( CWindowSimpleSharedController::GetTemporaryLabel() );
		pUndoManipulator->GetNameList( &( pUndoController->GetNameList() ) );
	}
	bUOResult = true;
	return (pUndoManipulator != 0) && (pUndoController != 0);
}


bool CWindowSimpleSharedEditor::UOEnd()
{
	if ( bUOResult )
	{
		if ( pUndoController->Redo( false, true, 0 ) )
		{
			Singleton<IControllerContainer>()->Add( pUndoController );
			return true;
		}
	}
	return false;
}


bool CWindowSimpleSharedEditor::UOSetValue( const string & szName, const CVariant & newValue )
{
	if ( (pUndoManipulator != 0) && (pUndoController != 0) && bUOResult)
	{
		if ( pUndoController->GetInternalController()->AddChangeOperation( szName, newValue, pUndoManipulator ) )
		{
			return true;
		}
	}
	bUOResult = false;
	return false;
}


bool CWindowSimpleSharedEditor::UOInsertNode( const string &szName, const string &szChildTypeName, const CDBID &rDBID )
{
	if ( (pUndoManipulator != 0) && (pUndoController != 0) && bUOResult)
	{
		if ( pUndoController->GetInternalController()->AddInsertOperation( szName, NODE_ADD_INDEX, pUndoManipulator ) )
		{
			pUndoController->SetChildDesc( szChildTypeName, rDBID );
			return true;
		}
	}
	bUOResult = false;
	return false;
}


bool CWindowSimpleSharedEditor::UORemoveNode( const string &szName, int nIndex, const string &szChildTypeName, const CDBID &rDBID )
{
	if ( (pUndoManipulator != 0) && (pUndoController != 0) && bUOResult )
	{
		if ( pUndoController->GetInternalController()->AddRemoveOperation( szName, nIndex, pUndoManipulator ) )
		{
			pUndoController->SetChildDesc( szChildTypeName, rDBID );
			return true;
		}
	}
	bUOResult = false;
	return false;
}


bool CWindowSimpleSharedEditor::InsertObject( IManipulator *pManipulator, const string & szName )
{
	//TODO: implement undo-op
	return pManipulator->InsertNode( szName );
}


bool CWindowSimpleSharedEditor::RemoveObject( IManipulator *pManipulator, const string & szName )
{
	//TODO: implement undo-op
	return pManipulator->RemoveNode( szName );
}


void CWindowSimpleSharedEditor::Undo( IController* pController )
{
	string tempLabel;
	pController->GetTemporaryLabel( &tempLabel );

	if ( tempLabel == CWindowSimpleSharedController::GetTemporaryLabel() )
	{
		if ( CWindowSimpleSharedController *pCustomController = dynamic_cast<CWindowSimpleSharedController*>( pController ) )
		{
			CObjectBaseController *pObjectController = pCustomController->GetInternalController();
			pObjectController->Undo( true, true, 0 );

			const string & szTypeName = pObjectController->GetObjectSet().szObjectTypeName;
			for ( CObjectController::CUndoDataList::const_iterator posUndoData = pObjectController->undoDataList.begin(); posUndoData != pObjectController->undoDataList.end(); ++posUndoData )
			{
				const string szName = posUndoData->szName;
				switch ( posUndoData->eType )
				{
					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_INSERT:
						{
							dynamic_cast<CWindowSimpleSharedState*>(GetInputState())->UndoInsert( pCustomController->GetChildTypeName(), pCustomController->GetChildID(), szName );
							//DebugTrace( "Undo: TYPE_INSERT %s[%d] %s %d", szTypeName.c_str(), nID, szName.c_str(), (int)( posUndoData->oldValue ) );
						}
						break;

					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_REMOVE:
						{
							dynamic_cast<CWindowSimpleSharedState*>(GetInputState())->UndoRemove( pCustomController->GetChildTypeName(), pCustomController->GetChildID(), szName );
							//DebugTrace( "Undo: TYPE_REMOVE %s[%d] %s %d", szTypeName.c_str(), nID, szName.c_str(), (int)( posUndoData->newValue ) );
						}
						break;

					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_CHANGE:
						{
							dynamic_cast<CWindowSimpleSharedState*>(GetInputState())->UndoChange( szTypeName, pObjectController->GetObjectSet().objectNameSet.begin()->first, szName, posUndoData->oldValue );
							//DebugTrace( "Undo: TYPE_CHANGE %s[%d] %s", szTypeName.c_str(), nID, szName.c_str() );
						}
						break;
				}
			}
		}
	}
}


void CWindowSimpleSharedEditor::Redo( IController* pController )
{
	string tempLabel;
	pController->GetTemporaryLabel( &tempLabel );

	if ( tempLabel == CWindowSimpleSharedController::GetTemporaryLabel() )
	{
		if ( CWindowSimpleSharedController *pCustomController = dynamic_cast<CWindowSimpleSharedController*>( pController ) )
		{
			CObjectBaseController *pObjectController = pCustomController->GetInternalController();
			pObjectController->Redo( false, true, 0 );

			const string & szTypeName = pObjectController->GetObjectSet().szObjectTypeName;

			for ( CObjectController::CUndoDataList::const_iterator posUndoData = pObjectController->undoDataList.begin(); posUndoData != pObjectController->undoDataList.end(); ++posUndoData )
			{
				const string szName = posUndoData->szName;
				switch ( posUndoData->eType )
				{
					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_INSERT:
						{
							//DebugTrace( "Redo: TYPE_INSERT %s[%d] %s %d", szTypeName.c_str(), nID, szName.c_str(), (int)( posUndoData->oldValue ) );
						}
						break;

					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_REMOVE:
						{
							//DebugTrace( "Redo: TYPE_REMOVE %s[%d] %s  %d", szTypeName.c_str(), nID, szName.c_str(), (int)( posUndoData->newValue ) );
						}
						break;

					///////////////////////////////////////////
					case CObjectController::SUndoData::TYPE_CHANGE:
						{
							//DebugTrace( "Redo: TYPE_CHANGE %s[%d] %s", szTypeName.c_str(), nID, szName.c_str() );
						}
						break;
				}
			}
		}
	}
}


void CWindowSimpleSharedEditor::PushRunModeState( const string &rszEditorTypeName, const CDBID &rDBID )
{
	PushState( GetObjectSet(), new CUIRunModeState( this, rszEditorTypeName, rDBID ), true );
}


void CWindowSimpleSharedEditor::PopRunModeState()
{
	PopState();
}


void CWindowSimpleSharedEditor::RemoveViewManipulator()
{
	if( HasMoreThanOnePushedStates() )
	{
		// call Leave only for last state
		states.back().pState->Leave();

		// just destroy all states
		while ( HasMoreThanOnePushedStates() )
		{
			delete states.back().pState;
			states.pop_back();
		}
		// enter to remaining state
		//SetupState( states.back().objectSet );
		//states.back().pState->Enter();
	}
}

int CWindowSimpleSharedEditorSettings::operator&( IXmlSaver &xs )
{
	//xs.Add( "ObjectTypeDataMap", &objectTypeDataMap );
	xs.Add( "TemplateScreenDBID", &templateScreenDBID );
	xs.Add( "TemplateWindowDBID", &templateWindowDBID );
	return 0;
}


