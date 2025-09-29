#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "LongObjectCreation.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEntrenchmentPart;
class CEntrenchment;
namespace NDb
{
	struct SEntrenchmentRPGStats; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEntrenchmentCreation : public CLongObjectCreation
{
	OBJECT_BASIC_METHODS( CEntrenchmentCreation );
	float GetTrenchWidth( int nType );// 0 - секция , 1 - поворот
	typedef vector< CObj<CEntrenchmentPart> > CParts;
	bool CanBuildNextInner() const;
	bool CanBuildNextInnerSlow( const int _nIndex ) const;

	ZDATA_(CLongObjectCreation)
	int nStartIndex;
	CDBPtr<SEntrenchmentRPGStats> pEntrenchmentStats;
	CObj<CEntrenchment> pFullEntrenchment;
	CParts parts;
	
	CObj<CEntrenchmentPart> pBeginTerminator;		//
	CObj<CEntrenchmentPart> pEndTerminator;			// текущий конечный терминатор
	CObj<CEntrenchmentPart> pNewEndTerminator;	// будуший конечный терминатор
	
	vector<CVec2> vPoints;						// центры окопов
	//
	int nCurIndex;
	SAIAngle wAngle;
	CLine2 line;
	bool bCannot;
	bool bSayAck;

	list<SVector> tilesUnder;									// ТАйлы под следующим сегментом

	//consts
	int nTermInd;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLongObjectCreation*)this); f.Add(2,&nStartIndex); f.Add(3,&pEntrenchmentStats); f.Add(4,&pFullEntrenchment); f.Add(5,&parts); f.Add(6,&pBeginTerminator); f.Add(7,&pEndTerminator); f.Add(8,&pNewEndTerminator); f.Add(9,&vPoints); f.Add(10,&nCurIndex); f.Add(11,&wAngle); f.Add(12,&line); f.Add(13,&bCannot); f.Add(14,&bSayAck); f.Add(15,&tilesUnder); f.Add(16,&nTermInd); return 0; }
	void OnSerialize( IBinSaver &saver );
	//
	bool CanDig( const NDb::SEntrenchmentRPGStats *pRPG, const CVec2 &pt, WORD angle, int nFrameIndex );
	CEntrenchmentPart * AddElement( const NDb::SEntrenchmentRPGStats *pRPG, const CVec3 &pt, WORD angle, int nFrameIndex, int nPlayer );
	void CreateNewEndTerminator();
	void CalcTilesUnder();
	void GetTilesUnderForIndex( list<SVector> *pTiles, const int nIndex ) const;
	void InitConsts();
	bool CanDigBecauseOfOtherTrenches( const SRect &rect, const CVec2 &pt ) const;
	bool CanDigTile( const SVector &tile ) const;
public:
	CEntrenchmentCreation() : CLongObjectCreation( -1, false ) { }
	CEntrenchmentCreation( const int nPlayer, const bool bAllowAIModification );
	
	static bool SearchTrenches( const CVec2 &vCenter, const SRect &rectToTest );

	int GetEntrenchmentID() const;

	bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock );
	virtual CLine2 GetCurLine() { return line; }
	const int GetMaxIndex() const;
	const int GetCurIndex() const;
	const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const;
	const CVec2 GetBuildPointForIndex( const int nPlace, const int nMaxPlace, const int _nIndex ) const;
	void BuildNext();
	void BuildAll( const int _nMinIndex, const int _nMaxIndex );
	void GetUnitsPreventing( list< CPtr<CAIUnit> > * units );
	void GetUnitsPreventingByIndex( list< CPtr<CAIUnit> > *units, const int _nIndex );
	bool IsAnyUnitPrevent() const;
	bool IsAnyUnitPreventByIndex( const int _nIndex ) const;
  bool CanBuildNext() const; 
	bool CanBuildByIndexSlow( const int _nIndex ) const;
	void LockNext();
	float GetPrice();
	void LockCannotBuild();
	bool CannotFinish() const { return bSayAck; }
	void CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
