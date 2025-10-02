#pragma once

#include "LongObjectCreation.h"
class CCommonStaticObject;
class CFenceCreation;

// builds obstacle as shown. "X" - antitank, "-" - wire  fence
//
//		X  X  X
//	-----------
//
class CComplexObstacleCreation : public CLongObjectCreation
{
	OBJECT_BASIC_METHODS( CComplexObstacleCreation )
	ZDATA_(CLongObjectCreation)
		vector< CObj<CCommonStaticObject> > antitanks;
	CPtr<CFenceCreation> pFenceCreation;
	int nCurIndex;
	bool bCannot;
	list<SVector> tilesUnder;
	bool bTmpNonCheatPath; // if skipped 1 or more objects - move to another without cheat path
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLongObjectCreation*)this); f.Add(2,&antitanks); f.Add(3,&pFenceCreation); f.Add(4,&nCurIndex); f.Add(5,&bCannot); f.Add(6,&tilesUnder); f.Add(7,&bTmpNonCheatPath); return 0; }
	const bool IsAntitank( const int _nIndex ) const
	{
		return _nIndex < antitanks.size();
	}
	const int GetAntitankIndex( const int _nIndex ) const
	{
		return _nIndex;
	}
	const int GetWireFenceIndex( const int _nIndex ) const 
	{
		return _nIndex - antitanks.size();
	}

	void AdvanceToNextIndex();

public:
	CComplexObstacleCreation() : CLongObjectCreation( -1, false ) {  }
	CComplexObstacleCreation( const int _nPlayer, const bool bAllowAIModification ) 
		: nCurIndex( 0 ), CLongObjectCreation( _nPlayer, bAllowAIModification ),
		bTmpNonCheatPath( false ), bCannot( false )
	{  
	}
	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock );
	const int GetMaxIndex() const;
	const int GetCurIndex() const;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const;
	void BuildNext();
	void GetUnitsPreventing( list< CPtr<CAIUnit> > * units );
	bool IsAnyUnitPrevent() const;
	bool CanBuildNext() const;
	void LockCannotBuild();
	void LockNext();
	CLine2 GetCurLine();
	float GetPrice();
	bool IsCheatPath() const;
	bool CannotFinish() const;
	void AddWork( const float fAdd );
	float GetWorkDone() const;
	void CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate );
};
