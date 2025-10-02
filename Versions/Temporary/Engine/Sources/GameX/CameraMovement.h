#pragma once

#include "../Stats_B2_M1/DBMapInfo.h"

struct ICamera;

class CCameraMovement
{
	CDBPtr<NDb::SMapInfo> pMapInfo;
	NDb::SCameraPlacement start;
	int nTimeToMove;
	bool bMovementFinished;
	int nFinishPoint;
	CPtr<ICamera> pCamera;
	NTimer::STime timeMoveStart;
public:
	CCameraMovement() : nTimeToMove( 0 ), bMovementFinished( true ), timeMoveStart( 0 ), nFinishPoint( -1 ) {  }

	int operator&( IBinSaver &saver );
	void Init( const NDb::SMapInfo *_pMapInfo );
	
	// return false if movement is finished
	bool Segment( const NTimer::STime curTime );
	// move to
	void SetFinishPoint( const int nPoint, const int nTimeToMove, const NTimer::STime curTime );
	void FinishMove() { bMovementFinished = true; }
};


