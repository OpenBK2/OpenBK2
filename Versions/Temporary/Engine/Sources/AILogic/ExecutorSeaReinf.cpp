#include "stdafx.h"

#include "ExecutorSeaReinf.h"
#include "GroupLogic.h"
#include "Common_RTS_AI/AIMap.h"
#include "DebugTools/DebugInfoManager.h"
#include "UnitStates.h"
#include "B2AI.h"

extern CGroupLogic theGroupLogic;

static void DisplayDebugCross( const CVec2 &vPos, const float fSize = 5.0f, const int nWidth = 1, const NDebugInfo::EColor eColor = NDebugInfo::WHITE )
{
	CSegment segm;
	segm.p1 = vPos + CVec2( fSize, 0 );
	segm.p2 = vPos + CVec2( -fSize, 0 );
	segm.dir = segm.p2 - segm.p1;
	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );

	segm.p1 = vPos + CVec2( 0, fSize );
	segm.p2 = vPos + CVec2( 0, -fSize );
	segm.dir = segm.p2 - segm.p1;
	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );
}

CExecutorTransportReinforcement::CExecutorTransportReinforcement( list<CAIUnit*> *pTransports, const NDb::SReinforcement *_pReinf, const CVec2 &_vStart, const CVec2 &_vUnload, const CVec2 &_vTarget ) :
CExecutor(TID_TRANSPORT_REINFORCEMENT, SConsts::BEH_UPDATE_DURATION/SConsts::AI_SEGMENT_DURATION), 
vStart( _vStart ), vTarget( _vTarget )
{
	// Assign transports
	transports.resize( pTransports->size() );
	int i = 0;
	for ( list<CAIUnit*>::iterator it = pTransports->begin(); it != pTransports->end(); ++it, ++i )
	{
		transports[i].pUnit = *it;
		transports[i].eState = ETS_FULL;
	}

	// Find spot to send to: first impassable point along the line
	CVec2 vCurPoint = vStart;
	CVec2 vStep = _vUnload - vCurPoint;
	Normalize( &vStep );
	vStep = vStep * AI_TILE_SIZE;
	while ( 1 )
	{
		SVector vTile = GetAIMap()->GetTile( vCurPoint + vStep );
		if ( !GetAIMap()->IsPointInside( vCurPoint ) ||
			!GetAIMap()->IsTileInside( vTile ) ||
			( GetTerrain()->GetTileLockInfo( vTile ) & EAC_WATER ) != 0 )
			break;

		vCurPoint += vStep;
	}
	vUnload = vCurPoint;

	// Give commands
	vector<int> ids;
	for ( list<CAIUnit*>::iterator it = pTransports->begin(); it != pTransports->end(); ++it )
	{
		ids.push_back( (*it)->GetUniqueId() );
	}
	const int nGroup = theGroupLogic.GenerateGroupNumber();
	theGroupLogic.RegisterGroup( ids, nGroup );

	SAIUnitCmd cmd;
	cmd.nCmdType = ACTION_COMMAND_UNLOAD;
	cmd.vPos = vTarget;
	theGroupLogic.GroupCommand( cmd, nGroup, false );
	theGroupLogic.UnregisterGroup( nGroup );
	Singleton<IAILogic>()->SetNeedNewGroupNumber();
}

int CExecutorTransportReinforcement::Segment()
{
	// Check if transports reached the shore
	bool bFinish = true;
	for ( int i = 0; i < transports.size(); ++i )
	{
		if ( transports[i].eState == ETS_LEFT )
			continue;

		if ( !transports[i].pUnit->IsAlive() )			// Check if the transport is dead
		{
			transports[i].eState = ETS_LEFT;
			continue;
		}

		if ( transports[i].pUnit->IsIdle() )
		{
			switch ( transports[i].eState )
			{
			case ETS_FULL:
				{	
					transports[i].eState = ETS_UNLOADING;
					bFinish = false;
				}
				break;
			case ETS_UNLOADING:
				{
					if ( transports[i].pUnit->GetState()->GetName() != EUSN_LAND )
					{
						CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Transport going back" );
						transports[i].eState = ETS_EMPTY;
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vStart ), transports[i].pUnit, false );
					}
					bFinish = false;
				}
				break;
			case ETS_EMPTY:
				{
					// Remove transport from map
					transports[i].pUnit->Stop();
					transports[i].pUnit->Disappear();
					transports[i].eState = ETS_LEFT;
				}
				break;
			}
		}
		else
			bFinish = false;
	}

	if ( bFinish )
		return -1;
	else
		return GetNextTime();
}

REGISTER_SAVELOAD_CLASS( 0x19174C40, CExecutorTransportReinforcement )

