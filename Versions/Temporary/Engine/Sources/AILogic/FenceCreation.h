#pragma once
#include "LongObjectCreation.h"

class CFence;
class CGivenPassabilityStObject;


class CFenceCreation : public CLongObjectCreation
{
	OBJECT_BASIC_METHODS( CFenceCreation );
	// скопировал у Костика
	struct APointHelper
	{
		vector<CVec2> m_points;
		APointHelper() {}
		bool operator() ( long x, long y ) { m_points.push_back( CVec2( x, y ) ); return true; }
	};

	ZDATA_(CLongObjectCreation)
	vector< CObj<CFence> > fenceSegements;	// сегменты
	vector<CVec2> vPoints;					// позиции
	bool bTmpNonCheatPath; // if skipped 1 or more objects - move to another without cheat path

	int nCurIndex;
	list<SVector> tilesUnder;
	SAIAngle wAngle;
	CLine2 line;
	bool bCannot;
	bool bSayAck;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLongObjectCreation*)this); f.Add(2,&fenceSegements); f.Add(3,&vPoints); f.Add(4,&bTmpNonCheatPath); f.Add(5,&nCurIndex); f.Add(6,&tilesUnder); f.Add(7,&wAngle); f.Add(8,&line); f.Add(9,&bCannot); f.Add(10,&bSayAck); return 0; }
private:
	void CalcTilesUnder();
	bool IsCegmentToBeBuilt( class CFence *pObj ) const;
	bool CanPlaceOnTerrain( CGivenPassabilityStObject *pObj ) const;
public:
	CFenceCreation() : CLongObjectCreation( -1, false )  {  }
	CFenceCreation( const int nPlayer, const bool bAllowAIModification );

	const CVec2 & GetCenter( const int nIndex ) { return vPoints[nIndex]; }

	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock );
	CLine2 GetCurLine() { return line; }
	const int GetMaxIndex() const;
	const int GetCurIndex() const;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const;
	void BuildNext();
	void GetUnitsPreventing( list< CPtr<CAIUnit> > * units );
	bool IsAnyUnitPrevent() const;
	bool CanBuildNext() const; 
	void LockNext();
	float GetPrice();
	void LockCannotBuild(){ bCannot = true; }
	void CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate );
	bool IsCheatPath() const;
};

