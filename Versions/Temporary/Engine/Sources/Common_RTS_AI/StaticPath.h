#pragma once

#include "../DebugTools/DebugInfoManager.h"

// большой статический потайловый  путь, вдоль которого идут юниты
interface IStaticPath : public CAIObjectBase
{
	virtual const SVector GetStartTile() const	= 0;
	virtual const SVector GetFinishTile() const = 0;
	virtual const CVec2& GetFinishPoint() const = 0;

	virtual const int GetLength() const	= 0;
	virtual const SVector GetTile( const int n ) const	= 0;
	virtual void MoveFinishPointBy( const CVec2 &vMove ) = 0;

	virtual void MoveStartTileTo( const int nStart ) = 0;
	virtual void MoveFinishTileTo( const int nFinish ) = 0;

	virtual bool MergePath( IStaticPath *pAppendant, const int nStartTile ) = 0;

	virtual int MarkStaticPath( const int nID, const NDebugInfo::EColor color ) const = 0;
};


