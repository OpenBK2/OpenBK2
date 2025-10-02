#pragma once


namespace NDb
{
	struct SSoundDesc;
}

class CSoundManager
{
public:
	static interface ISound* CreateSound2D( const NDb::SSoundDesc *pDesc, const bool bLooped );
	static interface ISound* CreateSound3D( const NDb::SSoundDesc *pDesc, const bool bLooped );
};


