#include "stdafx.h"

#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/DBClientConsts.h"
#include "Stats_B2_M1/DBCameraConsts.h"
#include "InterfaceMissionBase.h"
#include "WorldClient.h"

#include "AILogic/B2AI.h"
#include "Transceiver.h"

#include "Sound/SoundScene.h"
#include "Sound/MusicSystem.h"
#include "GetConsts.h"
#include "ScenarioTracker.h"

#include "GameX_export.h"

#include <zconf.h>

class CVisualNotificationsEmpty : public IVisualNotifications
{
	OBJECT_NOCOPY_METHODS( CVisualNotificationsEmpty );
public:
	bool Notify( EVisualNotification eType, int nID, const CVec2 &vPos ) { return true; }
	bool Notify( EVisualNotification eType, class CMapObj *pMO ) { return true; }
	void Step( const NTimer::STime nDeltaTime, bool bAppActive ) {}
};

// CInterfaceMissionBase

CInterfaceMissionBase::CInterfaceMissionBase( const std::string &szInterfaceType, const std::string &szBindSection ) :
	CInterfaceScreenBase( szInterfaceType, szBindSection ), bFrozen( false )
{
}

CInterfaceMissionBase::~CInterfaceMissionBase()
{
	Unload();
}

void CInterfaceMissionBase::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( false );

	if ( pWorld ) 
		pWorld->OnGetFocus( true );
}

bool CInterfaceMissionBase::StepLocal( bool bAppActive )
{
	const bool bRet = CInterfaceScreenBase::StepLocal( bAppActive );
	if ( !bAppActive ) 
		return false;

	if ( IsValid( pTransceiver ) )
		pTransceiver->DoSegments();

	if ( pWorld )
		pWorld->Update();

	return bRet;
}

void CInterfaceMissionBase::Freeze( const bool bFreeze )
{
	bFrozen = bFreeze;
}

void CInterfaceMissionBase::NewMap( const NDb::SMapInfo *_pMap, ITransceiver *_pTransceiver, IScenarioTracker *_pScenarioTracker )
{
	pTransceiver = _pTransceiver;

	pMap = _pMap;

	//camera.Init( pMapInfo );

	Camera()->SetFOV( NGameX::GetClientConsts()->pCamera->fFOV );
	//Scene()->SetFOV( NGameX::GetClientConsts()->pCamera->fFOV );

	// Loading world
	pWorld = new CWorldClient( pTransceiver, new CVisualNotificationsEmpty(), Singleton<IAILogic>(), 
		_pScenarioTracker, 0 );

	if ( pMap->pMusic )
		Singleton<IMusicSystem>()->Init( pMap->pMusic, 0 );

	Singleton<IAILogic>()->Init( pTransceiver->GetCheckSumLogger(), pMap, NGameX::GetAIConsts(), _pScenarioTracker );
	pWorld->LoadMap( pMap );
	Singleton<IAILogic>()->InitAfterMapLoad( pMap );

	Scene()->ResetTimer( GetTickCount() );
	Singleton<IAILogic>()->ToggleWarFog( false );
	Singleton<IAILogic>()->PostMapLoad();
	pWorld->Update();
}

void CInterfaceMissionBase::StartNewMap( const NDb::SMapInfo *_pMap, ITransceiver *_pTransceiver, IScenarioTracker *pScenarioTracker )
{
	_pTransceiver->StartMission( _pMap, Singleton<IAILogic>() );
	NewMap( _pMap, _pTransceiver, pScenarioTracker );
}

void CInterfaceMissionBase::Unload()
{
	if ( pWorld )
	{
		Singleton<ICommonB2M1AI>()->Clear();
		Singleton<IAILogic>()->ClearAI();
		pWorld->Clear();
		pWorld = 0;
	}

	pTransceiver = 0;

	Singleton<ISoundScene>()->ClearSounds();
}

void CInterfaceMissionBase::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();
	
	if ( pWorld )
		pWorld->AfterLoad( pMap );
}

int CInterfaceMissionBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, (CInterfaceScreenBase*)this );

	saver.Add( 2, &pWorld );
//	saver.Add( 3, &nMapDBID );
	//saver.Add( 4, &camera );
	saver.Add( 5, &bFrozen );
	saver.Add( 6, &pTransceiver );
	saver.Add( 8, &pMap );

	return 0;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x11135C00, CVisualNotificationsEmpty );

