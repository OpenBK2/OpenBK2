#include "stdafx.h"

#include "B2AI.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/Terrain.h"
#include "DebugTools/DebugInfoManager.h"
#include "ExecutorSeaReinf.h"
#include "GroupLogic.h"
#include "UnitStates.h"

#include "AILogic_export.h"

#include <cmath>
#include <cstdint>

extern CGroupLogic theGroupLogic;

static void DisplayDebugCross( const CVec2 &vPos,
                               const float fSize = 5.0f,
                               const int nWidth = 1,
                               const NDebugInfo::EColor eColor = NDebugInfo::WHITE )
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

CVec2 CExecutorTransportReinforcement::FindUnloadPoint( CAIMap *pAIMap, const CVec2 &vStart, const CVec2 &vDestination )
{
	CVec2 vCurPoint = vStart;
	CVec2 vStep = vDestination - vStart;
	const float fDistance = fabs( vStep );
	if ( !pAIMap->IsPointInside( vStart ) || !std::isfinite( fDistance ) || !( fDistance > 0.0f ) )
	{
		return vCurPoint;
	}
	Normalize( &vStep );
	vStep *= AI_TILE_SIZE;
	// A straight ray leaves a rectangular map within its width plus height. Bound retries by that distance.
	const int64_t nMaxSteps =
			( int64_t( pAIMap->GetSizeX() ) + pAIMap->GetSizeY() ) * pAIMap->GetTileSize() / AI_TILE_SIZE + 2;
	for ( int64_t nStep = 0; nStep < nMaxSteps; ++nStep )
	{
		const CVec2 vNext = vCurPoint + vStep;
		if ( vNext == vCurPoint || !pAIMap->IsPointInside( vNext ) )
		{
			// Equal endpoints or rounded-away movement must not keep scanning the same water tile.
			break;
		}
		const SVector vTile = pAIMap->GetTile( vNext );
		if ( !pAIMap->IsTileInside( vTile ) || ( pAIMap->GetTerrain()->GetTileLockInfo( vTile ) & EAC_WATER ) != 0 )
		{
			break;
		}
		vCurPoint = vNext;
	}
	return vCurPoint;
}

CExecutorTransportReinforcement::CExecutorTransportReinforcement( std::list<CAIUnit *> *pTransports,
                                                                  const NDb::SReinforcement *_pReinf,
                                                                  const CVec2 &_vStart,
                                                                  const CVec2 &_vUnload,
                                                                  const CVec2 &_vTarget )
	: CExecutor( TID_TRANSPORT_REINFORCEMENT, SConsts::BEH_UPDATE_DURATION / SConsts::AI_SEGMENT_DURATION )
	, vStart( _vStart )
	, vTarget( _vTarget )
{
	// Assign transports
	transports.resize( pTransports->size() );
	int i = 0;
	for ( std::list<CAIUnit *>::iterator it = pTransports->begin(); it != pTransports->end(); ++it, ++i )
	{
		transports[i].pUnit = *it;
		transports[i].eState = ETS_FULL;
	}

	// Find spot to send to: first impassable point along the line
	vUnload = FindUnloadPoint( GetAIMap(), vStart, _vUnload );

	// Give commands
	std::vector<int> ids;
	for ( std::list<CAIUnit *>::iterator it = pTransports->begin(); it != pTransports->end(); ++it )
	{
		ids.push_back( ( *it )->GetUniqueId() );
	}
	const int nGroup = theGroupLogic.GenerateSimulationGroupNumber();
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
		{
			continue;
		}

		if ( !transports[i].pUnit->IsAlive() ) // Check if the transport is dead
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
				default:
					break;
			}
		}
		else
		{
			bFinish = false;
		}
	}

	if ( bFinish )
	{
		return -1;
	}
	else
	{
		return GetNextTime();
	}
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x19174C40, CExecutorTransportReinforcement )
