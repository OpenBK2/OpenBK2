#pragma once

struct IPointChecking : public CAIObjectBase
{
	virtual bool IsGoodTile( const SVector &curTile ) const = 0;
};


