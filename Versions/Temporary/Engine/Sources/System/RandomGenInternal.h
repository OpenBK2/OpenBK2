#pragma once

#include "RandomGen.h"

#include <cstdint>

const int RANDSIZL = 8;
const int RANDSIZ = 1 << RANDSIZL;

struct SRandData
{
	uint32_t randcnt;
	uint32_t randrsl[RANDSIZ];
	uint32_t randmem[RANDSIZ];
	uint32_t randa;
	uint32_t randb;
	uint32_t randc;
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


