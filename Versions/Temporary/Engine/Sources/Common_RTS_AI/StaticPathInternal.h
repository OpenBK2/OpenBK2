#pragma once

#include "StaticPath.h"
#include "DebugTools/DebugInfoManager.h"

class CAIMap;

// большой статический путь, вдоль которого идут юниты
class CCommonStaticPath : public IStaticPath
{
	OBJECT_BASIC_METHODS( CCommonStaticPath );

	ZDATA
		vector<SVector> path;
		int nLen;
		// в тайловых координатах
		SVector startTile, finishTile;
		CVec2 finishPoint;
		CPtr<CAIMap> pAIMap;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&path); f.Add(3,&nLen); f.Add(4,&startTile); f.Add(5,&finishTile); f.Add(6,&finishPoint); f.Add(7,&pAIMap); return 0; }
public:
	CCommonStaticPath()	: nLen( -1 ), startTile( -1, -1 ), finishTile( -1, -1 ), finishPoint( VNULL2 )	{ }
	CCommonStaticPath( class CCommonPathFinder *pStaticPathFinder, CAIMap *pAIMap );
	
	// передвинуть начальный тайл на path[nStart]
	void MoveStartTileTo( const int nStart );
	// сделать конечным тайл на path[nFinish - 1]
	void MoveFinishTileTo( const int nFinish );
	// передвинуть конечную точку на vMove
	void MoveFinishPointBy( const CVec2 &vMove );
	
	const int GetLength() const	{ return nLen; }

	virtual const SVector GetTile( const int n ) const	{	NI_ASSERT( n >= 0 && n < nLen, "Wrong point number" ); return path[n]; }
	
	const SVector GetStartTile() const	{ return startTile; }
	const SVector GetFinishTile() const	{ return finishTile; }
	const CVec2& GetFinishPoint() const { return finishPoint; }

	bool MergePath( IStaticPath *pAppendant, const int nStartTile );

	int MarkStaticPath( const int nID, const NDebugInfo::EColor color ) const;
};


