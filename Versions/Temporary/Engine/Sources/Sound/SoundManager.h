#pragma once


namespace NDb
{
	struct SSoundDesc;
}

class CSoundManager
{
public:
	static struct ISound* CreateSound2D( const NDb::SSoundDesc *pDesc, const bool bLooped );
	static struct ISound* CreateSound3D( const NDb::SSoundDesc *pDesc, const bool bLooped );
};


