#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"

#include "System/Dg.h"
#include "System/GResource.h"
#include "MapeditorLib/Interface_ChildFrame.h"
#include "MapeditorLib/Interface_MainFrame.h"
#include "MapeditorLib/Interface_Progress.h"
#include "ControllerContainer.h"

#include "MapEditorLib/EditorFactory.h"
#include "EditorContainer.h"

string CEditorContainer::GetBaseObjectType( const string &rszExtendObjectTypeName )
{
	CExtendTypeMap::iterator posExtendType = extendTypeMap.find( rszExtendObjectTypeName );
	if( posExtendType != extendTypeMap.end() )
	{
		return posExtendType->second;
	}
	else
	{
		return rszExtendObjectTypeName;
	}
}


bool CEditorContainer::CanCreate( const string &rszObjectTypeName )
{
	return NEditorFactory::CanCreateEditor( GetBaseObjectType( rszObjectTypeName ) );
}


void CEditorContainer::Create( const string &rszObjectTypeName )
{
	const string szBaseObjectTypeName = GetBaseObjectType( rszObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	if( posEditor == editorMap.end() )
	{
		DebugTrace( "NEditorFactory: Create Editor: <%s>", rszObjectTypeName.c_str() );
		editorMap[szBaseObjectTypeName] = NEditorFactory::CreateEditor( szBaseObjectTypeName );
	}
}


void CEditorContainer::AddExtendObjectType( const string &rszBaseObjectTypeName, const string &rszExtendObjectTypeName )
{
	extendTypeMap[rszExtendObjectTypeName] = rszBaseObjectTypeName;
}


void CEditorContainer::Destroy( const string &rszObjectTypeName, bool bDestroyChildFrame )
{
	//	
	if ( szActiveTypeName == rszObjectTypeName )
	{
		DestroyActiveEditor( string(), string(), bDestroyChildFrame );
	}
	//
	const string szBaseObjectTypeName = GetBaseObjectType( rszObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	if ( posEditor != editorMap.end() )
	{
		DebugTrace( "NEditorFactory: Destroy Editor: <%s>", rszObjectTypeName.c_str() );
		editorMap.erase( posEditor );
		posEditor->second->DestroyControls();
	}
}


void CEditorContainer::DestroyActiveEditor( const string &rszNewEditorTypeName, const string &rszNewChildFrameTypeName, bool bDestroyChildFrame )
{
	if ( !szActiveTypeName.empty() )
	{
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		const string szNewBaseObjectTypeName = GetBaseObjectType( rszNewEditorTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			if ( posEditor->second )
			{
				IView *pView = posEditor->second->GetView();
				IInputState *pInputState = posEditor->second->GetInputState();
				//
				SSWTParams swtParams;
				Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
				if ( pView )
				{
					DebugTrace( "Leave View: <%s>", szActiveTypeName.c_str() );
					pView->Leave();
				}
				if ( pInputState )
				{
					DebugTrace( "Leave Editor Input State: <%s>", szActiveTypeName.c_str() );
					pInputState->Leave();
				}
				if ( pView )
				{
					DebugTrace( "Remove Editor View Manipulator: <%s>", szActiveTypeName.c_str() );
					pView->RemoveViewManipulator();
				}
				if ( szNewBaseObjectTypeName != szActiveBaseObjectTypeName )
				{
					DebugTrace( "Destroy Editor: <%s>", szActiveTypeName.c_str() );
					posEditor->second->Destroy();
					szActiveTypeName.clear();
				}
				// удаляем Child Frame
				if ( !rszNewChildFrameTypeName.empty() && Singleton<IChildFrameContainer>()->IsActive( rszNewChildFrameTypeName ) )
				{
					Singleton<IChildFrameContainer>()->Leave();
				}
				else if ( bDestroyChildFrame )
				{
					Singleton<IChildFrameContainer>()->Destroy();
				}
			}
		}
	}
}


void CEditorContainer::CreateNewEditor( IManipulator* _pManipulator, const SObjectSet &rObjectSet, const string &rszNewChildFrameTypeName )
{
	if ( !CanCreate( rObjectSet.szObjectTypeName ) )
	{
		return;
	}
	// Создаем заранее новый редактор чтобы опросить тип ChildFrame
	const string szBaseObjectTypeName = GetBaseObjectType( rObjectSet.szObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	NI_ASSERT( posEditor != editorMap.end(), "CEditorContainer::CreateNewEditor(): posEditor = editorMap.end()" );
	if ( posEditor->second )
	{
		// Создаем Child Frame
		string szChildFrameTypeName;
		posEditor->second->GetChildFrameType( &szChildFrameTypeName );
		if ( !szChildFrameTypeName.empty() )
		{
			if ( Singleton<IChildFrameContainer>()->IsActive( szChildFrameTypeName ) )
			{
				Singleton<IChildFrameContainer>()->Enter();
			}
			else
			{
				Singleton<IChildFrameContainer>()->Create( szChildFrameTypeName );
			}
		}
		if ( szActiveTypeName.empty() )
		{
			//
			DebugTrace( "Create Editor: <%s>", rObjectSet.szObjectTypeName.c_str() );
			posEditor->second->Create();
			szActiveTypeName = rObjectSet.szObjectTypeName;
		}
		IView *pView = posEditor->second->GetView();
		IInputState *pInputState = posEditor->second->GetInputState();
		//
		SSWTParams swtParams;
		swtParams.szType = rObjectSet.szObjectTypeName;
		if ( !rObjectSet.objectNameSet.empty() )
		{
			swtParams.szObject = rObjectSet.objectNameSet.begin()->first.ToString();
		}
		Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
		if ( pView )
		{
			DebugTrace( "Set Editor View Manipulator: <%s>", rObjectSet.szObjectTypeName.c_str() );
			string szTemporaryLabel;
			posEditor->second->GetTemporaryLabel( &szTemporaryLabel );
			pView->SetViewManipulator( _pManipulator, rObjectSet, szTemporaryLabel );
		}
		if ( pInputState )
		{
			DebugTrace( "Enter Editor Input State: <%s>", rObjectSet.szObjectTypeName.c_str() );
			pInputState->Enter();
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		}
		if ( pView )
		{
			DebugTrace( "Enter View: <%s>", szActiveTypeName.c_str() );
			pView->Enter();
		}
	}
}


IEditor* CEditorContainer::Create( IManipulator* _pManipulator, const SObjectSet &rObjectSet )
{
	NProgress::Create( true );
	if ( !rObjectSet.objectNameSet.empty() )
	{
		CString strPM;
		strPM.LoadString( IDS_PM_CREATE_EDITOR );
		NProgress::SetMessage( StrFmt( strPM, rObjectSet.objectNameSet.begin()->first.ToString().c_str() ) );
	}
	NProgress::SetRange( 0, 2 );
	NProgress::SetPosition( 0 );
	//
	if ( !CanCreate( rObjectSet.szObjectTypeName ) )
	{
		NProgress::Destroy();	
		return 0;
	}
	// Создаем заранее новый редактор чтобы опросить тип ChildFrame
	const string szBaseObjectTypeName = GetBaseObjectType( rObjectSet.szObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	if( posEditor == editorMap.end() )
	{
		DebugTrace( "NEditorFactory: Create Editor: <%s>", rObjectSet.szObjectTypeName.c_str() );
		editorMap[szBaseObjectTypeName] = NEditorFactory::CreateEditor( szBaseObjectTypeName );
		posEditor = editorMap.find( szBaseObjectTypeName );
	}
	//
	if ( posEditor->second )
	{
		string szChildFrameTypeName;
		posEditor->second->GetChildFrameType( &szChildFrameTypeName );

		// Разрушаем старый редактор и закрываем ChildFrame (если необходимо)
		DestroyActiveEditor( rObjectSet.szObjectTypeName, szChildFrameTypeName, true );
		if ( posEditor->second->ShowProgress() )
		{
			NProgress::SetPosition( 1 );
		}
		// Создаем новый редактор и новый ChildFrame (если необходимо)
		CreateNewEditor( _pManipulator, rObjectSet, szChildFrameTypeName );
		if ( posEditor->second->ShowProgress() )
		{
			NProgress::SetPosition( 2 );
		}
		NProgress::Destroy();	
		return posEditor->second;
	}
	NProgress::Destroy();	
	return 0;
}


void CEditorContainer::DestroyActiveEditor( bool bDestroyChildFrame )
{
	NProgress::Create( true );
	CString strPM;
	strPM.LoadString( IDS_PM_DESTROY_EDITOR );
	NProgress::SetMessage( string( strPM ) );
	NProgress::SetRange( 0, 1 );
	NProgress::SetPosition( 0 );
	Destroy( szActiveTypeName, bDestroyChildFrame );
	NProgress::SetPosition( 1 );
	NProgress::Destroy();	
}


void CEditorContainer::ReloadActiveEditor( bool bClearResources )
{
	if ( !szActiveTypeName.empty() )
	{
		NProgress::Create( true );
		CString strPM;
		strPM.LoadString( IDS_PM_RELOAD_EDITOR );
		NProgress::SetMessage( string( strPM ) );
		NProgress::SetRange( 0, 1 );
		NProgress::SetPosition( 0 );
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			if ( posEditor->second )
			{
				IInputState *pInputState = posEditor->second->GetInputState();
				//
				if ( pInputState )
				{
					DebugTrace( "Leave Editor Input State: <%s>", szActiveTypeName.c_str() );
					pInputState->Leave();
					// Перегружаем Child Frame
					string szChildFrameTypeName;
					posEditor->second->GetChildFrameType( &szChildFrameTypeName );
					if ( !szChildFrameTypeName.empty() )
					{
						if ( Singleton<IChildFrameContainer>()->IsActive( szChildFrameTypeName ) )
						{
							Singleton<IChildFrameContainer>()->Leave();
						}
					}
					//
					if ( bClearResources )
					{
						//NGScene::ReloadTexture( 0 );
						ClearHoldQueue();
						NGScene::CResourceFileOpener::Clear();
					}
					//
					if ( !szChildFrameTypeName.empty() )
					{
						if ( Singleton<IChildFrameContainer>()->IsActive( szChildFrameTypeName ) )
						{
							Singleton<IChildFrameContainer>()->Enter();
						}
					}
					//
					DebugTrace( "Enter Editor Input State: <%s>", szActiveTypeName.c_str() );
					pInputState->Enter();
				}
			}
		}
	}
	NProgress::SetPosition( 1 );
	NProgress::Destroy();	
}


IEditor* CEditorContainer::GetActiveEditor()
{
	if ( !szActiveTypeName.empty() )
	{
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			return posEditor->second;
		}
	}
	return 0;
}


IInputState* CEditorContainer::GetActiveInputState()
{
	if ( !szActiveTypeName.empty() )
	{
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			if ( posEditor->second != 0 )
			{
				return posEditor->second->GetInputState();
			}
		}
	}
	return 0;
}


void CEditorContainer::CreateControls()
{
	for ( CEditorMap::iterator itEditor = editorMap.begin(); itEditor != editorMap.end(); ++itEditor )
	{
		itEditor->second->CreateControls();
	}
}


void CEditorContainer::PostCreateControls()
{
	for ( CEditorMap::iterator itEditor = editorMap.begin(); itEditor != editorMap.end(); ++itEditor )
	{
		itEditor->second->PostCreateControls();
	}
}


void CEditorContainer::PreDestroyControls()
{
	for ( CEditorMap::iterator itEditor = editorMap.begin(); itEditor != editorMap.end(); ++itEditor )
	{
		itEditor->second->PreDestroyControls();
	}
}


void CEditorContainer::DestroyControls()
{
	for ( CEditorMap::iterator itEditor = editorMap.begin(); itEditor != editorMap.end(); ++itEditor )
	{
		itEditor->second->DestroyControls();
	}
}


void CEditorContainer::Save( bool bSaveChanges )
{
	if ( !szActiveTypeName.empty() )
	{
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			if ( posEditor->second )
			{
				posEditor->second->Save( bSaveChanges );
			}
		}
	}
}


bool CEditorContainer::IsModified()
{
	if ( !szActiveTypeName.empty() )
	{
		const string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		CEditorMap::iterator posEditor = editorMap.find( szActiveBaseObjectTypeName );
		if ( posEditor != editorMap.end() )
		{
			if ( posEditor->second )
			{
				return posEditor->second->IsModified();
			}
		}
	}
	return false;
}

// basement storage  


