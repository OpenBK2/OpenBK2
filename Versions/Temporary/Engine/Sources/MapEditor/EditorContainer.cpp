#include "stdafx.h"
#include <fmt/printf.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "System/Dg.h"
#include "System/GResource.h"
#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Progress.h"
#include "ControllerContainer.h"

#include "MapEditorLib/EditorFactory.h"
#include "EditorContainer.h"

std::string CEditorContainer::GetBaseObjectType( const std::string &rszExtendObjectTypeName )
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


bool CEditorContainer::CanCreate( const std::string &rszObjectTypeName )
{
	return NEditorFactory::CanCreateEditor( GetBaseObjectType( rszObjectTypeName ) );
}


void CEditorContainer::Create( const std::string &rszObjectTypeName )
{
	const std::string szBaseObjectTypeName = GetBaseObjectType( rszObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	if( posEditor == editorMap.end() )
	{
		DebugTrace( "NEditorFactory: Create Editor: <%s>", rszObjectTypeName.c_str() );
		editorMap[szBaseObjectTypeName] = NEditorFactory::CreateEditor( szBaseObjectTypeName );
	}
}


void CEditorContainer::AddExtendObjectType( const std::string &rszBaseObjectTypeName, const std::string &rszExtendObjectTypeName )
{
	extendTypeMap[rszExtendObjectTypeName] = rszBaseObjectTypeName;
}


void CEditorContainer::Destroy( const std::string &rszObjectTypeName, bool bDestroyChildFrame )
{
	//	
	if ( szActiveTypeName == rszObjectTypeName )
	{
		DestroyActiveEditor( std::string(), std::string(), bDestroyChildFrame );
	}
	//
	const std::string szBaseObjectTypeName = GetBaseObjectType( rszObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	if ( posEditor != editorMap.end() )
	{
		DebugTrace( "NEditorFactory: Destroy Editor: <%s>", rszObjectTypeName.c_str() );
		editorMap.erase( posEditor );
		posEditor->second->DestroyControls();
	}
}


void CEditorContainer::DestroyActiveEditor( const std::string &rszNewEditorTypeName, const std::string &rszNewChildFrameTypeName, bool bDestroyChildFrame )
{
	if ( !szActiveTypeName.empty() )
	{
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
		const std::string szNewBaseObjectTypeName = GetBaseObjectType( rszNewEditorTypeName );
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


void CEditorContainer::CreateNewEditor( IManipulator* _pManipulator, const SObjectSet &rObjectSet, const std::string &rszNewChildFrameTypeName )
{
	if ( !CanCreate( rObjectSet.szObjectTypeName ) )
	{
		return;
	}
	// Создаем заранее новый редактор чтобы опросить тип ChildFrame
	const std::string szBaseObjectTypeName = GetBaseObjectType( rObjectSet.szObjectTypeName );
	CEditorMap::iterator posEditor = editorMap.find( szBaseObjectTypeName );
	NI_ASSERT( posEditor != editorMap.end(), "CEditorContainer::CreateNewEditor(): posEditor = editorMap.end()" );
	if ( posEditor->second )
	{
		// Создаем Child Frame
		std::string szChildFrameTypeName;
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
			std::string szTemporaryLabel;
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
		NProgress::SetMessage( fmt::sprintf( strPM.GetString(), rObjectSet.objectNameSet.begin()->first.ToString().c_str() ) );
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
	const std::string szBaseObjectTypeName = GetBaseObjectType( rObjectSet.szObjectTypeName );
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
		std::string szChildFrameTypeName;
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
	NProgress::SetMessage( std::string( strPM ) );
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
		NProgress::SetMessage( std::string( strPM ) );
		NProgress::SetRange( 0, 1 );
		NProgress::SetPosition( 0 );
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
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
					std::string szChildFrameTypeName;
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
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
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
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
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
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
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
		const std::string szActiveBaseObjectTypeName = GetBaseObjectType( szActiveTypeName );
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


