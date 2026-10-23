#pragma once

#include "LongObjectCreation.h"

#include "AILogic_export.h"

class CFullBridge;
class CBridgeSpan;

class CBridgeCreation : public CLongObjectCreation
{
	OBJECT_BASIC_METHODS( CBridgeCreation );

	// для сортировки
	struct SBridgeSpanSort
	{
		bool operator()( const CObj<CBridgeSpan> &s1, const CObj<CBridgeSpan> &s2 );
	};

	ZDATA_( CLongObjectCreation )
	CObj<CFullBridge> pFullBridge;
	std::vector<CObj<CBridgeSpan>> spans;
	CVec2 vStartPoint;
	CLine2 line;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nCurIndex = {};
	SAIAngle wDir;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CLongObjectCreation *)this );
		f.Add( 2, &pFullBridge );
		f.Add( 3, &spans );
		f.Add( 4, &vStartPoint );
		f.Add( 5, &line );
		f.Add( 6, &nCurIndex );
		f.Add( 7, &wDir );
		return 0;
	}

	void UnlockTiles();
	void LockTiles();

public:
	CBridgeCreation()
		: CLongObjectCreation( -1, false )
	{
	}

	CBridgeCreation( class CFullBridge *pBridge, class CCommonUnit *pUnit, const bool bAllowAIModification );

	AILOGIC_EXPORT static CVec2 SortBridgeSpans( std::vector<CObj<CBridgeSpan>> *spans, class CCommonUnit *pUnit );

	//specific
	const CVec2 &GetStartPoint() const; // куда посылать грузовик
	bool IsFirstSegmentBuilt() const;

	// common
	CLine2 GetCurLine() override;
	const int GetMaxIndex() const override;
	const int GetCurIndex() const override;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const override;
	void BuildNext() override;
	float GetPrice() override;

	//
	bool IsAnyUnitPrevent() const override
	{
		return false;
	}

	bool CanBuildNext() const override
	{
		return true;
	}

	void LockNext() override {}

	void LockCannotBuild() override {}

	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock ) override
	{
		return true;
	}

	void GetUnitsPreventing( std::list<CPtr<CAIUnit>> *units ) override {}

	bool IsCheatPath() const override
	{
		return true;
	}
};
