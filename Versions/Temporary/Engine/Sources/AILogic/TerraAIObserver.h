#pragma once

#include "Common_RTS_AI/TerraAIObserver.h"

typedef std::list<CVec2> CRiverSounds;

class CTerraAIObserverInGame :	public CTerraAIObserver
{
	OBJECT_NOCOPY_METHODS( CTerraAIObserverInGame )

	static CRiverSounds riverSounds;

	void InitSizes( const int nSizeX, const int nSizeY );
	virtual void AddVSO( const NDb::SVSOInstance *pVSO );

public:
	CTerraAIObserverInGame() {  }
	CTerraAIObserverInGame( bool bOnlyForRiversInit ) {  } //cheat
	CTerraAIObserverInGame( const int nSizeX, const int nSizeY );
	virtual ~CTerraAIObserverInGame() {}

	virtual void UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heights );
	virtual void UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<BYTE> &types );

	void AddRiver( const NDb::SVSOInstance *pInstance );
	virtual void FinalizeUpdates();

	static void InitInGame();
	int operator&( IBinSaver &saver );
};


