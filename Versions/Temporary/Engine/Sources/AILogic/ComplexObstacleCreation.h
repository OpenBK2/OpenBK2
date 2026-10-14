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
	ZDATA_( CLongObjectCreation )
	std::vector<CObj<CCommonStaticObject>> antitanks;
	CPtr<CFenceCreation> pFenceCreation;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nCurIndex = {};
	bool bCannot = {};
	std::list<SVector> tilesUnder;
	bool bTmpNonCheatPath = {}; // if skipped 1 or more objects - move to another without cheat path
public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CLongObjectCreation *)this );
		f.Add( 2, &antitanks );
		f.Add( 3, &pFenceCreation );
		f.Add( 4, &nCurIndex );
		f.Add( 5, &bCannot );
		f.Add( 6, &tilesUnder );
		f.Add( 7, &bTmpNonCheatPath );
		return 0;
	}

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
	CComplexObstacleCreation()
		: CLongObjectCreation( -1, false )
	{
	}

	CComplexObstacleCreation( const int _nPlayer, const bool bAllowAIModification )
		: nCurIndex( 0 )
		, CLongObjectCreation( _nPlayer, bAllowAIModification )
		, bTmpNonCheatPath( false )
		, bCannot( false )
	{
	}

	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock ) override;
	const int GetMaxIndex() const override;
	const int GetCurIndex() const override;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const override;
	void BuildNext() override;
	void GetUnitsPreventing( std::list<CPtr<CAIUnit>> *units ) override;
	bool IsAnyUnitPrevent() const override;
	bool CanBuildNext() const override;
	void LockCannotBuild() override;
	void LockNext() override;
	CLine2 GetCurLine() override;
	float GetPrice() override;
	bool IsCheatPath() const override;
	bool CannotFinish() const override;
	void AddWork( const float fAdd ) override;
	float GetWorkDone() const override;
	void CreateObjects( SAIObjectsUnderConstructionUpdate *pUpdate ) override;
};
