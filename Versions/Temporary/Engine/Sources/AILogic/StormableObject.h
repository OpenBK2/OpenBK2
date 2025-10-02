#ifndef __STORMABLE_OBJECT_H__
#define __STORMABLE_OBJECT_H__

#pragma once
#include "ListsSet.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldier;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*													CStormableObject												*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStormableObject
{

	ZDATA
	CListsSet< CPtr<CSoldier> > attackers; // атакующие для каждой из сторон
	vector<int> nAttackers; // количество атакующих для каждой из сторон
	int nActiveAttackers;				 // количество тех атакующих, которые уже участвуют в бое
	bool bAttackers;						 // есть ли атакующие

	vector<NTimer::STime> startTimes;

	NTimer::STime lastSegment;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&attackers); f.Add(3,&nAttackers); f.Add(4,&nActiveAttackers); f.Add(5,&bAttackers); f.Add(6,&startTimes); f.Add(7,&lastSegment); return 0; }

	//
	void Combat( class CSoldier *pAttacker, class CSoldier *pDefender );
	const float GetDamage( class CBasicGun *pGun, class CSoldier *pTarget ) const;

	void MakeDefenders( const int nParty );
	void DelFromAttackers( CSoldier *pUnit );
	bool FindInAttackers( CSoldier *pUnit ) const;
protected:
	virtual void AddSoldier( class CSoldier *pUnit ) = 0;
	virtual void DelSoldier( class CSoldier *pUnit, const bool bFillEmptyFireplace ) = 0;
	virtual void SoldierDamaged( class CSoldier *pUnit ) = 0;
public:
	CStormableObject() : 
			attackers( SAIConsts::MAX_NUM_OF_PLAYERS ), 
			nAttackers( SAIConsts::MAX_NUM_OF_PLAYERS, 0 ),
			startTimes( SAIConsts::MAX_NUM_OF_PLAYERS, 0 ),
			nActiveAttackers( 0 ),
			bAttackers( false ), 
			lastSegment( 0 ) 
	{
		memset( &nAttackers[0], 0, sizeof( nAttackers[0] * nAttackers.size() ) );
	}

	void AddInsider( class CSoldier *pUnit );
	void DelInsider( class CSoldier *pUnit );
	void InsiderDamaged( class CSoldier *pUnit );

	bool Segment();

	bool IsUnitsInside() const { return bAttackers || GetNDefenders() != 0; }
	const int GetNAttackers( const int nPlayer ) const { return nAttackers[nPlayer]; }
	const int GetNFriendlyAttackers( const int nPlayer ) const;
	bool IsAnyAttackers() const { return bAttackers; }

	virtual const int GetNDefenders() const = 0;
	virtual class CSoldier* GetUnit( const int n ) const = 0;
	virtual const BYTE GetPlayer() const = 0;
	const bool IsAnyInsiderVisible() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __STORMABLE_OBJECT_H__

