#pragma once

#include "Common_RTS_AI/TerraAIObserver.h"

#include <cstdint>

typedef std::list<CVec2> CRiverSounds;

class CTerraAIObserverInGame : public CTerraAIObserver
{
	OBJECT_NOCOPY_METHODS( CTerraAIObserverInGame )

	static CRiverSounds riverSounds;

	void InitSizes( const int nSizeX, const int nSizeY );
	void AddVSO( const NDb::SVSOInstance *pVSO ) override;

public:
	CTerraAIObserverInGame() {}

	CTerraAIObserverInGame( bool bOnlyForRiversInit ) {} //cheat

	CTerraAIObserverInGame( const int nSizeX, const int nSizeY );

	~CTerraAIObserverInGame() override {}

	void
	UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heights ) override;
	void
	UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<uint8_t> &types ) override;

	void AddRiver( const NDb::SVSOInstance *pInstance ) override;
	void FinalizeUpdates() override;

	static void InitInGame();
	int operator&( IBinSaver &saver ) override;
};
