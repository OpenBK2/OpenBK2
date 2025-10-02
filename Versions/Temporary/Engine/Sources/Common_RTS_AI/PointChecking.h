#pragma once

interface IPointChecking : public CAIObjectBase
{
	virtual bool IsGoodTile( const SVector &curTile ) const = 0;
};

