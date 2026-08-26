#pragma once

#include "Units.h"
#include "Diplomacy.h"

#include <cstdint>

extern CUnits units;
extern CDiplomacy theDipl;

class CGlobalIter
{
ZDATA	
	int iter;
	int nCurParty;
	int nParties;
	std::vector<uint8_t> parties;
	det_set<int> visitedUnits;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&iter); f.Add(3,&nCurParty); f.Add(4,&nParties); f.Add(5,&parties); f.Add(6,&visitedUnits); return 0; }
public:
	CGlobalIter() : parties( 3/*SAIConsts::MAX_NUM_OF_PARTIES*/ ) { }
	CGlobalIter( const uint8_t cStartDipl, const uint8_t cFilter ) : parties( 3/*SAIConsts::MAX_NUM_OF_PARTIES*/ ) { Init( cStartDipl, cFilter ); }

	void Init( const uint8_t cStartDipl, const uint8_t cFilter );

	void Iterate();
	CAIUnit* operator*() const;
	const bool IsFinished() const { return iter == 0; }
};

class CPlanesIter
{
	private: int operator&( IBinSaver &saver ); private:

	std::list< CObj<CAviation> >::iterator iter;
public:
	CPlanesIter();

	void Iterate();
	class CAviation* operator*() const;
	const bool IsFinished() const;
};


