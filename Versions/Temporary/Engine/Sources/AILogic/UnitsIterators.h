#pragma once

#include "Units.h"
#include "Diplomacy.h"

extern CUnits units;
extern CDiplomacy theDipl;

class CLineIter
{
	struct Cells : public std::list<SVector>
	{
		bool operator() ( long x, long y ) ;
	};
	
	SVector vCurPoint;
	Cells cells;
	
	void GetNext();
public:
	
	CLineIter( const CVec2 &vStart, const CVec2 &vFinish );

	void Iterate()
	{
		GetNext();
	}

	const int GetX() const { return vCurPoint.x; }
	const int GetY() const { return vCurPoint.y; }

	const bool IsFinished() const;
};

// only visible units!
template<class T>
class CIter
{
	T geomIter;
	int nCurParty, nParties, nCellId, iter;
	std::vector<BYTE> parties;

	public: virtual int operator&( IBinSaver &saver ) {  saver.Add( 1, &geomIter ); saver.Add( 2, &nCurParty ); saver.Add( 3, &nParties ); saver.Add( 4, &nCellId ); saver.Add( 5, &iter ); saver.Add( 6, &parties ); return 0; } private:
public:
	CIter( const BYTE cStartDipl, const BYTE cFilter, const T &_geomIter, bool bOnlyMech = false )
		: geomIter( _geomIter ), parties( 2 * 3 /*SAIConsts::MAX_NUM_OF_PARTIES*/ )
	{
		nParties = 0;
		for ( int i = 0; i < 3; ++i )
		{
			if ( theDipl.GetDiplStatusForParties( i, cStartDipl ) & cFilter )
			{
				parties[nParties++] = 2 * i;
				if ( !bOnlyMech )
					parties[nParties++] = 2 * i + 1;
			}
		}

		while ( !geomIter.IsFinished() && units.nUnitsCell[geomIter.GetY()][geomIter.GetX()] == 0 )
			geomIter.Iterate();
		nCurParty = -1; iter = 0; nCellId = 0;
		Iterate();
	}

	void Iterate()
	{
		if ( nCellId )
			iter = units.unitsInCells[1].GetNext( iter );

		while ( ( iter == 0 || operator*() == 0 ) && !geomIter.IsFinished() )
		{
			if ( nCurParty < nParties - 1 )
			{
				++nCurParty;
				// 3 - это количество сторон
				nCellId = units.nCell[geomIter.GetY()][geomIter.GetX()] * 2 * 3 + parties[nCurParty] + 1;

				iter = units.unitsInCells[1].begin( nCellId );
			}
			else
			{
				nCurParty = -1;
				do
				{
					geomIter.Iterate();
				} while ( !geomIter.IsFinished() && units.nUnitsCell[geomIter.GetY()][geomIter.GetX()] == 0 );
			}
		}
	}

	class CAIUnit* operator*() const { return units[units.unitsInCells[1].GetEl( iter )]; }
	const bool IsFinished() const { return geomIter.IsFinished(); }
};

class CGlobalIter
{
ZDATA	
	int iter;
	int nCurParty;
	int nParties;
	std::vector<BYTE> parties;
	det_set<int> visitedUnits;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&iter); f.Add(3,&nCurParty); f.Add(4,&nParties); f.Add(5,&parties); f.Add(6,&visitedUnits); return 0; }
public:
	CGlobalIter() : parties( 3/*SAIConsts::MAX_NUM_OF_PARTIES*/ ) { }
	CGlobalIter( const BYTE cStartDipl, const BYTE cFilter ) : parties( 3/*SAIConsts::MAX_NUM_OF_PARTIES*/ ) { Init( cStartDipl, cFilter ); }

	void Init( const BYTE cStartDipl, const BYTE cFilter );

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


