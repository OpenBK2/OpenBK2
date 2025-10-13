#include "stdafx.h"

#include "MOEntrenchment.h"
#include "SceneB2/TerraGen.h"
#include "DebugTools/DebugInfoManager.h"

#include <cstdint>

bool CMOEntrenchmentPart::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	CDBPtr<NDb::SEntrenchmentRPGStats> pStats = checked_cast<const NDb::SEntrenchmentRPGStats*>( GetStats() );
	nFrameIndex = ( pUpdate != 0 ) ? pUpdate->info.nFrameIndex : 0;
	const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &segment = pStats->segments[nFrameIndex];
	const NDb::SModel *pModel = GetModel( segment.pVisObj, eSeason );
	const CVec3 vPos = ( pUpdate != 0 ) ? CVec3( pUpdate->info.center.x, pUpdate->info.center.y, pUpdate->info.z + 5.0f ) : VNULL3;

	if ( pUpdate != 0 )
		DeriveTransform( vPos, pUpdate->info.dir );					//Fill cache

	SetPlacement( vPos, cached.qRot );

	if ( pUpdate != 0 )
	{
	}

	Scene()->AddObject( nUniqueID, pModel, cached.mPlace, OBJ_ANIM_MODE_DEFAULT, 0 );
	SetModel( pModel );

	/*Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW, 
		StrFmt( "Create trench" ) );*/


	return true;
}

void CMOEntrenchmentPart::AIUpdatePlacement( const SAINotifyPlacement &placement, IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	if ( placement.bNewFormat )
	{
		DeriveTransform( placement.vPlacement, placement.dir );

		SetPlacement( placement.vPlacement, placement.rotation );
	}
	else
	{
		CVec3 vPos = CVec3( placement.center.x, placement.center.y, placement.z );

		DeriveTransform( vPos, placement.dir );

		SetPlacement( vPos, cached.qRot );
	}
	pScene->MoveObject( GetID(), cached.mPlace );
}

//void DisplayDebugCross( const CVec2 &vPos, const float fSize = 5.0f, const int nWidth = 1, const NDebugInfo::EColor eColor = NDebugInfo::WHITE )
//{
//	CSegment segm;
//	segm.p1 = vPos + CVec2( fSize, 0 );
//	segm.p2 = vPos + CVec2( -fSize, 0 );
//	segm.dir = segm.p2 - segm.p1;
//	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );
//
//	segm.p1 = vPos + CVec2( 0, fSize );
//	segm.p2 = vPos + CVec2( 0, -fSize );
//	segm.dir = segm.p2 - segm.p1;
//	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );
//}

// calculate transform from position, rotation, normal
void CMOEntrenchmentPart::DeriveTransform( const CVec3 &_vPos, const uint16_t _wDir )
{
	if ( cached.vPos == _vPos && cached.wDir == _wDir )
		return;				//Same matrix, no need to recompute

	SHMatrix	mTransform;
	SHMatrix	mShear;
	CVec3			vPos;
	CVec3			vNormal( VNULL3 );
	float			fShearFactor = 0.0f;
	CDBPtr<NDb::SEntrenchmentRPGStats> pStats = checked_cast<const NDb::SEntrenchmentRPGStats*>( GetStats() );
	const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &segment = pStats->segments[nFrameIndex];

	//Store new values
	cached.wDir			= _wDir;
	cached.vPos			= _vPos;

	//Calculate matrix and rotation quat
	cached.qRot = CQuat( float( _wDir ) / 65536 * FP_2PI, V3_AXIS_Z );

	// Calculate hole params
	cached.fHoleWidth = segment.vAABBHalfSize.y;
	CVec3 vTemp( segment.vAABBCenter.x + segment.vAABBHalfSize.x, segment.vAABBCenter.y, 0 );
	CVec3 v1, v2;
	cached.qRot.Rotate( &v1, vTemp );
	vTemp.Set( segment.vAABBCenter.x - segment.vAABBHalfSize.x, segment.vAABBCenter.y, 0 );
	cached.qRot.Rotate( &v2, vTemp );
	v1 += cached.vPos;
	v2 += cached.vPos;
	cached.vHoleStart = CVec2( v1.x, v1.y );
	cached.vHoleEnd = CVec2( v2.x, v2.y );

	// Calculate normal vector
	float fZ1 = Scene()->GetZ( cached.vHoleStart.x, cached.vHoleStart.y );
	float fZ2 = Scene()->GetZ( cached.vHoleEnd.x, cached.vHoleEnd.y );
	vTemp.Set( segment.vAABBCenter.x, segment.vAABBCenter.y + segment.vAABBHalfSize.x, 0 );
	cached.qRot.Rotate( &v1, vTemp );				// v1 is a side sample
	v1 += cached.vPos;
	v1.z = Scene()->GetZ( v1.x, v1.y );
	vTemp.Set( segment.vAABBCenter.x, segment.vAABBCenter.y - segment.vAABBHalfSize.x, 0 );
	cached.qRot.Rotate( &v2, vTemp );				// v2 is a side sample
	v2 += cached.vPos;
	v2.z = Scene()->GetZ( v2.x, v2.y );

	vNormal.Set( ( fZ2 - fZ1 ) / 2.0f, ( v2.z - v1.z ) / 2.0f, segment.vAABBHalfSize.x );

	cached.vPos.z = ( fZ1 + fZ2 ) / 2.0f + segment.vAABBHalfSize.z / 3.0f;
	AI2Vis( &vPos, cached.vPos );
	MakeMatrix( &mTransform, vPos, cached.qRot, CVec3( 1, 1, 1 ) );

	// Calculate shear parameters
	CVec2 vNormalProj( vNormal.x, vNormal.y );

	Normalize( &vNormalProj );
	Normalize( &vNormal );
	fShearFactor = - sqrt( vNormal.x * vNormal.x + vNormal.y * vNormal.y ) / vNormal.z;

	// Set shear part
	mShear.Set( 1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							fShearFactor * vNormalProj.x, fShearFactor * vNormalProj.y, 1.0f, 0.0f,
							0.0f,	0.0f, 0.0f, 1.0f );

	// Combine
	cached.mPlace = mTransform * mShear;
}

void CMOEntrenchmentPart::GetHoleParams( CVec2 *pStart, CVec2 *pEnd, float *pWidth )
{ 
	*pStart = cached.vHoleStart;
	*pEnd = cached.vHoleEnd;
	*pWidth = cached.fHoleWidth;
}

bool CMOEntrenchmentPart::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	return CMapObj::Create( nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor );
}

void CMOEntrenchmentPart::GetStatus( SObjectStatus *pStatus ) const
{
	CMOSelectable::GetStatus( pStatus );
}

void CMOEntrenchmentPart::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_BOARD );
	}
}

void CMOEntrenchmentPart::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
}

int CMOEntrenchmentPart::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CMapObj*>(this) );
	saver.Add( 2, &pParent );
	saver.Add( 3, &nFrameIndex );
	return 0;
}

// CMOEntrenchment

CMOEntrenchment::~CMOEntrenchment(void)
{
	while ( !parts.empty() ) 
	{
		CPtr<CMOEntrenchmentPart> pPart = parts.back();
		parts.pop_back();
		pPart->SetParent( 0 );
	}
}

bool CMOEntrenchment::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	//const SAITrenchUpdate *pUpdate = checked_cast<const SAITrenchUpdate *>( _pUpdate );
	CMapObj::SetID( nUniqueID );

	return true;
}

void CMOEntrenchment::AddPart( CMOEntrenchmentPart *pPart, const bool bLast, const bool bDigBySegment /*= false*/ )
{ 
	std::vector<CVec2> pts;
	CVec2	vStart, vEnd;
	float fWidth = 0.0f;

	pPart->SetParent( this ); 
	parts.push_back( pPart ); 
	/*Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
		StrFmt( "Add trench (DBID %d)", pPart->GetID() ) );*/

	ITerraManager *pTerraManager = Scene()->GetTerraManager();
	if ( bLast )
	{																							//All parts created, dig the trench in the terrain
		if ( bDigBySegment )
		{												// Piecewise digging, dig only one last segment
			pPart->GetHoleParams( &vStart, &vEnd, &fWidth );
			pts.resize( 2 );
			pts[0] = vStart;
			pts[1] = vEnd;
			pTerraManager->AddEntrenchment( pts, fWidth );
		}
		else
		{
			int nSize = parts.size();

			pts.resize( nSize + 1 );
			if ( nSize > 0 )
			{
				(*parts.begin())->GetHoleParams( &vStart, &vEnd, &fWidth );

				if ( nSize > 1 )			// select start point smartly
				{
					CPartsList::iterator it = parts.begin();
					++it;
					CMOEntrenchmentPart *pSecondPart = *it;

					CVec2 vCenter2( pSecondPart->GetCenter().x, pSecondPart->GetCenter().y );
					float fStartDiff = fabs2( vCenter2 - vStart );
					float fEndDiff = fabs2( vCenter2 - vEnd );

					if ( fStartDiff > fEndDiff )
						pts[0].Set( vStart.x, vStart.y );
					else
						pts[0].Set( vEnd.x, vEnd.y );
				}
				else
				{
					pts[0].Set( vStart.x, vStart.y );
				}

				/*Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
					StrFmt( "Digging trench (%d segments), width %.2f", nSize, fWidth ) );
				Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
					StrFmt( "Point[%d] = (%.2f, %.2f)", 0, pts[0].x, pts[0].y ) );
				DisplayDebugCross( pts[0], 20, 1, NDebugInfo::BLUE );*/

				int i = 1;
				CVec2 vPrevPoint = pts[0];
				for ( CPartsList::iterator it = parts.begin(); it != parts.end(); ++it, ++i )
				{
					(*it)->GetHoleParams( &vStart, &vEnd, &fWidth );

					float fStartDiff = fabs2( vPrevPoint - vStart );
					float fEndDiff = fabs2( vPrevPoint - vEnd );
					CVec2 vNextPoint = ( fStartDiff > fEndDiff ) ? vStart : vEnd;
					pts[ i ].Set( vNextPoint.x, vNextPoint.y );

					/*Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
						StrFmt( "Point[%d] = (%.2f, %.2f)", i, vNextPoint.x, vNextPoint.y ) );
					DisplayDebugCross( vNextPoint, 20 );*/

					vPrevPoint = vNextPoint;
				}
				parts.back()->GetHoleParams( &vStart, &vEnd, &fWidth );
				pts[nSize].Set( vStart.x, vStart.y );

				pTerraManager->AddEntrenchment( pts, fWidth );
			}
		}
	}
}

void CMOEntrenchment::GetStatus( SObjectStatus *pStatus ) const
{
	CMOSelectable::GetStatus( pStatus );
}

void CMOEntrenchment::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
		pActions->SetAction( NDb::USER_ACTION_BOARD );
	}
}

void CMOEntrenchment::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
}

NDb::EUserAction CMOEntrenchment::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const
{
	if ( bAltMode && pActionsWith->HasAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN ) )
		return NDb::USER_ACTION_MOVE_LIKE_TERRAIN;
		
	return CMapObj::GetBestAutoAction( actionsBy, pActionsWith, bAltMode );
}

bool CMOEntrenchment::IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const
{
	return eUserAction == NDb::USER_ACTION_MOVE || eUserAction == NDb::USER_ACTION_MOVE_LIKE_TERRAIN;
}

int CMOEntrenchment::operator&( IBinSaver &saver )
{
	saver.Add( 1, checked_cast<CMapObj*>(this) );
	saver.Add( 2, &parts );
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x100A7482, CMOEntrenchmentPart );
REGISTER_SAVELOAD_CLASS( 0x100A7483, CMOEntrenchment );

