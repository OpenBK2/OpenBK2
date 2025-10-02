#pragma once

#include "RandomGen.h"

const int RANDSIZL = 8;
const int RANDSIZ = 1 << RANDSIZL;

struct SRandData
{
	unsigned _int32 randcnt;
	unsigned _int32 randrsl[RANDSIZ];
	unsigned _int32 randmem[RANDSIZ];
	unsigned _int32 randa;
	unsigned _int32 randb;
	unsigned _int32 randc;
};

class CRandomGenSeed : public IRandomSeed
{
	OBJECT_BASIC_METHODS( CRandomGenSeed );
	//
	SRandData rnd;
	//
	bool RecFindFile( LPSTR pszFindedName, LPCSTR pszBaseMask, int nToFind, int* pnTotFinded );
	void FillRandRsl();
	void __declspec(dllexport) SFLB0_InitVariables();
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

