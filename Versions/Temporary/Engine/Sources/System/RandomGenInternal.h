#pragma once

#include "RandomGen.h"

#include <cstdint>

const int RANDSIZL = 8;
const int RANDSIZ = 1 << RANDSIZL;

struct SRandData
{
	// Seed initialization and restoration overwrite these defensive defaults.
	uint32_t randcnt = 0;
	uint32_t randrsl[RANDSIZ]{};
	uint32_t randmem[RANDSIZ]{};
	uint32_t randa = 0;
	uint32_t randb = 0;
	uint32_t randc = 0;
};

class CRandomGenSeed : public IRandomSeed
{
	OBJECT_BASIC_METHODS( CRandomGenSeed );
	//
	SRandData rnd;
	//
	void SFLB0_InitVariables();
public:
	void Init();
	void InitByZeroSeed();
	//
	const SRandData& GetRandData() const { return rnd; }
	void SetRandData( const SRandData &_rnd ) { rnd = _rnd; }
	//
	void Store( CDataStream *pStream );
	void Restore( CDataStream *pStream );
	//
	int operator&( IBinSaver &saver );
	int operator&( IXmlSaver &saver );
};


