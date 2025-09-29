#pragma once

#include "LongObjectCreation.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFullBridge;
class CBridgeSpan;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBridgeCreation : public CLongObjectCreation
{
	OBJECT_BASIC_METHODS( CBridgeCreation );
	// для сортировки
	struct SBridgeSpanSort
	{
		bool operator()( const CObj<CBridgeSpan> &s1, const CObj<CBridgeSpan> &s2 );
	};

	ZDATA_(CLongObjectCreation)
	CObj<CFullBridge> pFullBridge;
	vector< CObj<CBridgeSpan> > spans;
	CVec2 vStartPoint;
	CLine2 line;
	int nCurIndex;
	SAIAngle wDir;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLongObjectCreation*)this); f.Add(2,&pFullBridge); f.Add(3,&spans); f.Add(4,&vStartPoint); f.Add(5,&line); f.Add(6,&nCurIndex); f.Add(7,&wDir); return 0; }
	void UnlockTiles();
	void LockTiles();
public:
	CBridgeCreation() : CLongObjectCreation( - 1, false ) {  }
	CBridgeCreation( class CFullBridge *pBridge, class CCommonUnit *pUnit, const bool bAllowAIModification );

	static CVec2 SortBridgeSpans( vector< CObj<CBridgeSpan> > *spans, class CCommonUnit *pUnit );

	//specific
	const CVec2 & GetStartPoint() const;	// куда посылать грузовик
	bool IsFirstSegmentBuilt() const;

	// common
	CLine2 GetCurLine();
	const int GetMaxIndex() const;
	const int GetCurIndex() const;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const;
	void BuildNext();
	float GetPrice();

	//
	bool IsAnyUnitPrevent() const { return false; }
	bool CanBuildNext() const { return true; }
	void LockNext() { }
	void LockCannotBuild() { }
	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock ) { return true; } 
	void GetUnitsPreventing( list< CPtr<CAIUnit> > * units ){}
	virtual bool IsCheatPath() const { return true; }
};
