#include "StdAfx.h"
#include ".\cameramovement.h"
#include "..\SceneB2\Camera.h"


int CCameraMovement::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pMapInfo );
	saver.Add( 2, &start );
	saver.Add( 3, &nTimeToMove );
	saver.Add( 4, &bMovementFinished );
	saver.Add( 5, &nFinishPoint );
	saver.Add( 6, &timeMoveStart );
	saver.Add( 7, &pCamera );
	return 0;
}

void CCameraMovement::Init( const NDb::SMapInfo *_pMapInfo )
{
	pCamera = Camera();
	pMapInfo = _pMapInfo;
}

// return false if movement is finished
bool CCameraMovement::Segment( const NTimer::STime curTime )
{
	if ( bMovementFinished ) 
		return false;

	const NTimer::STime timeDiff( curTime - timeMoveStart );
	const NDb::SCameraPlacement &pos( pMapInfo->cameraPositions[nFinishPoint] );
	if ( timeDiff >= nTimeToMove || 0 == nTimeToMove )
	{
		pCamera->SetAnchor( pos.vAnchor );
		pCamera->SetPlacement( pos.fDist, pos.fPitch, pos.fYaw );
		bMovementFinished = true;
		return false;
	}
	else
	{
		const float fMoved( float(curTime - timeMoveStart ) / float( nTimeToMove ) );
		pCamera->SetAnchor( start.vAnchor + (pos.vAnchor - start.vAnchor) * fMoved );
		pCamera->SetPlacement( start.fDist + (pos.fDist - start.fDist) * fMoved,
													 start.fPitch + (pos.fPitch - start.fPitch ) * fMoved,
													 start.fYaw + (pos.fYaw - start.fYaw) * fMoved );
		return true;
	}
}

// move to
void CCameraMovement::SetFinishPoint( const int nPoint, const int _nTimeToMove, const NTimer::STime _curTime )
{
	NI_ASSERT( nPoint < pMapInfo->cameraPositions.size(), StrFmt( "attempt to move camera to position number %i, in map there is only %i positions", nPoint, pMapInfo->cameraPositions.size() ) ) ;
	if ( nPoint < pMapInfo->cameraPositions.size() )
	{
		start.vAnchor = pCamera->GetAnchor();
		pCamera->GetPlacement( &start.fDist, &start.fPitch, &start.fYaw );
		nFinishPoint = nPoint;
		nTimeToMove = _nTimeToMove;
		timeMoveStart = _curTime;
		bMovementFinished = false;
	}
}

