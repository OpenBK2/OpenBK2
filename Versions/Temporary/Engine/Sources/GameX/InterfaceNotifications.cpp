#include "stdafx.h"
#include "InterfaceNotifications.h"
#include "ScenarioTracker.h"
#include "InterfaceState.h"
#include "Sound/SoundScene.h"
#include "DBGameRoot.h"
#include "WorldClient.h"
#include "GameXClassIDs.h"
#include "B2_M1_World/MissionObjectiveStates.h"
#include "InterfaceMisc.h"
#include "Utils.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "AILogic/B2AI.h"
#include "Stats_B2_M1/DBClientConsts.h"
#include "System/Text.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "InterfaceArmyBranchDlg.h"
#include "SaveLoadHelper.h"
#include "System/Commands.h"
#include "Misc/StrProc.h"

#include "GameX_export.h"

#include <fmt/format.h>

static bool s_bAutosaveObjectiveComplete = false;

const float FADE_DELTA_TIME = 1.0f;
const float NEW_REINFORCEMENT_TIME = 10.0f;
const float NEW_REINFORCEMENT_TIME_STEP = 1.0f;
const float OBJECTIVES_NOTIFY_REPEAT = 60 * 1000; // (msec)

const float TEXTURE_POINT_X = 4.0f; 
const float TEXTURE_POINT_Y = 7.0f;
const float KEY_OBJECT_TEXTURE_POINT_X = 8.0f; 
const float KEY_OBJECT_TEXTURE_POINT_Y = 6.0f;

const int EVENT_ITEMS_INTERVAL = 5;
const char* EVENT_NAME_PREFIX = "Btn";

const int CLIENT_UNIQUE_ID_MAP_POINTER = -10000; // look for other CLIENT_UNIQUE_ID_xxx (криво, но менять поздно)

#define MP_MARKER_POINTER_ID 1000
#define MP_MARKER_EXPIRE 120000

// CVisualNotifications::SMapPointer

CVisualNotifications::SMapPointer::~SMapPointer()
{
	Remove();
}

void CVisualNotifications::SMapPointer::Init( int _nUniqueID, const CVec3 &_vPos )
{
	nUniqueID = _nUniqueID;
	vPos = _vPos;

	bPlaced = false;

	const NDb::SClientGameConsts *pClientConsts = NGameX::GetClientConsts();
	const NDb::SVisObj *pVisObj = (pClientConsts != 0) ? pClientConsts->pMapPointer : 0;
	const NDb::SModel *pModel = (pVisObj != 0 && !pVisObj->models.empty()) ? pVisObj->models.front().pModel : 0;
	
	if ( !pModel )
		return;
		
	Scene()->AddObject( nUniqueID, pModel, vPos, 
		QNULL, CVec3(1, 1, 1), OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC, 0 );
	if ( pModel->pSkeleton != 0 ) 
	{
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nUniqueID ) )
		{
			for ( int i = 0; i < pModel->pSkeleton->animations.size(); ++i )
			{
				const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2*>( pModel->pSkeleton->animations[i] );
				if ( pAnim->eType == NDb::ANIMATION_IDLE )
				{
					AddAnimation( pAnim, Singleton<IGameTimer>()->GetGameTime(), pAnimator, pAnim->bLooped );
					break;
				}
			}
		}
	}
	bPlaced = true;
}

void CVisualNotifications::SMapPointer::Remove()
{
	if ( bPlaced )
	{
		Scene()->RemoveObject( nUniqueID );
		bPlaced = false;
	}
}

void CVisualNotifications::SMapPointer::Move( const CVec3 &vPos )
{
	if ( bPlaced )
	{
		CQuat q;
		q.FromAngleAxis( 0.0f, CVec3( 0.0f, 0.0f, 1.0f ) );
		Scene()->MoveObject( nUniqueID, vPos, q );
	}
}

// CVisualNotifications

CVisualNotifications::CVisualNotifications()
{
	InitPrivate();
}

CVisualNotifications::CVisualNotifications( IWindow *pParent, IMiniMap *_pMiniMap, const CVec2 &_vMapSize, IAILogic *_pAI )
{
	InitPrivate();

	pMiniMap = _pMiniMap;
	vMapSize = _vMapSize;
	pAI = _pAI;

	nSelectedID = -1;
	bShowObjectives = false;
	bNewObjective = false;
	fNewObjectiveTime = 0.0f;
	nSelectedKeyObject = -1;
	
	InitEvents( pParent );

	if ( NGlobal::GetVar("game_mode_editor", 0) == 0 )
	{
		const NDb::SGameRoot *pRoot = InterfaceState()->GetGameRoot();
		if ( pRoot )
		{
			for ( std::vector< CDBPtr< NDb::SNotification > >::const_iterator it = pRoot->notifications.begin();
				it != pRoot->notifications.end(); ++it )
			{
				const NDb::SNotification *pNotification = *it;
				if ( !pNotification )
					continue;

				entries[pNotification->eType] = pNotification;
			}
		}
	}
}

void CVisualNotifications::InitPrivate()
{
	textMessageTimes.resize( NDb::NTF_COUNT, 0 );

	nAbsTime = 0;
	bEventTimerStopped = true;
	
	nLastFreeID = CLIENT_UNIQUE_ID_MAP_POINTER;
	bShowMPMarker = false;
}

bool CVisualNotifications::Notify( EVisualNotification eType, int nID, const CVec2 &vPos )
{
	switch ( eType )
	{
		case EVNT_CAPTURE_KEY_OBJECT:
			return OnCaptureKeyObject( nID, vPos );
		
		case EVNT_LOSS_KEY_OBJECT:
			return OnLossKeyObject( nID, vPos );
			
		case EVNT_RECEIVE_OBJECTIVE:
			return OnReceiveObjective( nID, vPos );
			
		case EVNT_COMPLETE_OBJECTIVE:
			return OnCompleteObjective( nID, vPos );

		case EVNT_FAIL_OBJECTIVE:
			return OnFailObjective( nID, vPos );
			
		case EVNT_REMOVE_OBJECTIVE:
			return OnRemoveObjective( nID, vPos );

		case EVNT_SELECT_OBJECTIVE:
			return OnSelectObjective( nID, vPos );
		
		case EVNT_CHECK_OBJECTIVES:
			return OnCheckObjectives( nID, vPos );

		case EVNT_SHOW_OBJECTIVES:
			return OnShowObjectives( nID, vPos );

		case EVNT_HIDE_OBJECTIVES:
			return OnHideObjectives( nID, vPos );
			
//		case EVNT_CHECK_OBJECTIVE_NOTIFY:
//			return OnCheckObjectiveNotify( nID, vPos );

		case EVNT_ENEMY_ARTILLERY_SEEN:
			return OnArtillerySeen( nID, vPos );

		case EVNT_ENEMY_AA_SEEN:
			return OnAASeen( nID, vPos );
			
		case EVNT_REINFORCEMENT_AVAILABLE:
			return OnReinforcementAvailable( nID, vPos );

		case EVNT_REINFORCEMENT_ARRIVED:
			return OnReinforcementArrived( nID, vPos );

		case EVNT_SELECT_KEY_POINT:
			return OnSelectKeyPoint( nID );
		
		case EVNT_CAMERA_BACK:
			return OnCameraBack();
			
		case EVNT_UPDATE_OBJECTIVE:
			return OnUpdateObjective( nID );

		case EVNT_UNITS_GIVEN:
			return OnUnitsGiven( nID, vPos );
	};
	return false;
}

bool CVisualNotifications::Notify( EVisualNotification eType, CMapObj *pMO )
{
	switch ( eType )
	{
		case EVNT_KEY_OBJECT_STATE:
			return OnKeyObjectState( pMO );
	};

	return false;
}

void CVisualNotifications::Step( const NTimer::STime nDeltaGameTime, bool bAppActive )
{
	NTimer::STime nAbsCurrTime = Singleton<IGameTimer>()->GetAbsTime();
	float fDeltaAbsTime = (nAbsTime == 0) ? 0.0f : max( 0.0f, (float)(nAbsCurrTime - nAbsTime) / 1000.0f );

	StepAbs( fDeltaAbsTime, bAppActive );
	nAbsTime = nAbsCurrTime;

	bool bEventTimerActive = bAppActive && !GameTimer()->HasPause( PAUSE_TYPE_USER_PAUSE ) &&
		!GameTimer()->HasPause( PAUSE_TYPE_INACTIVE ) &&
		!GameTimer()->HasPause( PAUSE_TYPE_INTERFACE );
	if ( bEventTimerActive && !bEventTimerStopped )
		UpdateEvents( (float)( nDeltaGameTime ) / 1000.0f );
	else
		UpdateEvents( 0.0f );

	if ( bShowMPMarker && nAbsCurrTime > timeMarkerExpire )
	{
		bShowMPMarker = false;
		RemoveObjectivePointers( MP_MARKER_POINTER_ID );
	}
	bEventTimerStopped = !bEventTimerActive;
}

void CVisualNotifications::StepAbs( float fDeltaTime, bool bAppActive )
{
	std::vector< IMiniMap::SFigure > figures;
	for ( std::list< SNotification >::iterator it = notifications.begin(); it != notifications.end(); )
	{
		SNotification &notification = *it;
		
		bool bFinished = false;
		if ( notification.fTimeRemain >= 0.0f )
		{
			notification.fTimeRemain -= fDeltaTime;
			if ( notification.fTimeRemain <= 0.0f )
				bFinished = true;
		}
		notification.fSize -= notification.fSpeed * fDeltaTime;
		if ( notification.fSize < 0.0f )
			bFinished = true;
		notification.fAngle += notification.fRotationSpeed * fDeltaTime;
		
		if ( !bFinished )
		{
			figures.push_back( IMiniMap::SFigure() );
			IMiniMap::SFigure &figure = figures.back();
			figure.eType = notification.eFigure;
			figure.vPos = notification.vPos;
			figure.fSize = notification.fSize;
			figure.fAngle = notification.fAngle;
			figure.color = notification.color;
		}
		
		if ( bFinished )
			it = notifications.erase( it );
		else
			++it;
	}
	pMiniMap->SetFigures( figures );
	
	fNewObjectiveTime -= fDeltaTime;
	if ( fNewObjectiveTime <= 0.0f )
	{
		fNewObjectiveTime = FADE_DELTA_TIME;
		if ( bNewObjective )
			NInput::PostEvent( "fade_show_objectives_button", 0, 0 );
	}

	UpdateMarkers();
}

void CVisualNotifications::AddNotification( int nID, const CVec2 &vPos, NDb::ENotificationType eType )
{
	static std::wstring wszEmpty;
	AddNotificationMain( nID, vPos, eType, wszEmpty );
	AddNotificationMinimap( nID, vPos, eType );
}

void CVisualNotifications::AddNotificationMain( int nID, const CVec2 &vPos, NDb::ENotificationType eType, 
	const std::wstring &wszCustomText )
{
	CEntries::const_iterator it = entries.find( eType );
	if ( it == entries.end() )
		return;

	const NDb::SNotification *pEntry = it->second;

	if ( pEntry->pSound )
		SoundScene()->AddSound( pEntry->pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 2 );

	if ( entries[eType]->fDuplicateDelay == 0.0f || textMessageTimes[eType] == 0 ||
		GameTimer()->GetAbsTime() - textMessageTimes[eType] > entries[eType]->fDuplicateDelay )
	{
		// Ignore duplicate messages
		
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,Text) )
		{
			InterfaceState()->WriteToMissionConsole( GET_TEXT_PRE(pEntry->,Text) + wszCustomText );
			textMessageTimes[eType] = GameTimer()->GetAbsTime();
		}
	}
}		

void CVisualNotifications::AddNotificationMinimap( int nID, const CVec2 &vPos, NDb::ENotificationType eType )
{
	CEntries::const_iterator it = entries.find( eType );
	if ( it == entries.end() )
		return;

	const NDb::SNotification *pEntry = it->second;

	notifications.push_back( SNotification() );
	SNotification &notification = notifications.back();
	notification.nID = nID;
	notification.eFigure = pEntry->eFigureType;
	notification.vPos = vPos;
	notification.fSize = ( pEntry->fRotationSpeed > 0 ) ? pEntry->fSize * (vMapSize.x + vMapSize.y) * 0.5f : 0.0f;
	notification.fAngle = 0.0f;
	notification.color = NGfx::SPixel8888( pEntry->nColor );
	notification.fTimeRemain = pEntry->fTime;
	notification.fSpeed = ( ( pEntry->fRotationSpeed > 0 ) ? 1 : -1 ) * pEntry->fSize * (vMapSize.x + vMapSize.y) * 0.5f / notification.fTimeRemain;
	notification.fRotationSpeed = ToRadian( pEntry->fRotationSpeed );
}

void CVisualNotifications::AddObjectivePointers( int nID )
{
	const NDb::SMissionObjective *pObjective = GetObjective( nID );
	if ( !pObjective )
		return;
		
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST->IsDynamicObjective( nID ) )
		return;
	
	std::vector<CVec3> places;
	pST->GetObjectivePlaces( nID, &places );

	AddPointerModels( nID, places );
}

void CVisualNotifications::AddPointerModels( int nID, const std::vector<CVec3> &places )
{
	std::unordered_map<int,SMapPointers>::iterator it = pointers.find( nID );
	if ( it != pointers.end() )
		return;

	it = pointers.insert( std::pair<int, SMapPointers>( nID, SMapPointers() ) ).first;
	std::vector<SMapPointer> &mapPointers = it->second;

	mapPointers.resize( places.size() );
	for ( int i = 0; i < mapPointers.size(); ++i )
	{
		SMapPointer &pointer = mapPointers[i];
		int nUniqueID;
		if ( !freeIDs.empty() )
		{
			nUniqueID = freeIDs.back();
			freeIDs.pop_back();
		}
		else
		{
			nUniqueID = nLastFreeID;
			nLastFreeID++;
		}
		pointer.Init( nUniqueID, places[i] );
	}
}

void CVisualNotifications::RemoveObjectivePointers( int nID )
{
	std::unordered_map<int,SMapPointers>::iterator it = pointers.find( nID );
	if ( it == pointers.end() )
		return;

	std::vector<SMapPointer> &mapPointers = it->second;
	for ( int i = 0; i < mapPointers.size(); ++i )
	{
		SMapPointer &pointer = mapPointers[i];
		freeIDs.push_back( pointer.nUniqueID );
		pointer.Remove();
	}
	pointers.erase( it );
}

void CVisualNotifications::UpdateObjectivePointers( int nID )
{
	const NDb::SMissionObjective *pObjective = GetObjective( nID );
	if ( !pObjective )
		return;

	std::unordered_map<int,SMapPointers>::iterator it = pointers.find( nID );
	if ( it == pointers.end() )
		return;
		
	std::vector<SMapPointer> &mapPointers = it->second;
	std::vector<CVec3> places;
	Singleton<IScenarioTracker>()->GetObjectivePlaces( nID, &places );

	if ( places.size() != mapPointers.size() )
	{
		RemoveObjectivePointers( nID );
		if ( !places.empty() )
			AddObjectivePointers( nID );
	}

	it = pointers.find( nID );
	if ( it == pointers.end() )
		return;
	std::vector<SMapPointer> &mapPointers2 = it->second;
	if ( places.size() == mapPointers2.size() )
	{
		for ( int i = 0; i < places.size(); ++i )
		{
			SMapPointer &pointer = mapPointers2[i];
			pointer.Move( places[i] );
		}
	}
}

void CVisualNotifications::AddObjectiveNotification( int nID, NDb::ENotificationType eType )
{
	const NDb::SMissionObjective *pObjective = GetObjective( nID );
	if ( !pObjective )
		return;
	//

	std::wstring wszCustomText;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pObjective->,Header) )
		wszCustomText = GET_TEXT_PRE(pObjective->,Header);
	AddNotificationMain( nID, VNULL2, eType, wszCustomText );
	
	for ( std::vector< CVec2 >::const_iterator it = pObjective->mapPositions.begin();
		it != pObjective->mapPositions.end(); ++it )
	{
		AddNotificationMinimap( nID, *it, eType );
	}
}

void CVisualNotifications::NewObjective( bool bChecked )
{
	bNewObjective = bChecked;
}

bool CVisualNotifications::OnCaptureKeyObject( int nID, const CVec2 &vPos )
{
	AddNotification( nID, vPos, NDb::NTF_KEY_OBJECT_CAPTURED );

	SEventParams params;
	params.eEventType = NDb::NEVT_KEY_POINT_CAPTURED;
	params.nID = nID;
	params.positions.push_back( vPos );
	AddEvent( params );
	
	RemoveEvent( NDb::NEVT_KEY_POINT_LOST, nID );

	return true;
}

bool CVisualNotifications::OnArtillerySeen( int nID, const CVec2 &vPos )
{
	AddNotification( nID, vPos, NDb::NTF_ENEMY_ARTILLERY );

/*	SEventParams params; // CRAP
	params.eEventType = NDb::NEVT_ENEMY_ART_DETECTED;
	params.nID = nID;
	params.positions.push_back( vPos );
	AddEvent( params );*/

	return true;
}

bool CVisualNotifications::OnAASeen( int nID, const CVec2 &vPos )
{
	AddNotification( nID, vPos, NDb::NTF_ENEMY_AA_FIRE );

/*	SEventParams params; // CRAP
	params.eEventType = NDb::NEVT_ENEMY_AA_DETECTED;
	params.nID = nID;
	params.positions.push_back( vPos );
	AddEvent( params );*/

	return true;
}

bool CVisualNotifications::OnReinforcementAvailable( int nID, const CVec2 &vPos )
{
	if ( const NDb::SComplexSoundDesc *pSound = InterfaceState()->GetSoundEntry( "SOUND_REINF_AVAILABLE" ) )
		SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 2 );

	return true;
}

bool CVisualNotifications::OnReinforcementArrived( int nID, const CVec2 &vPos )
{
	AddNotification( nID, vPos, NDb::NTF_REINFORCEMENT_ARRIVED );

	return true;
}

bool CVisualNotifications::OnUnitsGiven( int nID, const CVec2 &vPos )
{
	//AddNotification( nID, vPos, NDb::NTF_UNITS_GIVEN );

	SEventParams params;
	params.eEventType = NDb::NEVT_UNITS_GIVEN;
	params.nID = nID;
	params.positions.push_back( vPos );
	AddEvent( params );

	return true;
}

bool CVisualNotifications::OnSelectKeyPoint( int nID )
{
	nSelectedKeyObject = nID;

	return true;
}

bool CVisualNotifications::OnLossKeyObject( int nID, const CVec2 &vPos )
{
	AddNotification( nID, vPos, NDb::NTF_KEY_OBJECT_LOSED );

	SEventParams params;
	params.eEventType = NDb::NEVT_KEY_POINT_LOST;
	params.nID = nID;
	params.positions.push_back( vPos );
	AddEvent( params );

	RemoveEvent( NDb::NEVT_KEY_POINT_CAPTURED, nID );

	return true;
}

bool CVisualNotifications::OnKeyObjectState( CMapObj *pMO )
{
	keyObjects[pMO->GetID()] = pMO;
	return true;
}

bool CVisualNotifications::OnReceiveObjective( int nID, const CVec2 &vPos )
{
//	AddNewObjectiveNotify( nID );
	AddObjectiveNotification( nID, NDb::NTF_OBJECTIVE_RECEIVED );
	AddObjectivePointers( nID );
	NewObjective( true );

	NInput::PostEvent( "blink_objective_button", 1, 0 );

	UpdateMarkers();
	
	NInput::PostEvent( "mission_objectives_changed", 0, 0 );

	SEventParams params;
	params.eEventType = NDb::NEVT_OBJECTIVE_RECEIVED;
	params.nID = nID;
	AddEvent( params );

	return true;
}

bool CVisualNotifications::OnCompleteObjective( int nID, const CVec2 &vPos )
{
	AddObjectiveNotification( nID, NDb::NTF_OBJECTIVE_COMPLETED );
	RemoveObjectivePointers( nID );
	NewObjective( true );

	UpdateMarkers();

	NInput::PostEvent( "mission_objectives_changed", 0, 0 );

	SEventParams params;
	params.eEventType = NDb::NEVT_OBJECTIVE_COMPLETED;
	params.nID = nID;
	AddEvent( params );
	// make autosave on objective complete
	if ( s_bAutosaveObjectiveComplete )
		NSaveLoad::MakeUniqueSave( InterfaceState()->GetTextEntry("T_AUTO_SAVE_OBJECTIVE_COMPLETE"), true, false, false );

	return true;
}

bool CVisualNotifications::OnFailObjective( int nID, const CVec2 &vPos )
{
	AddObjectiveNotification( nID, NDb::NTF_OBJECTIVE_FAILED );
	RemoveObjectivePointers( nID );
	NewObjective( true );

	UpdateMarkers();
	
	NInput::PostEvent( "mission_objectives_changed", 0, 0 );

	SEventParams params;
	params.eEventType = NDb::NEVT_OBJECTIVE_FAILED;
	params.nID = nID;
	AddEvent( params );

	return true;
}

bool CVisualNotifications::OnRemoveObjective( int nID, const CVec2 &vPos )
{
	RemoveObjectivePointers( nID );

	UpdateMarkers();
	
	NInput::PostEvent( "mission_objectives_changed", 0, 0 );

	return true;
}

bool CVisualNotifications::OnSelectObjective( int nID, const CVec2 &vPos )
{
	nSelectedID = nID;
	
	return true;
}

bool CVisualNotifications::OnCheckObjectives( int nID, const CVec2 &vPos )
{
	NewObjective( false );
	return true;
}

bool CVisualNotifications::OnShowObjectives( int nID, const CVec2 &vPos )
{
	bShowObjectives = true;
	UpdateMarkers();

	return true;
}

bool CVisualNotifications::OnHideObjectives( int nID, const CVec2 &vPos )
{
	bShowObjectives = false;
	UpdateMarkers();

	return true;
}

void CVisualNotifications::UpdateMarkers()
{
	if ( !Singleton<IScenarioTracker>() )
		return;
	if ( !pMiniMap || NGlobal::GetVar("game_mode_editor", 0) != 0 )
		return;
		
	std::vector<IMiniMap::SMarker> markers;

	const NDb::STexture *pNeutral = InterfaceState()->GetTextureEntry( "TX_MINIMAP_KEY_OBJECT_NEUTRAL" );
	const NDb::STexture *pFriend = InterfaceState()->GetTextureEntry( "TX_MINIMAP_KEY_OBJECT_FRIEND" );
	const NDb::STexture *pEnemy = InterfaceState()->GetTextureEntry( "TX_MINIMAP_KEY_OBJECT_ENEMY" );
	const NDb::STexture *pSelected = InterfaceState()->GetTextureEntry( "TX_MINIMAP_KEY_OBJECT_FRIEND_SELECTED" );

	// For multiplayer - select different flags from MPConsts
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
/*	if ( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		int nFriendSide = pScenarioTracker->GetPlayerSide( pScenarioTracker->GetLocalPlayer() );
		int nEnemySide = ( nFriendSide != 2 ) ? 1 - nFriendSide : 2;

		pNeutral = pMPConsts->diplomacyInfo[2]->pMinimapKeyObjectIcon;
		pFriend = pMPConsts->diplomacyInfo[nFriendSide]->pMinimapKeyObjectIcon;
		pEnemy = pMPConsts->diplomacyInfo[nEnemySide]->pMinimapKeyObjectIcon;
		pSelected = pMPConsts->diplomacyInfo[nFriendSide]->pMinimapKeyObjectIconSelected;
	}*/

	for ( std::unordered_map<int, CPtr<CMapObj> >::const_iterator it = keyObjects.begin(); it != keyObjects.end(); ++it )
	{
		CMapObj *pMO = it->second;
		
		const NDb::STexture *pTexture = 0;	
		if ( const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission() )
		{
			const int nKeyObjectPlayer = pMO->GetKeyObjectPlayer();
			if ( nKeyObjectPlayer >= 0 && nKeyObjectPlayer < pMapInfo->players.size() )
			{
				const NDb::SMapPlayerInfo &info = pMapInfo->players[nKeyObjectPlayer];
				const NDb::SPartyDependentInfo *pPartyInfo = Singleton<IScenarioTracker>()->GetPlayerParty( nKeyObjectPlayer );
				if ( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
				{
					int nPlayerSide = Singleton<IScenarioTracker>()->GetPlayerSide( nKeyObjectPlayer );
					pPartyInfo = pMPConsts->diplomacyInfo[nPlayerSide];
				}
				if ( pPartyInfo )
				{
					if ( it->first == nSelectedKeyObject )
						pTexture = pPartyInfo->pMinimapKeyObjectIconSelected;
					else
						pTexture = pPartyInfo->pMinimapKeyObjectIcon;
				}
			}
		}

		if ( pTexture )
		{
			markers.push_back( IMiniMap::SMarker() );
			IMiniMap::SMarker &marker = markers.back();
			marker.vPos = CVec2( pMO->GetCenter().x, pMO->GetCenter().y );
			marker.pTexture = pTexture;
			marker.vTexturePoint = CVec2( KEY_OBJECT_TEXTURE_POINT_X, KEY_OBJECT_TEXTURE_POINT_Y );
		}
	}

	const NDb::STexture *pMinimapNormal = InterfaceState()->GetTextureEntry( "TX_MINIMAP_OBJECTIVE_NORMAL" );
	const NDb::STexture *pMinimapSelected = InterfaceState()->GetTextureEntry( "TX_MINIMAP_OBJECTIVE_SELECTED" );

	const int known_count = Singleton<IScenarioTracker>()->GetKnownObjectiveCount();
	for ( int i = 0; i < known_count; ++i )
	{
		int nID = Singleton<IScenarioTracker>()->GetKnownObjectiveID( i );

		EMissionObjectiveState eState = Singleton<IScenarioTracker>()->GetObjectiveState( nID );
		if ( !bShowObjectives && eState != EMOS_RECEIVED )
			continue;

		std::vector<CVec3> places;
		bool bPlaces = Singleton<IScenarioTracker>()->GetObjectivePlaces( nID, &places);
		if ( bPlaces )
		{
			for ( int nPlace = 0; nPlace < places.size(); ++nPlace )
			{
				const CVec3 &vPos3 = places[nPlace];

				markers.push_back( IMiniMap::SMarker() );
				IMiniMap::SMarker &marker = markers.back();
				marker.vPos = CVec2( vPos3.x, vPos3.y );
				marker.pTexture = (nID != nSelectedID || !bShowObjectives) ? pMinimapNormal : pMinimapSelected;
				marker.vTexturePoint = CVec2( TEXTURE_POINT_X, TEXTURE_POINT_Y );
			}
		}
	}

	// north marker position
	const NDb::SMapInfo * pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( pMapInfo && pMapInfo->nNortType > 0 && pMapInfo->nNortType < 5 )
	{
		const NDb::STexture *pNorthTexture = InterfaceState()->GetTextureEntry( fmt::format( "TX_MINIMAP_NORTH_MARKER{}", pMapInfo->nNortType ) );
		if ( pNorthTexture )
			pMiniMap->SetNortDirectionTexture( pNorthTexture );
	}

	if ( bShowMPMarker )
	{
		IMiniMap::SMarker marker;
		marker.vPos = vMPMarkerPos;
		marker.pTexture = InterfaceState()->GetTextureEntry( "TX_MINIMAP_MP_MARKER" );
		marker.vTexturePoint = CVec2( 6.0f, 3.0f );
		markers.push_back( marker );
	}

	pMiniMap->SetMarkers( markers );
}

void CVisualNotifications::OnBtn( const std::string &szSender, bool bRightBtn )
{
	for ( std::list< CObj<SEvent> >::iterator it = events.begin(); it != events.end(); ++it )
	{
		SEvent *pEvent = *it;
		
		if ( pEvent->szName == szSender )
		{
			if ( bRightBtn )
			{
				if ( pParent )
					pParent->RemoveChild( pEvent->pItemWnd );
				events.erase( it );
				RearrangeEvents();
			}
			else
			{
				bool bErase;
				EventLeftClick( *it, &bErase );
				if ( bErase )
				{
					if ( pParent )
						pParent->RemoveChild( pEvent->pItemWnd );
					events.erase( it );
					RearrangeEvents();
				}
			}

			return;
		}
	}
}

void CVisualNotifications::InitEvents( IWindow *_pParent )
{
	nFreeEvent = 0;
	nEventItemHeight = 0;
	pParent = _pParent;
	
	IWindow *pEventPanel = GetChildChecked<IWindow>( pParent, "NotificationEventsPanel", true );
	pItemTemplateWnd = GetChildChecked<IWindow>( pEventPanel, "ItemTemplate", true );

	if ( pEventPanel )
		pEventPanel->ShowWindow( true );
	if ( pItemTemplateWnd )
		pItemTemplateWnd->ShowWindow( false );

	if ( pEventPanel )
	{
		CTRect<float> rect = pEventPanel->GetWindowRect();
		nEventBottom = rect.y2;
	}
	if ( pItemTemplateWnd )
		pItemTemplateWnd->GetPlacement( 0, 0, 0, &nEventItemHeight );

	nMaxEventCount = nEventBottom / (nEventItemHeight + EVENT_ITEMS_INTERVAL);

	if ( pEventPanel )
		pEventPanel->ShowWindow( false );
		
	if ( const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot() )
	{
		dbEvents.resize( NDb::NEVT_COUNT );
		for ( std::vector< CDBPtr< NDb::SNotificationEvent > >::const_iterator it = pGameRoot->notificationEvents.begin();
			it != pGameRoot->notificationEvents.end(); ++it )
		{
			if ( const NDb::SNotificationEvent *pDBEvent = *it )
			{
				dbEvents[pDBEvent->eType] = pDBEvent;
			}
		}
	}
}

CVisualNotifications::SEvent* CVisualNotifications::CreateEventItem( const SEventParams &params )
{
	const NDb::SNotificationEvent *pDBEvent = GetEvent( params.eEventType ); 
	if ( !pDBEvent )
		return 0;

	SEvent *pEvent = new SEvent();
	pEvent->pItemWnd = AddWindowCopy( pParent, pItemTemplateWnd );
	pEvent->pDBEvent = pDBEvent;
	pEvent->fVisibleTime = 0.0f;
	pEvent->szName = fmt::format( "{}{}", EVENT_NAME_PREFIX, nFreeEvent );
	pEvent->positions = params.positions;
	pEvent->objects = params.objects;
	pEvent->nID = params.nID;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBEvent->,Text) )
		pEvent->wszText = GET_TEXT_PRE(pDBEvent->,Text);
	nFreeEvent++;

	return pEvent;
}

void CVisualNotifications::CreateEventItemView( SEvent *pEvent )
{
	const NDb::SNotificationEvent *pDBEvent = pEvent->pDBEvent;

	ITextView *pDescView = GetChildChecked<ITextView>( pEvent->pItemWnd, "Desc", true );
	IButton *pIconBtn = GetChildChecked<IButton>( pEvent->pItemWnd, "IconBtn", true );
	IWindow *pIconWnd = GetChildChecked<IWindow>( pIconBtn, "Icon", true );

	if ( pEvent->pItemWnd )
		pEvent->pItemWnd->ShowWindow( true );
	if ( pIconWnd )
		pIconWnd->SetTexture( pDBEvent->pTexture );
	if ( pDescView )
	{
		std::wstring wszFormat = InterfaceState()->GetMissionConsoleMLTag();
		std::wstring wszText = pEvent->wszText;

		if ( pDBEvent->eType == NDb::NEVT_PLAYER_ELIMINATED )
		{
			const int nPlayer = pEvent->nID;
			IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
			IScenarioTracker::SMultiplayerInfo *pMPInfo = pScenarioTracker->GetMultiplayerInfo();

			for ( int i = 0; pMPInfo && i < pMPInfo->players.size(); ++i )
			{
				const IScenarioTracker::SMultiplayerInfo::SPlayer &player = pMPInfo->players[i];
				if ( player.nIndex != nPlayer || player.wszName.empty() )
					continue;

				// Player elimination stores the removed player slot in nID.
				const uint32_t dwColor = pScenarioTracker->GetPlayerColor( nPlayer ).dwColor;
				const std::wstring wszPlayerColor = NStr::ToUnicode( fmt::format( "<color=0x{:X}>", dwColor ) );
				
				//const int side = pScenarioTracker->GetPlayerSide( nPlayer );

				// TODO: these should be loaded from some text files, not hardcoded like this crap
				// std::wstring wszSideText = L"";
				// if (side == 0)
				// {
				// 	wszSideText += L"<color=0x00FF00FF>\n[Green]";
				// }
				// else if (side == 1)
				// {
				// 	wszSideText += L"<color=0x0000FFFF>\n[Red]";
				// }
				// else
				// {
				// 	wszSideText += L"<color=0xEEEEEEFF>\n[Neutral]";
				// }

				wszText += L": ";
				wszText += wszPlayerColor;
				wszText += player.wszName;
				// wszText += wszSideText;
				wszText += wszFormat;
				break;
			}
		}

		pDescView->SetText( pDescView->GetDBText() + wszFormat + wszText );
	}
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBEvent->,Tooltip) )
	{
		if ( pIconBtn )
			pIconBtn->SetTooltip( GET_TEXT_PRE(pDBEvent->,Tooltip) );
	}
	if ( pIconBtn )
		pIconBtn->SetName( pEvent->szName );

	if ( pEvent->pDBEvent->pSound )
		SoundScene()->AddSound( pEvent->pDBEvent->pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 2 );
}

void CVisualNotifications::UpdateEvents( float fDeltaTime )
{
	int nCount = 0;
	for ( std::list< CObj<SEvent> >::iterator it = events.begin(); it != events.end(); )
	{
		SEvent *pEvent = *it;

		pEvent->fVisibleTime += fDeltaTime;
		if ( pEvent->pDBEvent->fAutoRemoveTime > 0.0f && pEvent->fVisibleTime > pEvent->pDBEvent->fAutoRemoveTime )
		{
			if ( pParent )
				pParent->RemoveChild( pEvent->pItemWnd );
			it = events.erase( it );
			continue;
		}
		++nCount;
		++it;
	}
	
	if ( nCount > nMaxEventCount )
	{
		for ( std::list< CObj<SEvent> >::iterator it = events.begin(); it != events.end(); )
		{
			if ( nCount <= nMaxEventCount )
				break;

			SEvent *pEvent = *it;

			if ( pEvent->pDBEvent->fAutoRemoveTime > 0.0f ) // can auto remove
			{
				if ( pParent )
					pParent->RemoveChild( pEvent->pItemWnd );
				it = events.erase( it );
				--nCount;
				continue;
			}
			++it;
		}
	}

	RearrangeEvents();
}

void CVisualNotifications::RearrangeEvents()
{
	int nPos = nEventBottom - nEventItemHeight;
	for ( std::list< CObj<SEvent> >::iterator it = events.begin(); it != events.end(); ++it )
	{
		SEvent *pEvent = *it;
		
		int nPosItem = 0;
		if ( pEvent->pItemWnd )
		{
			pEvent->pItemWnd->GetPlacement( 0, &nPosItem, 0, 0 );
			if ( nPosItem != nPos )
			{
				pEvent->pItemWnd->SetPlacement( 0, nPos, 0, 0, EWPF_POS_Y );
			}
		}
		nPos -= nEventItemHeight + EVENT_ITEMS_INTERVAL;
	}
}

void CVisualNotifications::EventLeftClick( SEvent *pEvent, bool *pErase )
{
	*pErase = (pEvent->pDBEvent->fAutoRemoveTime > 0.0f);

	vLastCameraPos = Camera()->GetAnchor();

	if ( pEvent->pDBEvent->bShowByCamera )
	{
		CVec2 vNearPos;
		bool bResult = GetEventPos( &vNearPos, pEvent );
		if ( bResult )
			MoveCamera( vNearPos );
	}
	
	switch ( pEvent->pDBEvent->eType )
	{
		case NDb::NEVT_KEY_POINT_CAPTURED:
		case NDb::NEVT_KEY_POINT_LOST:
		case NDb::NEVT_ENEMY_UNIT_DETECTED:
		case NDb::NEVT_UNIT_ATTACKED:
		case NDb::NEVT_UNIT_BLOWUP_AT_MINE:
		case NDb::NEVT_UNIT_OUT_OF_AMMUNITION:
		case NDb::NEVT_ENGINEERING_MINE_DETECTED:
		case NDb::NEVT_ENGINEERING_COMPLETED:
		case NDb::NEVT_ENGINEERING_INTERRUPTED:
		{
			break;
		}
		
		case NDb::NEVT_OBJECTIVE_RECEIVED:
		case NDb::NEVT_OBJECTIVE_COMPLETED:
		case NDb::NEVT_OBJECTIVE_FAILED:
		{
			const auto command = std::to_string(  pEvent->nID );
			NMainLoop::Command( ML_COMMAND_MISSION_OBJECTIVES, command.c_str() );
			break;
		}

		case NDb::NEVT_KEY_POINT_ATTACKED:
		{
			break;
		}

		case NDb::NEVT_REINF_NEW_TYPE:
		{
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			const int nLocalPlayer = 0;

			std::vector<CInterfaceArmyBranchDlg::SBranch> branches;

			for ( int i = 0; i < NDb::_RT_NONE; ++i )
			{
				const NDb::EReinforcementType eReinforcementType = (NDb::EReinforcementType)( i );
				if ( pST->IsNewReinf( eReinforcementType ) )
				{
					const NDb::SReinforcement *pReinf = pST->GetReinforcement( nLocalPlayer, eReinforcementType );
					if ( !pReinf )
						continue;
					
					CInterfaceArmyBranchDlg::SBranch branch;
					if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
						branch.wszName = GET_TEXT_PRE(pReinf->,LocalizedName);
					if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedDesc) )
						branch.wszDesc = GET_TEXT_PRE(pReinf->,LocalizedDesc);
					branch.pIconTexture = pReinf->pIconTexture;

					if ( branch.wszDesc.empty() )
					{
						// look for common reinf type desc
						const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();
						if ( pUIC )
						{
							for ( int i = 0; i < pUIC->reinfButtons.size(); ++i )
							{
								if ( pUIC->reinfButtons[i].eType == eReinforcementType )
								{
									if ( branch.wszDesc.empty() )
									{
										if ( CHECK_TEXT_NOT_EMPTY_PRE(pUIC->reinfButtons[i].,Desc) )
											branch.wszDesc = GET_TEXT_PRE(pUIC->reinfButtons[i].,Desc);
									}
								}
							}
						}
					}
					
					branches.push_back( branch );
				}
			}

			CPtr<CICArmyBranchDlg> pCommand = new CICArmyBranchDlg( branches );
			NMainLoop::Command( pCommand );
			break;
		}

		case NDb::NEVT_REINF_LEVELUP:
		{
			const int nLocalPlayer = 0;
			const NDb::SReinforcement *pReinf = Singleton<IScenarioTracker>()->GetReinforcement( 
				nLocalPlayer, (NDb::EReinforcementType)( pEvent->nID ) );
			std::wstring wszText = InterfaceState()->GetTextEntry( "T_NOTIFICATIONS_LEVELUP_HEADER" );
			if ( pReinf )
			{
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
					wszText = wszText + GET_TEXT_PRE(pReinf->,LocalizedName);
			}
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
				CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszText ).c_str() );
			break;
		}

		case NDb::NEVT_AVIA_AVAILABLE:
		{
			NInput::PostEvent( "notification_open_reinf", 0, 0 );
			break;
		}

		case NDb::NEVT_AVIA_BAD_WEATHER_RETREAT:
		{
			break;
		}

		case NDb::NEVT_ENEMY_AVIA_DETECTED:
		{
			break;
		}
		
		case NDb::NEVT_REINF_CANT_CALL:
		{
			*pErase = false;
			break;
		}

		case NDb::NEVT_PLAYER_ELIMINATED:
		{
			*pErase = false;
			break;
		}
	}
}

const NDb::SNotificationEvent* CVisualNotifications::GetEvent( NDb::ENotificationEventType eEventType ) const
{
	NI_VERIFY( 0 <= eEventType && eEventType < dbEvents.size(), "Index out of range", return 0 );
	return dbEvents[eEventType];
}

void CVisualNotifications::AddEvent( const SEventParams &params )
{
	SEvent *pEvent = CreateEventItem( params );
	if ( !pEvent )
		return;

	events.push_back( pEvent );

	switch ( pEvent->pDBEvent->eType )
	{
		case NDb::NEVT_KEY_POINT_ATTACKED:
		{
			if ( !pEvent->positions.empty() )
				AddNotificationMinimap( -1, pEvent->positions.front(), NDb::NTF_KEY_OBJECT_ATTACKED );
			break;
		}

		case NDb::NEVT_REINF_LEVELUP:
		{
			break;
		}

		case NDb::NEVT_AVIA_BAD_WEATHER_RETREAT:
		{
			if ( !pEvent->positions.empty() )
				AddNotificationMinimap( -1, pEvent->positions.front(), NDb::NTF_AVIA_BAD_WEATHER_RETREAT );
			break;
		}

		case NDb::NEVT_ENEMY_AVIA_DETECTED:
		{
			if ( !pEvent->positions.empty() )
				AddNotificationMinimap( -1, pEvent->positions.front(), NDb::NTF_ENEMY_AVIA );
			break;
		}

		case NDb::NEVT_ENGINEERING_MINE_DETECTED:
		{
			if ( !pEvent->positions.empty() )
				AddNotificationMinimap( -1, pEvent->positions.front(), NDb::NTF_MINE_DETECTED );
			break;
		}
		
		case NDb::NEVT_OBJECTIVE_COMPLETED:
		case NDb::NEVT_OBJECTIVE_RECEIVED:
		case NDb::NEVT_OBJECTIVE_FAILED:
		{
			if ( const NDb::SMissionObjective *pObjective = GetObjective( pEvent->nID ) )
			{
				pEvent->positions = pObjective->mapPositions;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pObjective->,Header) )
					pEvent->wszText = pEvent->wszText + L": " + GET_TEXT_PRE(pObjective->,Header);
			}
			break;
		}
	}

	CreateEventItemView( pEvent );

	RearrangeEvents();
}

void CVisualNotifications::RemoveEvent( NDb::ENotificationEventType eEventType, int nID )
{
	bool bRemove = false;
	for ( std::list< CObj<SEvent> >::iterator it = events.begin(); it != events.end(); )
	{
		SEvent *pEvent = *it;
		
		if ( (eEventType == (NDb::ENotificationEventType)( -1 ) || pEvent->pDBEvent->eType == eEventType) && 
			(nID == -1 || pEvent->nID == nID) )
		{
			if ( pParent )
				pParent->RemoveChild( pEvent->pItemWnd );
			it = events.erase( it );
			bRemove = true;
		}
		else
			++it;
	}

	if ( bRemove )
		RearrangeEvents();
}

void CVisualNotifications::MoveCamera( const CVec2 &vPos )
{
	CVec2 vPosVis;
	AI2Vis( &vPosVis, vPos );
	Camera()->SetAnchor( CVec3( vPosVis.x, vPosVis.y, 0.0f ) );
}

const NDb::SMissionObjective* CVisualNotifications::GetObjective( int nID )
{
	if ( nID < 0 )
		return 0;

	const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( !pMapInfo )
		return 0;

	NI_VERIFY( nID < pMapInfo->objectives.size(), fmt::format( "Index out of range ({}:{})", nID, pMapInfo->objectives.size() ), return 0 );

	const NDb::SMissionObjective *pObjective = pMapInfo->objectives[nID];
	NI_ASSERT( pObjective, "No objective" );

	return pObjective;
}

void CVisualNotifications::OnEvent( const SEventParams &params )
{
	switch ( params.eEventType )
	{
		case NDb::NEVT_OBJECTIVE_MOVED:
		{
			ObjectiveMove( params.nID );
			return;
		}
	}
	
	AddEvent( params );
}

void CVisualNotifications::OnRemoveEvent( NDb::ENotificationEventType eEventType, int nID )
{
	RemoveEvent( eEventType, nID );
}

bool CVisualNotifications::GetEventPos( CVec2 *pPos, const SEvent *pEvent ) const
{
	CVec3 vCameraPos;
	Vis2AI( &vCameraPos, Camera()->GetAnchor() );
	CVec2 vCameraPos2( vCameraPos.x, vCameraPos.y );

	if ( !pEvent->objects.empty() )
	{
		bool bFound = false;
		CVec3 vNearPos;
		for ( int i = 0; i < pEvent->objects.size(); ++i )
		{
			CMapObj *pMO = pEvent->objects[i];
			if ( !IsValid( pMO ) )
				continue;
			if ( !pMO->IsAlive() )
				continue;
			if ( !pMO->IsVisible() )
				continue;
				
			const CVec3 &vPos3 = pMO->GetCenter();
			if ( !bFound || fabs2( CVec2( vPos3.x, vPos3.y ) - vCameraPos2 ) < fabs2( CVec2( vNearPos.x, vNearPos.y ) - vCameraPos2 ) )
			{
				vNearPos = vPos3;
				bFound = true;
			}
		}
		CVec3 vPos = CorrectPosByCameraAndHeight( vNearPos );
		pPos->x = vPos.x;
		pPos->y = vPos.y;
		if ( bFound )
			return true;
	}

	if ( !pEvent->positions.empty() )
	{
		CVec2 vNearPos = pEvent->positions[0];
		for ( int i = 1; i < pEvent->positions.size(); ++i )
		{
			const CVec2 &vPos = pEvent->positions[i];
			if ( fabs2( vPos - vCameraPos2 ) < fabs2( vNearPos - vCameraPos2 ) )
				vNearPos = vPos;
		}
		*pPos = vNearPos;
		return true;
	}
	
	return false;
}

bool CVisualNotifications::OnCameraBack()
{
	Camera()->SetAnchor( vLastCameraPos );

	return true;
}

bool CVisualNotifications::OnUpdateObjective( int nID )
{
	UpdateObjectivePointers( nID );
	return true;
}

void CVisualNotifications::ObjectiveMove( int nID )
{
	UpdateObjectivePointers( nID );
}

void CVisualNotifications::PlaceMarker( const CVec2 &vPos )
{
	bShowMPMarker = true;
	vMPMarkerPos = vPos;

	std::vector<CVec3> places;
	places.resize( 1, CVec3( vPos, Singleton<IAILogic>()->GetZ( vPos ) ) );
	RemoveObjectivePointers( MP_MARKER_POINTER_ID );
	AddPointerModels( MP_MARKER_POINTER_ID, places );

	AddNotification( MP_MARKER_POINTER_ID, vPos, NDb::NTF_MINIMAP_FLARE );

	timeMarkerExpire = GameTimer()->GetAbsTime() + MP_MARKER_EXPIRE;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x11135C01, CVisualNotifications )
REGISTER_SAVELOAD_CLASS_NM( GAMEX, 0x171BD2C0, SEvent, CVisualNotifications )

START_REGISTER( InterfaceNotifications )
REGISTER_VAR_EX( "autosave_objective_complete", NGlobal::VarBoolHandler, &s_bAutosaveObjectiveComplete, false, STORAGE_USER );
FINISH_REGISTER


