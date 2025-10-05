#include "stdafx.h"

#include "DebugInfoManagerInternal.h"

//REGISTER_SAVELOAD_CLASS( IDebugInfoManager::tidTypeID, CDebugInfoManager );

IDebugInfoManager *CreateDebugInfoManager()
{
	return new CDebugInfoManager();
}

const int CDebugInfoManager::GetID( const int nID )
{
	if ( nID == NDebugInfo::OBJECT_ID_FORGET || nID == NDebugInfo::OBJECT_ID_GENERATE )
	{
		++nCurrentID;
		return nCurrentID;
	}
	else
		return nID;
}

const int CDebugInfoManager::PushUpdate( NDebugInfo::SDebugInfoUpdate *pObject )
{
	updates.push_back( pObject );
	return pObject->nID;
}

int CDebugInfoManager::CreateMarker( const int nID, const std::vector<SVector> &tiles, const NDebugInfo::EColor eColor )
{
	return PushUpdate( new NDebugInfo::SDebugInfoMarker( GetID( nID ), tiles, eColor ) );
}

int CDebugInfoManager::CreateCircle( const int nID, const CCircle &circle, const NDebugInfo::EColor eColor )
{
	return PushUpdate( new NDebugInfo::SDebugInfoCircle( GetID( nID ), circle, eColor ) );
}

int CDebugInfoManager::CreateSegment( const int nID, const CSegment &segment, const int nThickness, const NDebugInfo::EColor eColor )
{
	return PushUpdate( new NDebugInfo::SDebugInfoSegment( GetID( nID ), segment, nThickness, eColor ) );
}

void CDebugInfoManager::DeleteObject( const int nID )
{
	if ( nID > 0 ) 
		PushUpdate( new NDebugInfo::SDebugInfoDeleteObject( nID ) );
}

const CVec4 CDebugInfoManager::Color2CVec4( const NDebugInfo::EColor eColor ) const
{
	CVec4 vColor = CVec4( 255, 255, 255, 255 );
	switch( eColor )
	{
	case NDebugInfo::BLACK:
		vColor = CVec4( 0, 0, 0, 255 );
		break;
	case NDebugInfo::GREEN:
		vColor = CVec4( 0, 255, 0, 255 );
		break;
	case NDebugInfo::BLUE:
		vColor = CVec4( 0, 0, 255 ,255 );
		break;
	case NDebugInfo::RED:
		vColor = CVec4( 255, 0, 0, 255 );
		break;
	}
	return vColor;
}

int CDebugInfoManager::DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const NDebugInfo::EColor eColor )
{
	return DrawLine( nID, arrowStart, arrowEnd, Color2CVec4( eColor ), true );
}

int CDebugInfoManager::DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const CVec4 &vColor, const bool bDepthCheck )
{
	return PushUpdate( new NDebugInfo::SDebugInfoLine( GetID( nID ), arrowStart, arrowEnd, vColor, bDepthCheck ) );
}

int CDebugInfoManager::DrawLine( const int nID, const CVec2 &vStart, const CVec2 &vEnd, const bool bArrowEnd, const float fZ, const CVec4 &vColor )
{
	NDebugInfo::SArrowHead arrowStart( CVec3( vStart, fZ ) );
	NDebugInfo::SArrowHead arrowEnd( CVec3( vEnd, fZ ), bArrowEnd ? 32.0f : 0.0f, bArrowEnd ? 32.0f : 0.0f );
	return DrawLine( nID, arrowStart, arrowEnd, vColor, true );
}

int CDebugInfoManager::DrawRect( const int nID, const SRect &rect, const float fZ, const CVec4 &vColor, const bool bDepthCheck )
{
	return PushUpdate( new NDebugInfo::SDebugInfoRect( GetID( nID ), rect, fZ, vColor, bDepthCheck ) );
}

int CDebugInfoManager::DrawRect( const int nID, const SRect &rect, const float fZ, const NDebugInfo::EColor eColor )
{
	return PushUpdate( new NDebugInfo::SDebugInfoRect( GetID( nID ), rect, fZ, Color2CVec4( eColor ), true ) );
}

void CDebugInfoManager::RemoveLine( const int nID )
{
	if ( nID > 0 ) 
		PushUpdate( new NDebugInfo::SDebugInfoDeleteLine( nID ) );
}

void CDebugInfoManager::ShowAxes( const bool bShow )
{
	const bool bAxesVisible = ( nRedAxisID != NDebugInfo::OBJECT_ID_GENERATE &&
		nGreenAxisID != NDebugInfo::OBJECT_ID_GENERATE && nBlueAxisID != NDebugInfo::OBJECT_ID_GENERATE );
	if ( bShow != bAxesVisible )
	{
		if ( bShow )
		{
			nRedAxisID = DrawLine( nRedAxisID, NDebugInfo::SArrowHead( VNULL3 ), NDebugInfo::SArrowHead( CVec3( 256, 0, 0 ), 64, 32 ), NDebugInfo::RED );
			nGreenAxisID = DrawLine( nGreenAxisID, NDebugInfo::SArrowHead( VNULL3 ), NDebugInfo::SArrowHead( CVec3( 0, 256, 0 ), 64, 32 ), NDebugInfo::GREEN );
			nBlueAxisID = DrawLine( nBlueAxisID, NDebugInfo::SArrowHead( VNULL3 ), NDebugInfo::SArrowHead( CVec3( 0, 0, 256 ), 64, 32 ), NDebugInfo::BLUE );
		}
		else
		{
			RemoveLine( nRedAxisID );
			RemoveLine( nGreenAxisID );
			RemoveLine( nBlueAxisID );
			nRedAxisID = NDebugInfo::OBJECT_ID_GENERATE;
			nGreenAxisID = NDebugInfo::OBJECT_ID_GENERATE;
			nBlueAxisID = NDebugInfo::OBJECT_ID_GENERATE;
		}
	}
}

NDebugInfo::EColor CDebugInfoManager::GetCycleColor()
{
	switch( currentColor ) 
	{
	case NDebugInfo::RED:
		currentColor = NDebugInfo::GREEN;
		break;
	case NDebugInfo::GREEN:
		currentColor = NDebugInfo::BLUE;
		break;
	case NDebugInfo::BLUE:
		currentColor = NDebugInfo::RED; break;
	default:
		currentColor = NDebugInfo::RED;
	}

	return currentColor;
}

void CDebugInfoManager::Reset()
{
	nCurrentID = 0;
	currentColor = NDebugInfo::BLUE;
	nRedAxisID = NDebugInfo::OBJECT_ID_GENERATE;
	nGreenAxisID = NDebugInfo::OBJECT_ID_GENERATE;
	nBlueAxisID = NDebugInfo::OBJECT_ID_GENERATE;

	updates.clear();
}

const NDebugInfo::SDebugInfoUpdate *CDebugInfoManager::GetUpdate() const
{
	if ( updates.empty() )
		return 0;
	else
		return updates.front();
}

const bool CDebugInfoManager::PopUpdate()
{
	if ( updates.empty() )
		return false;
	else
	{
		updates.pop_front();
		return true;
	}
}


