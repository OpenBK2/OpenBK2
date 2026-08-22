#include "stdafx.h"

#include "SceneB2/Scene.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "mapeditorlib/resourcedefines.h"
#include "EditorOptions.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Logger.h"
#include "SceneB2/CameraScriptMutators.h"

#include "DrawToolsDC.h"

#include "ScriptCameraState.h"
#include "ScriptCameraAddDlg.h"
#include "ScriptCameraRun.h"
#include "KeySettingsDlg.h"

#include <cstdint>

#define MOV_ED_DEF_LEN (60.0f) // default movie length (sec)

//
//		CAMERA UTILS
//

static void GetCameraPlacement( NCamera::CCameraPlacement *pCameraPlacement )
{
	if ( CObj<ICamera> pCamera = Camera() )
	{
		pCameraPlacement->vPosition = pCamera->GetPos();
		Vis2AI( &(pCameraPlacement->vPosition) );
		float fDistance;
		pCamera->GetPlacement( &fDistance, &(pCameraPlacement->fPitch), &(pCameraPlacement->fYaw) );
		pCameraPlacement->fFOV = pCamera->GetFOV();
	}
}


//
//
//		SCRIPT CAMERA STATE
//
//

CScriptCameraState::CScriptCameraState( CMapInfoEditor *_pMapInfoEditor )
	: pMapInfoEditor( _pMapInfoEditor ),
	nFOV( 26 ),
	bDrawMarkers( true )
{
	NI_ASSERT( pMapInfoEditor != 0, "CScriptCameraState(): Invalid parameter: pMapInfoEditor == 0" );
}


void CScriptCameraState::Enter()
{
	dialogData.scriptCameras.clear();
	moviesData.Clear();
	cameraMarkers.clear();

	RefreshDialogData( true );
	if ( moviesData.scriptMoviesData.scriptMovieSequences.size() < 1 )
	{
		AddSequence();
		RefreshDialogData( true );
	}

	Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_CAMERA_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAP_INFO_EDITOR, ID_MI_VIEW_MOVIE_EDITOR, 1 );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MOVIES_EDITOR_WINDOW, ID_MOV_ED_SET_TIMER, 0 );
}


void CScriptCameraState::Leave()
{
	if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
		pMoviesHolder->Stop();

	dialogData.scriptCameras.clear();
	moviesData.Clear();
	sceneDrawTool.Clear();

	//DeleteScriptCameraMarkers();

	Singleton<IGameTimer>()->SetSpeed( 0 );

	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_CAMERA_STATE );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAP_INFO_EDITOR, ID_MI_VIEW_MOVIE_EDITOR, 0 );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MOVIES_EDITOR_WINDOW, ID_MOV_ED_KILL_TIMER, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MOVIES_EDITOR_WINDOW, ID_MOV_ED_RESET_DIALOG, 0 );
}


bool CScriptCameraState::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID ) 
	{
		case ID_SCRIPT_CAMERA_GET_YAW:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_GET_YAW, (uint32_t)( &fYaw ) );
			Camera()->SetYaw( ToRadian(fYaw) );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			break;
		}
		//
		case ID_SCRIPT_CAMERA_GET_PITCH:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_GET_PITCH, (uint32_t)( &fPitch ) );
			Camera()->SetPitch( ToRadian(270.0 - fPitch) );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			break;
		}
		//
		case ID_SCRIPT_CAMERA_GET_FOV:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_GET_FOV, (uint32_t)( &nFOV ) );
			Camera()->SetFOV( nFOV );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			break;
		}
		//
		case ID_SCRIPT_CAMERA_SET_YAW:
		{
			fYaw = Camera()->GetYaw();
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_YAW, (uint32_t)(&fYaw) );
			break;
		}
		//
		case ID_SCRIPT_CAMERA_SET_PITCH:
		{
			fPitch = Camera()->GetPitch();
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_PITCH, (uint32_t)(&fPitch) );
			break;
		}
		//
		case ID_SCRIPT_CAMERA_SET_FOV:
		{
			nFOV = Camera()->GetFOV();
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_FOV, (uint32_t)(&nFOV) );
			break;
		}
		//
		case ID_MOV_ED_ON_TIMER:
		{
			//
		}
		//
		case ID_SCRIPT_CAMERA_WINDOW_UI_EVENT:
		{
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_SCRIPT_CAMERA_WINDOW, ID_WINDOW_GET_DIALOG_DATA, reinterpret_cast<uint32_t>(&dialogData)) )
			{
				switch ( dialogData.eLastAction )
				{
					case SScriptCameraWindowData::SCA_CAMERA_ADD:
					{
						if ( AddScriptPlacement() )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptCameraWindowData::SCA_CAMERA_SAVE:
					{
						if ( UpdateScriptPlacement(dialogData.nCurrentCamera) )
							RefreshDialogData( true );

						NCamera::CCameraPlacement newCamera;
						GetCameraPlacementByID( &newCamera, dialogData.nCurrentCamera );
						SetCameraPlacement( newCamera );
						break;
					}
					//
					case SScriptCameraWindowData::SCA_CAMERA_DELETE:
					{
						if ( DeleteScriptPlacement(dialogData.nCurrentCamera) )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptCameraWindowData::SCA_CAMERA_JUMP:
					{
						NCamera::CCameraPlacement newCamera;
						GetCameraPlacementByID( &newCamera, dialogData.nCurrentCamera );
						SetCameraPlacement( newCamera );
						RefreshDialogData( false );
						break;
					}
					//
					case SScriptCameraWindowData::SCA_CAMERA_RUN:
					{
						ScriptCameraRun( dialogData.eRunType );
						break;
					}
				}
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return true;
			}
			break;
		}
		//
		case ID_SCRIPT_CAMERA_MOV_ED_UI_EVENT:
		{
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_MOVIES_EDITOR_WINDOW, ID_WINDOW_GET_DIALOG_DATA, reinterpret_cast<uint32_t>(&moviesData) ) )
			{
				switch ( moviesData.eLastAction )
				{
					case SScriptMovieEditorData::ME_JUMP_FIRST_KEY:
					{
						if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
						{
							pMoviesHolder->JumpFirstKey();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_JUMP_LAST_KEY:
					{
						if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
						{
							pMoviesHolder->JumpLastKey();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_STEP_PREV_KEY:
					{
						if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
						{
							pMoviesHolder->StepPrevKey();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_STEP_NEXT_KEY:
					{
						if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
						{
							pMoviesHolder->StepNextKey();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_PLAY:
					{
						if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
						{
							pMoviesHolder->Play();
							bDrawMarkers = false;
							//DeleteScriptCameraMarkers();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_PAUSE:
					{
						if ( ICamera *pCamera = Camera() )
						{
							pCamera->ConformMouseMutator2ScriptMutator();
							if ( CScriptMoviesMutatorHolder *pMoviesHolder = pCamera->GetScriptMutatorsHolder() )
							{
								pMoviesHolder->Pause();
								bDrawMarkers = true;
								//RefreshScriptCameraMarkers();
							}
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_STOP:
					{
						if ( ICamera *pCamera = Camera() )
						{
							pCamera->ConformMouseMutator2ScriptMutator();
							if ( CScriptMoviesMutatorHolder *pMoviesHolder = pCamera->GetScriptMutatorsHolder() )
							{
								pMoviesHolder->Stop();
								//RefreshScriptCameraMarkers();
							}
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_MOVIE_SWITCH:
					{
						SetMovie( moviesData.nActiveMovie );
						RefreshDialogData( false );
						break;
					}
					//
					case SScriptMovieEditorData::ME_CHANGE_TIME:
					{
						if ( ICamera *pCamera = Camera() )
						{
							if ( CScriptMoviesMutatorHolder *pMoviesHolder = pCamera->GetScriptMutatorsHolder() )
							{
								pMoviesHolder->SetTime( moviesData.fCursorTime );
							}

							pCamera->ConformMouseMutator2ScriptMutator();
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_SELECT_CAMERA:
					{
						dialogData.nCurrentCamera = SelectCameraByMovieParams( moviesData.nActiveMovie, moviesData.fCursorTime );
						RefreshDialogData( false );
						break;
					}
					//
					case SScriptMovieEditorData::ME_ADD_SEQ:
					{
						if ( AddSequence() )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptMovieEditorData::ME_DEL_SEQ:
					{
						if ( DeleteSequence(moviesData.nActiveMovie) )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptMovieEditorData::ME_CHANGE_SPEED:
					{
						int nGameTimerSpeed = Clamp<int>( moviesData.nSpeed, -10, +10 );
						Singleton<IGameTimer>()->SetSpeed( nGameTimerSpeed );
						break;
					}
					//
					case SScriptMovieEditorData::ME_INSERT_KEY:
					{
						if ( AddPosKey(moviesData.fCursorTime, moviesData.nActiveMovie) )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptMovieEditorData::ME_SAVE_KEY:
					{
						if ( UpdateScriptPlacement(dialogData.nCurrentCamera) )
							RefreshDialogData( true );
						break;
					}
					//
					case SScriptMovieEditorData::ME_KEY_SETTINGS:
					{
						if ( KeySetup(moviesData.activeKeysList, moviesData.nActiveMovie) )
						{
							RefreshDialogData( true );
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_DELETE_KEYS:
					{
						if ( DeleteKeys(moviesData.activeKeysList, moviesData.nActiveMovie, false) )
						{
							RefreshDialogData( true );
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_MOVE_KEYS:
					{
						if ( MoveKeys(moviesData.activeKeysList, moviesData.fMoveValue, moviesData.nActiveMovie) )
						{
							RefreshDialogData( true );
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_RESIZE:
					{
						if ( SetSequenceLength(moviesData.fNewLength, moviesData.nActiveMovie) )
						{
							RefreshDialogData( true );
						}
						break;
					}
					//
					case SScriptMovieEditorData::ME_CLEAR_MARKERS:
					{
						bDrawMarkers = false;
						//DeleteScriptCameraMarkers();
						break;
					}
					//
					case SScriptMovieEditorData::ME_DRAW_MARKERS:
					{
						bDrawMarkers = true;
						//RefreshScriptCameraMarkers();
						break;
					}
				}
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
			}
			//
			break;
		}
	}
	return false;
}


bool CScriptCameraState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CScriptCameraState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CScriptCameraState::UpdateCommand(), pbCheck == 0" );

	switch( nCommandID ) 
	{
		case ID_SCRIPT_CAMERA_WINDOW_UI_EVENT:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		default:
			return false;
	}
	return false;
}


// align DB data by it's own rules
void CScriptCameraState::FixDBMoviesIndexes( int nDeletedKey )
{
	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			bool bResult = pManipulator->RemoveNode( "ScriptMovies.ScriptCameraPlacements", nDeletedKey );
			for ( int nPlacementID = nDeletedKey; nPlacementID < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size(); ++nPlacementID )
			{
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Name", nPlacementID),
																																		StrFmt("%d", nPlacementID), pManipulator );
			}

			for ( int nSeqID = 0; nSeqID < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size(); ++nSeqID )
			{
				const NDb::SScriptMovieSequence &seq = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqID];
				for ( int nPosKeyID = 0; nPosKeyID < seq.posKeys.size(); ++nPosKeyID )
				{
					int nPosIndex = seq.posKeys[nPosKeyID].nPositionIndex;
					NI_VERIFY( nDeletedKey != nPosIndex, "Error movie in DB", return )
					if ( seq.posKeys[nPosKeyID].nPositionIndex > nDeletedKey )
					{
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].PositionIndex", nSeqID, nPosKeyID),
																																				(int)(nPosIndex - 1), pManipulator );
					}
				}
			}

			if ( bResult )
			{
				pObjectController->Redo( false, true, GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}


// creates new ScriptCamera position and adds new MOV_ED_DEF_LEN sec. length cyclic movie sequence
bool CScriptCameraState::AddSequence()
{
	bool bReturn = false;

	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			bool bResult = true;

			bResult = bResult && AddScriptPlacement();	//	sequence start camera
			const int nStartCameraIndex = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() - 1;
			bResult = bResult && AddScriptPlacement();	//	sequence finish camera
			const int nFinishCameraIndex = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() - 1;

			if ( bResult && pObjectController->AddInsertOperation(StrFmt("ScriptMovies.ScriptMovieSequences"), NODE_ADD_INDEX, pManipulator) )
			{
				if ( (nStartCameraIndex >= 0) && (nFinishCameraIndex >= 0) )
				{
					const int nSeqIndex = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size() - 1;
					if ( pObjectController->AddInsertOperation(StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys", nSeqIndex), NODE_ADD_INDEX, pManipulator) &&
							 pObjectController->AddInsertOperation(StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys", nSeqIndex), NODE_ADD_INDEX, pManipulator) )
					{
						// start
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[0].StartTime", nSeqIndex),
																																				(float)(0.0f), pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[0].PositionIndex", nSeqIndex),
																																				(int)nStartCameraIndex, pManipulator );
						// end
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[1].StartTime", nSeqIndex),
																																				(float)(MOV_ED_DEF_LEN), pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[1].PositionIndex", nSeqIndex),
																																				(int)nFinishCameraIndex, pManipulator );
					}
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
			bReturn = bResult;
		}
	}

	return bReturn;
}


bool CScriptCameraState::DeleteSequence( int nSeqIndex )
{
	bool bReturn = false;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage,
										 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
			{
				CArray1Bit listOfAllKeys;
				listOfAllKeys.SetSize( GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys.size() );
				for ( int i = 0; i < listOfAllKeys.GetSize(); ++i )
				{
					listOfAllKeys.SetData( i );
				}
				if ( DeleteKeys(listOfAllKeys, nSeqIndex, true) )
				{
					if ( pManipulator->RemoveNode("ScriptMovies.ScriptMovieSequences", nSeqIndex) )
						bReturn = true;
				}
			}
		}
	}
	return bReturn;
}


bool CScriptCameraState::AddScriptPlacement()
{
	bool bReturn = false;

	string szNewCameraName = std::to_string(  GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() );
	//CScriptCameraAddDlg dlg( Singleton<IMainFrameContainer>()->GetSECWorkbook(), &szNewCameraName, dialogData.scriptCameras );
	//if ( dlg.DoModal() == IDOK )
	{
		if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
			{
				if ( pObjectController->AddInsertOperation(StrFmt("ScriptMovies.ScriptCameraPlacements"), NODE_ADD_INDEX, pManipulator) )
				{
					int nCamerasCount = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size();

					NCamera::CCameraPlacement newCamera;
					newCamera.szName = szNewCameraName;
					GetCameraPlacement( &newCamera );

					bool bResult = true;
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Name", nCamerasCount - 1),
																																			(string)newCamera.szName, pManipulator );
					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Position", nCamerasCount - 1 ),
																																												newCamera.vPosition, pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Yaw", nCamerasCount - 1),
																																			(float)newCamera.fYaw, pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Pitch", nCamerasCount - 1),
																																			(float)newCamera.fPitch, pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].FOV", nCamerasCount - 1),
																																			(float)newCamera.fFOV, pManipulator );

					SetCameraPlacement( newCamera );

					if ( bResult )
					{
						pObjectController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pObjectController );
					}
					bReturn = bResult;
				}
			}
		}
	}
	return bReturn;
}


bool CScriptCameraState::UpdateScriptPlacement( int nCamera )
{
	bool bReturn = false;

	if ( !IsCameraPlacementInDB(nCamera) )
		return bReturn;

	NCamera::CCameraPlacement updatedCamera;
	GetCameraPlacement( &updatedCamera );

	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			bool bResult = true;
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Position", nCamera),
																																										updatedCamera.vPosition, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Yaw", nCamera),
																																	updatedCamera.fYaw, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].Pitch", nCamera),
																																	updatedCamera.fPitch, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptCameraPlacements.[%d].FOV", nCamera),
																																	updatedCamera.fFOV, pManipulator );

			if ( bResult )
			{
				pObjectController->Redo( false, true, GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
			bReturn = bResult;
		}
	}
	return bReturn;
}


bool CScriptCameraState::DeleteScriptPlacement( int nCamera )
{
	bool bReturn = false;

	if ( nCamera == -1 )
		return bReturn;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage,
										 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		if ( !IsCameraPlacementInDB(nCamera) )
			return bReturn;

		CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator();
		CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController();
		if ( pObjectController->AddRemoveOperation(StrFmt("ScriptMovies.ScriptCameraPlacements" ), nCamera, GetMapInfoEditor()->GetViewManipulator()) )
		{
			pObjectController->Redo( false, true, 0 );
			Singleton<IControllerContainer>()->Add( pObjectController );
			bReturn = true;
		}
	}
	return bReturn;
}


bool CScriptCameraState::AddPosKey( float fTime, int nSeqIndex )
{
	bool bReturn = false;

	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			bool bResult = true;

			AddScriptPlacement();
			const int nCameraIndex = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() - 1;

			if ( nCameraIndex >= 0 )
			{
				if ( (nSeqIndex >= 0) && (nSeqIndex < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size()) )
				{
					const vector<NDb::SScriptMovieKeyPos> &posKeys = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys;

					for ( int nKeyIndex = 0; nKeyIndex < posKeys.size(); ++nKeyIndex )
					{
						if ( fTime <= posKeys[nKeyIndex].fStartTime )
						{
							if ( pObjectController->AddInsertOperation(StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys", nSeqIndex), nKeyIndex, pManipulator) )
							{
								bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].StartTime", nSeqIndex, nKeyIndex),
																																						(float)(fTime), pManipulator );
								bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].PositionIndex", nSeqIndex, nKeyIndex),
																																						(int)(nCameraIndex), pManipulator );
							}
							if ( bResult )
							{
								pObjectController->Redo( false, true, GetMapInfoEditor() );
								Singleton<IControllerContainer>()->Add( pObjectController );
							}
							break;
						}
					}
					bReturn = bResult;
				}
			}
		}
	}
	return bReturn;
}


bool CScriptCameraState::KeySetup( const CArray1Bit &actKeys, int nSeqIndex )
{
	int nCount = 0;
	for ( int i = 0; i < actKeys.GetSize(); ++i )
	{
		if ( actKeys.GetData(i) )
		{
			++nCount;
		}
	}
	NI_VERIFY( nCount == 1, "Error selection!", return false )
	NI_VERIFY( (nSeqIndex >= 0) && (nSeqIndex < moviesData.scriptMoviesData.scriptMovieSequences.size()), "Invalid script sequence!", return false )
	NDb::SScriptMovieSequence &actSeq = moviesData.scriptMoviesData.scriptMovieSequences[nSeqIndex];

	for ( int i = actKeys.GetSize() - 1; i >= 0; --i )
	{
		if ( actKeys.GetData(i) )
		{
			NI_VERIFY( (i >= 0) && (i < actSeq.posKeys.size()), "Active key couldn't be found in DB!\n", return false )
			NDb::SScriptMovieKeyPos *pActKey = &(actSeq.posKeys[i]);
			string szKeyName = StrFmt( "Movie %d, Key %d", nSeqIndex, i );
      
			CMovEditorKeySettingsDlg dlg(	Singleton<IMainFrameContainer>()->GetSECWorkbook(), pActKey, &szKeyName );
			if ( dlg.DoModal() == IDOK )
			{
				if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
				{
					if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
					{
						bool bResult = true;
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].StartTime", nSeqIndex, i),
																																				(float)(pActKey->fStartTime), pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].PositionIndex", nSeqIndex, i),
																																				(int)(pActKey->nPositionIndex), pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].IsTangentIn", nSeqIndex, i),
																																				(bool)pActKey->bIsTangentIn, pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].IsTangentOut", nSeqIndex, i),
																																				(bool)pActKey->bIsTangentOut, pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].KeyParam", nSeqIndex, i),
																																				pActKey->szKeyParam, pManipulator );
						if ( bResult )
						{
							pObjectController->Redo( false, true, GetMapInfoEditor() );
							Singleton<IControllerContainer>()->Add( pObjectController );
						}
						return bResult;
					}
				}
			}
			//
			break;
		}
	}
	return false;
}


bool CScriptCameraState::DeleteKeys( const CArray1Bit &delList, int nSeqIndex, bool bDeleteWholeSequence )
{
	bool bReturn = false;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( bDeleteWholeSequence || (::MessageBox(Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage,
																						 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) )
	{
		//const float fMaxDiff = 0.1f;

		if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
			{
				if ( (nSeqIndex >= 0) && (nSeqIndex < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size()) )
				{
					const vector<NDb::SScriptMovieKeyPos> &posKeys = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys;

					bool bResult = true;

					const int nLast = bDeleteWholeSequence ? delList.GetSize() - 1 : delList.GetSize() - 2;
					const int nFirst = bDeleteWholeSequence ? 0 : 1;
					// can not delete the first and the last keys of the sequence
					for ( int i = nLast; i >= nFirst; --i )
					{
						if ( delList.GetData(i) )
						{
							int nDeletedIndex = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys[i].nPositionIndex;
							//
							bResult = bResult && pManipulator->RemoveNode( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys", nSeqIndex), i );
							if ( bResult )
							{
								FixDBMoviesIndexes( nDeletedIndex );
							}
						}
					}
					bReturn = bResult;
				}
			}
		}
	}
	return bReturn;
}


bool CScriptCameraState::SetSequenceLength( float fNewLength, int nSeqIndex )
{
	bool bReturn = false;

	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			if ( (nSeqIndex >= 0) && (nSeqIndex < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size()) )
			{
				const vector<NDb::SScriptMovieKeyPos> &posKeys = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys;

				vector<NDb::SScriptMovieKeyPos>::const_iterator itLastKey = posKeys.end();
				--itLastKey;

				if ( itLastKey->fStartTime <= fNewLength )
				{
					// just move the final key
					const int nLastKeyIndex = posKeys.size() - 1;
					if ( pObjectController->AddChangeOperation(StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].StartTime", nSeqIndex, nLastKeyIndex), (float)(fNewLength), pManipulator) )
					{
						pObjectController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pObjectController );
						bReturn = true;
					}
				}
				else
				{
					// need to delete some keys
					bool bResult = true;

					int nLastKeyIndex = 0;
					for ( nLastKeyIndex = 0; nLastKeyIndex < posKeys.size(); ++nLastKeyIndex )
					{
						if ( posKeys[nLastKeyIndex].fStartTime > fNewLength )
						{
							break;
						}
					}
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].StartTime", nSeqIndex, nLastKeyIndex), (float)(fNewLength), pManipulator );
					for ( int i = nLastKeyIndex + 1; i < posKeys.size(); ++i )
					{
						bResult = bResult && pManipulator->RemoveNode( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys", nSeqIndex), i );
					}

					if ( bResult )
					{
						pObjectController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pObjectController );
					}
					bReturn = bResult;
				}
			}
		}
	}
	return bReturn;
}


bool CScriptCameraState::MoveKeys( const CArray1Bit &moveList, float fMoveValue, int nSeqIndex )
{
	bool bReturn = false;

	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator() )
		{
			if ( (nSeqIndex >= 0) && (nSeqIndex < GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences.size()) )
			{
				vector<NDb::SScriptMovieKeyPos> posKeys = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptMovieSequences[nSeqIndex].posKeys;

				// change data copy
				int i = 0;
				for ( vector<NDb::SScriptMovieKeyPos>::iterator itKey = posKeys.begin(); itKey != posKeys.end(); ++itKey, ++i )
				{
					if ( i == 0 )
						continue;

					if ( moveList.GetData(i) )
					{
						itKey->fStartTime += fMoveValue;
						if ( itKey->fStartTime < 0.0f )
						{
							itKey->fStartTime = 0.0f;
						}
						for ( int j = i - 1; j >= 0; --j )
						{
							if ( posKeys[j].fStartTime > posKeys[j + 1].fStartTime )
							{
								NDb::SScriptMovieKeyPos posJ = posKeys[j];
								posKeys[j] = posKeys[j + 1];
								posKeys[j + 1] = posJ;
							}
							else
								break;
						}
					}
				}

				// set to DB (sorted)
				bool bResult = true;
				for ( int i = 0; i < posKeys.size(); ++i )
				{
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].StartTime", nSeqIndex, i),
																																			(float)(posKeys[i].fStartTime), pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].PositionIndex", nSeqIndex, i),
																																			(int)(posKeys[i].nPositionIndex), pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].IsTangentIn", nSeqIndex, i),
																																			(bool)(posKeys[i].bIsTangentIn), pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("ScriptMovies.ScriptMovieSequences.[%d].posKeys.[%d].IsTangentIn", nSeqIndex, i),
																																			(bool)(posKeys[i].bIsTangentOut), pManipulator );
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
				bReturn = bResult;
			}
		}
	}
	return bReturn;
}


void CScriptCameraState::Draw( CPaintDC *pPaintDC )
{
	return;

	sceneDrawTool.Clear();

	if ( dialogData.scriptCameras.empty() )
		return;	// nothing to draw
	
	if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
	{
		if ( !pMoviesHolder->IsStopped() )
			return;
	}
	if ( !bDrawMarkers )
		return;

	int i = 0;
	//
	for ( vector<NCamera::CCameraPlacement>::const_iterator it = dialogData.scriptCameras.begin(); it < dialogData.scriptCameras.end(); ++it, ++i )
	{
		// draw camera
		CVec3 vEyePos(it->vPosition);
		sceneDrawTool.DrawCircle( vEyePos, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
		if ( i == dialogData.nCurrentCamera )
		{
			sceneDrawTool.DrawCircle( vEyePos, SELECTION_RADIUS0, SELECTION_PARTS, SELECTION_COLOR, false );
			sceneDrawTool.DrawCircle( vEyePos, SELECTION_RADIUS1, SELECTION_PARTS, SELECTION_COLOR, false );
		}
		//
		CVec3 vNewDir( VNULL3 );
		CVec3 vDir = CVec3( sin(ToRadian(it->fYaw))*cos(ToRadian(it->fPitch)),
												-cos(ToRadian(it->fYaw))*cos(ToRadian(it->fPitch)), 
												sin(ToRadian(it->fPitch)) );

		if ( vDir.z != 0 )
			vNewDir = CVec3( vEyePos.z*(vDir.x/vDir.z), vEyePos.z*(vDir.y/vDir.z), vEyePos.z );

		sceneDrawTool.DrawLine( vEyePos, vEyePos - vNewDir, 0x0000FF00, false ); // green
	}

	sceneDrawTool.Draw();
}


void CScriptCameraState::PostDraw( CPaintDC *pPaintDC )
{
	return;

	if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
	{
		if ( !pMoviesHolder->IsStopped() )
		{
			return;
		}
	}
	if ( !bDrawMarkers )
		return;

	for ( vector<NCamera::CCameraPlacement>::const_iterator it = dialogData.scriptCameras.begin(); it < dialogData.scriptCameras.end(); ++it )
	{
		// draw label
		CVec3 vEyePos(it->vPosition);
		AI2Vis( &vEyePos );

		CPtr<ICamera> pCam = Camera();
		if ( !pCam )
			continue;

		CPtr<IEditorScene> pScene = EditorScene();
		CVec2 res = VNULL2;

		if ( TestRayInFrustrum( vEyePos, pCam->GetTransform(), EditorScene()->GetScreenRect(), &res ) )
			NDrawToolsDC::DrawLabelDC( pPaintDC, StrFmt("%s", it->szName), res );
	}
}


void CScriptCameraState::RefreshDialogData( bool bNeedUpdateFromDB )
{
	if ( bNeedUpdateFromDB )
	{
		GetDBData();
		//RefreshScriptCameraMarkers();
		SetMovie( moviesData.nActiveMovie );
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<uint32_t>(&dialogData) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MOVIES_EDITOR_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<uint32_t>(&moviesData) );

	float fDist;
	Camera()->GetPlacement( &fDist, &fPitch, &fYaw );
	nFOV = (int)( Camera()->GetFOV() );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_YAW, (uint32_t)(&fYaw) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_PITCH, (uint32_t)(&fPitch) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_FOV, (uint32_t)(&nFOV) );
}


void CScriptCameraState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	switch ( nChar )
	{
		case VK_SPACE: // save screen camera
		{
			if ( dialogData.nCurrentCamera == -1 )
				return;

			if ( UpdateScriptPlacement(dialogData.nCurrentCamera) )
				RefreshDialogData( true );

			NCamera::CCameraPlacement newCamera;
			GetCameraPlacementByID( &newCamera, dialogData.nCurrentCamera );
			SetCameraPlacement( newCamera );
			break;
		}
		//
		case VK_RETURN: // set camera
		{
			if ( dialogData.nCurrentCamera == -1 )
				return;

			NCamera::CCameraPlacement newCamera;
			GetCameraPlacementByID( &newCamera, dialogData.nCurrentCamera );
			SetCameraPlacement( newCamera );
			break;
		}
		//
		case VK_ADD:
		{
			ChangeCurrentFOV( -1.0 );
			break;
		}
		//
		case VK_SUBTRACT:
		{
			ChangeCurrentFOV( 1.0 );
			break;
		}
		//
		case VK_INSERT:
		{
			//if ( AddScriptPlacement() )
			//	RefreshDialogData( true );
			break;
		}
		//
		case VK_DELETE:
		{
			//if ( DeleteScriptPlacement(dialogData.nCurrentCamera) )
			//	RefreshDialogData( true );
			break;
		}
	}
}


bool CScriptCameraState::IsCameraPlacementInDB( int nCameraIndex )
{
	int nCamerasCount = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size();
	return ( (nCamerasCount > 0) && (nCameraIndex >= 0) && (nCameraIndex < nCamerasCount) );
}


void CScriptCameraState::GetDBData()
{
	dialogData.scriptCameras.clear();

	if ( GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() <= 0 )
	{
		dialogData.nCurrentCamera = -1;
	}
	else
	{
		if ( dialogData.nCurrentCamera >= GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() )
			dialogData.nCurrentCamera = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.size() - 1;

		for ( vector<NDb::SScriptCameraPlacement>::const_iterator itCamPos = GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.begin();
																															itCamPos != GetMapInfoEditor()->pMapInfo->scriptMovies.scriptCameraPlacements.end(); ++itCamPos )
		{
			const NDb::SScriptCameraPlacement &camPos = (*itCamPos);

			NCamera::CCameraPlacement newCamera( camPos.vPosition, camPos.fYaw, camPos.fPitch, camPos.fFOV, 0.0f );
			newCamera.szName = camPos.szName;

			dialogData.scriptCameras.push_back( newCamera );
		}
	}


	moviesData.Reset();
	moviesData.scriptMoviesData = GetMapInfoEditor()->pMapInfo->scriptMovies;
	if ( moviesData.nActiveMovie < 0 )
		moviesData.nActiveMovie = 0;
	if ( moviesData.nActiveMovie >= moviesData.scriptMoviesData.scriptMovieSequences.size() )
		moviesData.nActiveMovie = moviesData.scriptMoviesData.scriptMovieSequences.size() - 1;
}


void CScriptCameraState::GetCameraPlacementByID( NCamera::CCameraPlacement *pCamera, int nID )
{
	int i = 0;
	for ( vector<NCamera::CCameraPlacement>::const_iterator it = dialogData.scriptCameras.begin(); it < dialogData.scriptCameras.end(); ++it, ++i )
	{
		if ( i == nID )
		{
			(*pCamera) = (*it);
		}
	}
}


void CScriptCameraState::DeleteScriptCameraMarkers()
{
	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pScene )
		return;
	for ( vector<SCameraMarker>::const_iterator it = cameraMarkers.begin(); it < cameraMarkers.end(); ++it )
	{
		pScene->RemoveObject( (*it).nMarkerID ); 
	}
	cameraMarkers.clear();
}


void CScriptCameraState::RefreshScriptCameraMarkers()
{
	DeleteScriptCameraMarkers();

	CPtr<IEditorScene > pScene = EditorScene();
	if ( !pScene )
		return;
	CPtr<IManipulator> pManipulator = NEditorOptions::CreateOptionsManipulator();
	if ( !pManipulator )
		return;

	string szModelName;
	if ( !CManipulatorManager::GetParamsFromReference( "UtilityModels.CameraModel", pManipulator, 0, &szModelName, 0 ) )
		return;
	const NDb::SModel *pModel = NDb::Get<const NDb::SModel>( CDBID( szModelName ) );
	if ( !pModel )
		return;

	int i = 0;	
	cameraMarkers.clear();
	if ( dialogData.scriptCameras.size() == 0 )
		return;

	for ( vector<NCamera::CCameraPlacement>::const_iterator it = dialogData.scriptCameras.begin(); it < dialogData.scriptCameras.end(); ++it, ++i )
	{
		if ( it->fFOV <= 0 )
		{
			NLog::Log( LT_ERROR, "Camera entry (%d) with zero FOV. Skipping.", i );
			continue;
		}
		//
		CVec3 vPos = it->vPosition;
		CVec3 vDir = CVec3( sin(ToRadian(it->fYaw))*cos(ToRadian(it->fPitch)),
												-cos(ToRadian(it->fYaw))*cos(ToRadian(it->fPitch)), 
												sin(ToRadian(it->fPitch)) );
		vDir = CVec3( -vDir.y, vDir.x, 0 ); // rotate new vDir 90 degree over OZ
		Normalize( &vDir );

		CQuat qRotation = CQuat( ToRadian(it->fYaw), V3_AXIS_Z );
		qRotation *= CQuat( ToRadian(-90.0 - it->fPitch), vDir );

		CVec3 vScale( 0.8*tan(ToRadian(it->fFOV/2)), 0.8*tan(ToRadian(it->fFOV/2)), 1);
		vScale *= 2;

		int nObjectSceneID = pScene->AddObject( INVALID_NODE_ID, pModel, vPos, qRotation, vScale, OBJ_ANIM_MODE_DEFAULT, 0 );
		if ( nObjectSceneID != INVALID_NODE_ID )
		{
			SCameraMarker newMarker;
			newMarker.nCameraID = i;
			newMarker.nMarkerID = nObjectSceneID;
      cameraMarkers.push_back( newMarker );
		}
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CScriptCameraState::ChangeCurrentFOV( float fFOVdelta )
{
	const float fFOVeps = 0.1f;
	float fNewFOV = Camera()->GetFOV() + fFOVdelta;

	if ( fNewFOV >= 180.0f )
		fNewFOV = 180.0f - fFOVeps;
	else if ( fNewFOV < fFOVeps )
		fNewFOV = fFOVeps;

	Camera()->SetFOV( fNewFOV );
	nFOV = (int)( fNewFOV );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_WINDOW, ID_SCRIPT_CAMERA_SET_FOV, (uint32_t)( &nFOV ) );
}


void CScriptCameraState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	return;
  
	if ( CPtr<IEditorScene> pScene = EditorScene() )
	{
		list<int> sceneIDList;
		pScene->PickObjects( sceneIDList, CVec2(rMousePoint.x, rMousePoint.y) );

		if ( !sceneIDList.empty() )
		{
			for ( list<int>::const_iterator itObj = sceneIDList.begin(); itObj != sceneIDList.end(); ++itObj )
			{
				int nObjID = (*itObj);
				if ( (dialogData.nCurrentCamera != -1) && (cameraMarkers.size() > dialogData.nCurrentCamera) )
					if ( cameraMarkers[dialogData.nCurrentCamera].nMarkerID == nObjID )
						continue;

				for ( vector<SCameraMarker>::const_iterator it = cameraMarkers.begin(); it < cameraMarkers.end(); ++it )
				{
					if ( (*it).nMarkerID == nObjID )
					{
						dialogData.nCurrentCamera = (*it).nCameraID;
						RefreshDialogData( false );
						return;
					}
				}
			}
		}
		else
		{
			dialogData.nCurrentCamera = -1;
			RefreshDialogData( false );
		}
	}
}


void CScriptCameraState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	return;

	if ( CPtr<IEditorScene> pScene = EditorScene() )
	{
		list<int> sceneIDList;
		pScene->PickObjects( sceneIDList, CVec2(rMousePoint.x, rMousePoint.y) );

		if ( !sceneIDList.empty() )
		{
			for ( list<int>::const_iterator itObj = sceneIDList.begin(); itObj != sceneIDList.end(); ++itObj )
			{
				int nObjID = (*itObj);

				for ( vector<SCameraMarker>::const_iterator it = cameraMarkers.begin(); it < cameraMarkers.end(); ++it )
				{
					if ( (*it).nMarkerID == nObjID )
					{
						dialogData.nCurrentCamera = (*it).nCameraID;
						NCamera::CCameraPlacement newCamera;
						GetCameraPlacementByID( &newCamera, dialogData.nCurrentCamera );
						SetCameraPlacement( newCamera );
						return;
					}
				}
			}
			dialogData.nCurrentCamera = -1;
		}
	}
}


void CScriptCameraState::ScriptCameraRun( NDb::EScriptCameraRunType eRunType )
{
	NI_VERIFY( !(dialogData.scriptCameras.empty()), "CScriptCameraState::ScriptCameraRun - No cameras in scene!\n", return );

	runDialogData.scriptCameras = dialogData.scriptCameras;
	CScriptCameraRunDlg dlg(	Singleton<IMainFrameContainer>()->GetSECWorkbook(), &runDialogData );

	if ( dlg.DoModal() == IDOK )
	{
		const NTimer::STime timeStart = Singleton<IGameTimer>()->GetGameTime();
		CCSTime *pTimer = EditorScene()->GetGameTimer();

		NDb::SScriptMovies moviesData;
		moviesData.scriptCameraPlacements.clear();
		moviesData.scriptMovieSequences.clear();

		NCamera::CCameraPlacement startCamera = runDialogData.GetStartCamera();
		NCamera::CCameraPlacement finishCamera = runDialogData.GetFinishCamera();

		NDb::SScriptMovieSequence seq;
		seq.followKeys.clear();
		seq.posKeys.clear();

		NDb::SScriptMovieKeyPos startPos;
		NDb::SScriptMovieKeyPos finishPos;

		startPos.fStartTime = 0;
		startPos.nPositionIndex = 0;
		finishPos.fStartTime = runDialogData.fTime;
		finishPos.nPositionIndex = 1;

		seq.posKeys.push_back( startPos );
		seq.posKeys.push_back( finishPos );

		moviesData.scriptMovieSequences.push_back( seq );

		NDb::SScriptCameraPlacement startCamPlacement;
		NDb::SScriptCameraPlacement finishCamPlacement;

		startCamPlacement.fYaw = startCamera.fYaw;
		startCamPlacement.fPitch = startCamera.fPitch;
		startCamPlacement.fFOV = startCamera.fFOV;
		startCamPlacement.vPosition = startCamera.vPosition;

		finishCamPlacement.fYaw = finishCamera.fYaw;
		finishCamPlacement.fPitch = finishCamera.fPitch;
		finishCamPlacement.fFOV = finishCamera.fFOV;
		finishCamPlacement.vPosition = finishCamera.vPosition;

		moviesData.scriptCameraPlacements.push_back( startCamPlacement );
		moviesData.scriptCameraPlacements.push_back( finishCamPlacement );

		CScriptMoviesMutatorHolder *pMoviesHolder = new CScriptMoviesMutatorHolder( moviesData, 0, pTimer );
		pMoviesHolder->SetTime( 0.0f );
		pMoviesHolder->SetSpeed( 1.0f );
		pMoviesHolder->Play();
		Camera()->SetScriptMutatorsHolder( pMoviesHolder );
	}
}


void CScriptCameraState::SetCameraPlacement( const NCamera::CCameraPlacement &rCameraPlacement )
{
	if ( CObj<ICamera> pCamera = Camera() )
	{
		if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
			pMoviesHolder->Stop();

		CVec3 vCamNewAnchor = rCameraPlacement.GetAnchor();
		AI2Vis( &vCamNewAnchor );

		pCamera->SetAnchor( vCamNewAnchor );
		pCamera->SetFOV( rCameraPlacement.fFOV );
		pCamera->SetPlacement( AI2Vis(rCameraPlacement.GetDistance()), rCameraPlacement.fPitch, rCameraPlacement.fYaw );

		RefreshDialogData( false );
	}
}


void CScriptCameraState::SetMovie( int nMovieIndex )
{
	const NDb::SScriptMovies &dbMoviesData = GetMapInfoEditor()->pMapInfo->scriptMovies;
	if ( (nMovieIndex < 0) || (nMovieIndex >= dbMoviesData.scriptMovieSequences.size()) )
	{
		Camera()->SetScriptMutatorsHolder( 0 );
		return;
	}

	CCSTime *pTimer = EditorScene()->GetGameTimer();
	CScriptMoviesMutatorHolder *pMutatorsHolder = new CScriptMoviesMutatorHolder( (GetMapInfoEditor()->pMapInfo->scriptMovies), nMovieIndex, pTimer );
	Camera()->SetScriptMutatorsHolder( pMutatorsHolder );
}


int CScriptCameraState::SelectCameraByMovieParams( int nMovieID, float fTime ) const
{
	NI_VERIFY( (nMovieID >= 0)  && (nMovieID < moviesData.scriptMoviesData.scriptMovieSequences.size()), "Error getting movie", return -1 )

	const NDb::SScriptMovieSequence &movie = moviesData.scriptMoviesData.scriptMovieSequences[nMovieID];
	for ( vector<NDb::SScriptMovieKeyPos>::const_iterator itKey = movie.posKeys.begin(); itKey != movie.posKeys.end(); ++itKey )
  {
		if ( fabs(itKey->fStartTime - fTime) <= DEF_SEL_RAD )
			return itKey->nPositionIndex;
  }
	return -1;
}



