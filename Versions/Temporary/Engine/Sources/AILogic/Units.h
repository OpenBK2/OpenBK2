#pragma once

#include "ListsSet.h"
#include "System/FreeIDs.h"
#include "Misc/2DArray.h"

class CAIUnit;
class CAviation;

class CCircleIter;
class CRectIter;
class CLineIter;
class CFormation;

template<class T> class CIter;
class CGlobalIter;
class CPlanesIter;
class CDeadPlanesIter;
namespace NDb
{
	struct SUnitStatsModifier;
}
template<BYTE cOnlyVisible, int nSize> class CUnitsIter;

class CUnits : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CUnits );
public: 
	int operator&( IBinSaver &saver ); 
private:
	struct SInitializedWord
	{
		int nValue;
		SInitializedWord() : nValue ( 0 ) {}
		SInitializedWord( int _nValue ) : nValue ( _nValue ) {}
	};

	typedef hash_map<int/*UniqueID*/, int/*CUnits internal ID*/ > CIDsRemap;
	CIDsRemap idsRemap;
	// все юниты, распределённые по дипл. сторонам
	CListsSet< CObj<CAIUnit> > units;
	hash_map< int, CObj<CFormation> > formations;
	vector<int> sizes;
	// самолёты
	list< CObj<CAviation> > planes;
	// все операции проводятся с большими ячейками

	int nBigCellsSizeX, nBigCellsSizeY;

	// количество юнитов в ячейке
	CArray2D<WORD> nUnitsCell;
	// номер ячейки
	CArray2D<WORD> nCell;
	// список юнитов для каждой из ячеек, 0 - not visible for enemy, 1 - visible for enemy
	vector< CListsSet<SInitializedWord> > unitsInCells;
	// позиция юнита в ячейках
	struct SUnitPosition
	{
		int nCellID; int nUnitPos; SVector cell;
		SUnitPosition() : nCellID(0), nUnitPos(0), cell( 0, 0 ) { }
	};
	vector<SUnitPosition> posUnitInCell;

	enum { N_CELLS_LEVELS = 3 };
	CArray2D<WORD> numUnits[2][N_CELLS_LEVELS][3][2];
	
	// для нумерации ячеек
	CFreeIds cellsIds;
	// для сериализации
	hash_map< int, SVector > cellIdToCoord;


	// CRAP{ for debug
	hash_set<int> unitsInCellsSet;
	// CRAP}
	
	vector< hash_map<int, int> > nUnitsOfType;
	
	//
	void AddUnitToConcreteCell( class CAIUnit* pUnit, const SVector &cell, bool bWithLeveledCelles );
	void AddUnitToCell( class CAIUnit *pUnit, const CVec2 &newPos, bool bWithLeveledCelles );

	void AddUnitToLeveledCells( CAIUnit *pUnit, const SVector &bigCell, const int nVis );
	void DelUnitFromLeveledCells( CAIUnit *pUnit, const SVector &bigCell, const int nVis );
	const bool IsUnitInCell( const int nUnitID ) const;
	CAIUnit* operator[]( const int id );

public:
	CUnits() { }
	
	void Init();
	void Clear() { DestroyContents(); }
	
	void AddFormation( class CFormation *pFormation );
	void DelFormation( class CFormation *pFormation );
	
	// добавить юнит в массив юнитов
	void AddUnitToUnits( class CAIUnit *pUnit, const int nPlayer, const int nUnitType );
	// добавить юнит на карту
	void AddUnitToMap( class CAIUnit *pUnit );
	
	// убирает юнит отовсюду, но не отдаёт его id
	void DeleteUnitFromMap( class CAIUnit *pUnit );
	// окончательно удаляет юнит
	void FullUnitDelete( class CAIUnit *pUnit );

	void AddUnitToCell( class CAIUnit *pUnit, bool bWithLeveledCelles );
	void DelUnitFromCell( class CAIUnit *pUnit, bool bWithLeveledCelles );
	
	void UnitChangedPosition( class CAIUnit *pUnit, const CVec2 &newPos );
	void ChangePlayer( class CAIUnit *pUnit, const BYTE cNewPlayer );
	
	const int Size( const int nParty ) const;
	
	// количество солдат в круге центром vCenter и радиусом fRadius с партией nParty
	// сейчас реализация - просто итерирование внутри круга
	const int GetNSoldiers( const CVec2 &vCenter, const float fRadius, const int nParty );
	const int GetNUnits( const CVec2 &vCenter, const float fRadius, const int nParty );

	const int GetNUnitsOfType( const int nParty, const int nType )
	{
		NI_ASSERT( nParty >= 0 && nParty < 3, StrFmt( "Wrong number of party (%d)", nParty ) );
		return nUnitsOfType[nParty][nType];
	}

	void UpdateUnitVis4Enemy( CAIUnit *pUnit );
	const int GetVisIndex( CAIUnit *pUnit );

	bool IsBigCellInside( const SVector &bigCell ) { return ( bigCell.x >= 0 && bigCell.y >= 0 && bigCell.x < nBigCellsSizeX && bigCell.y < nBigCellsSizeY ); }

	void ApplyModifierToAll( const NDb::SUnitStatsModifier *pBonus, const bool bForward );

	// for debug
	void CheckCorrectness( const SVector &tile );
	void CheckUnitCell();
	
	friend class CIter<CCircleIter>;
	friend class CIter<CRectIter>;
	friend class CIter<CLineIter>;
  friend class CGlobalIter;
	friend class CPlanesIter;
	friend class CDeadPlanesIter;

	friend class CUnitsIter<0,3>;
	friend class CUnitsIter<0,2>;
	friend class CUnitsIter<0,1>;
	friend class CUnitsIter<0,0>;
	friend class CUnitsIter<1,3>;
	friend class CUnitsIter<1,2>;
	friend class CUnitsIter<1,1>;
	friend class CUnitsIter<1,0>;
};


