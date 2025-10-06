#pragma once

#include "Diplomacy.h"
#include "Units.h"
#include "Common_RTS_AI/AIMap.h"

extern CUnits units;

template<BYTE cOnlyOneTypeVisibility, int NSize>
class CUnitsIter
{
	CVec2 vDownLeft;
	CVec2 vUpRight;

	int nDownX, nDownY, nUpX, nUpY;
	BYTE cStartDipl;
	BYTE cCurDipl;
	BYTE cDiplEnd;
	BYTE cCurMech;
	BYTE cMechEnd;

	BYTE cCurVis;

	int nXCell, nYCell;

	CUnitsIter<1, NSize-1> iter;

	//
	void Init( BYTE _cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		nDownX = vDownLeft.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nDownY = vDownLeft.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpX = vUpRight.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpY = vUpRight.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );

		cStartDipl = _cStartDipl;
		cCurDipl = cStartDipl;
		cDiplEnd = cStartDipl + cDiplomacies;
		cCurMech = cStartMech;
		cMechEnd = cStartMech + cMechs;

		nXCell = nDownX - 1;
		nYCell = nDownY;
	}

	void IterateByCells()
	{
		++nXCell;
		if ( nXCell > nUpX )
		{
			++nYCell;
			nXCell = nDownX;

			if ( nYCell > nUpY )
			{
				if ( !cOnlyOneTypeVisibility && cCurVis < 1 )
					cCurVis = 1;
				else
				{
					if ( !cOnlyOneTypeVisibility )
						cCurVis = 0;

					++cCurDipl;
					if ( cCurDipl >= cDiplEnd )
					{
						cCurDipl = cStartDipl;
						++cCurMech;
						if ( cCurMech >= cMechEnd )
							return;
					}
				}

				nYCell = nDownY;
			}
		}
	}

	void InitLowIterator()
	{
		const CVec2 vCellDownLeft( nXCell * ( 1 << NSize ) * SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF, nYCell * ( 1 << NSize ) * SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF );
		const CVec2 vCellUpRight( (nXCell + 1 ) * ( 1 << NSize ) * SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF - 1, (nYCell + 1) * ( 1 << NSize ) * SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF - 1 );

		const CVec2 vLowIterDownLeft( (std::max)( vCellDownLeft.x, vDownLeft.x ), (std::max)( vCellDownLeft.y, vDownLeft.y ) );
		const CVec2 vLowIterUpRight( (std::min)( vCellUpRight.x, vUpRight.x ), (std::min)( vCellUpRight.y, vUpRight.y ) );

		iter.Init4HighIter( vLowIterDownLeft, vLowIterUpRight, cCurDipl, 1, cCurMech, 1, cCurVis );
	}

	void InitAll( const CVec2 &vCenter, const CVec2 &vAABBHalfSize, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		vDownLeft.x = Clamp( vCenter.x - vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vDownLeft.y = Clamp( vCenter.y - vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );
		vUpRight.x = Clamp( vCenter.x + vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vUpRight.y = Clamp( vCenter.y + vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
		Iterate();
	}
public:
	CUnitsIter() : cMechEnd( 0 ), cCurMech( 1 ) { }

	CUnitsIter( const BYTE _cStartDipl, BYTE cDiplFilter, const CVec2 &vCenter, const float &fR, bool bOnlyMech = false )
	{
		BYTE cStartMech = 0;
		BYTE cMechs = 1;

		if ( !bOnlyMech )
			cMechs = 2;

		BYTE cStartDipl = 0;
		BYTE cDiplomacies = 0;
		if ( cDiplFilter == ANY_PARTY ) { cStartDipl = 0; cDiplomacies = 3; }
		else if ( _cStartDipl == 2 )
		{
			if ( cDiplFilter == EDI_ENEMY )
			{
				cCurMech = 1; cMechEnd = 0;
				return;
			}
			else { cStartDipl = 0; cDiplomacies = 2; }
		}
		else if ( cDiplFilter == EDI_FRIEND ) { cStartDipl = _cStartDipl; cDiplomacies = 1; }
		else if ( cDiplFilter == EDI_ENEMY ) { cStartDipl = 1 - _cStartDipl; cDiplomacies = 1; }

		cCurVis = cOnlyOneTypeVisibility;

		InitAll( vCenter, CVec2( fR, fR ), cStartDipl, cDiplomacies, cStartMech, cMechs );
	}
	
	void Init4HighIter( const CVec2 &_vDownLeft, const CVec2 &_vUpRight, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs, BYTE cHighIterVis )
	{
		vDownLeft = _vDownLeft;
		vUpRight = _vUpRight;

		cCurVis = cHighIterVis;

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
		Iterate();
	}

	void Iterate()
	{
		bool bFinished = false;
		while ( !bFinished )
		{
			if ( !iter.IsFinished() )
				iter.Iterate();

			while ( iter.IsFinished() && !IsFinished() )
			{
				if ( iter.IsFinished() )
					IterateByCells();

				while ( !IsFinished() && units.numUnits[cCurVis][NSize-1][cCurDipl][cCurMech][nYCell][nXCell] == 0 )
					IterateByCells();
				if ( !IsFinished() )
					InitLowIterator();
			}

			bFinished = IsFinished();
			if ( !bFinished ) 
			{
				CObjectBase *pCheckUnit = CastToObjectBase( operator*() );
				bFinished = pCheckUnit && pCheckUnit->IsRefValid();
			}
		}
	}

	class CAIUnit* operator*() const { return *iter; }
	const bool IsFinished() const { return cCurMech >= cMechEnd; }
};

template<>
class CUnitsIter<1, 0>
{
	enum
	{
		cOnlyOneTypeVisibility = 1,
		NSize = 0,
	};

	CVec2 vDownLeft;
	CVec2 vUpRight;

	int nDownX, nDownY, nUpX, nUpY;
	BYTE cStartDipl;
	BYTE cCurDipl;
	BYTE cDiplEnd;
	BYTE cCurMech;
	BYTE cMechEnd;
	BYTE cCurVis;

	int nXCell, nYCell;
	int nIter;

	//
	void IterateByCells()
	{
		++nXCell;
		if ( nXCell > nUpX )
		{
			++nYCell;
			nXCell = nDownX;

			if ( nYCell > nUpY )
			{
				if ( !cOnlyOneTypeVisibility && cCurVis < 1)
				 cCurVis = 1;
				else
				{
					if ( !cOnlyOneTypeVisibility )
						cCurVis = 0;

					++cCurDipl;
					if ( cCurDipl >= cDiplEnd )
					{
						cCurDipl = cStartDipl;
						++cCurMech;
						if ( cCurMech >= cMechEnd )
							return;
					}
				}

				nYCell = nDownY;
			}
		}
	}

	int GetCurCellIter() const
	{
		const int nCellID = units.nCell[nYCell][nXCell] * 2 * 3 + 2 * cCurDipl + cCurMech + 1;
		return units.unitsInCells[cCurVis].begin( nCellID );
	}
	
	void Init( BYTE _cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		nDownX = vDownLeft.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nDownY = vDownLeft.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpX = vUpRight.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpY = vUpRight.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );

		cStartDipl = _cStartDipl;
		cCurDipl = cStartDipl;
		cDiplEnd = cStartDipl + cDiplomacies;
		cCurMech = cStartMech;
		cMechEnd = cStartMech + cMechs;

		nXCell = nDownX - 1;
		nYCell = nDownY;

		do
		{
			IterateByCells();
		} while ( !IsFinished() && ( units.nUnitsCell[nYCell][nXCell] == 0 || GetCurCellIter() == 0 ) );

		if ( !IsFinished() )
			nIter = GetCurCellIter();
	}

	void InitAll( const CVec2 &vCenter, const CVec2 &vAABBHalfSize, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		vDownLeft.x = Clamp( vCenter.x - vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vDownLeft.y = Clamp( vCenter.y - vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );
		vUpRight.x = Clamp( vCenter.x + vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vUpRight.y = Clamp( vCenter.y + vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
	}
public:
	CUnitsIter() : cMechEnd( 0 ), cCurMech( 1 ) { }
	CUnitsIter( const BYTE _cStartDipl, BYTE cDiplFilter, const CVec2 &vCenter, const float &fR, bool bOnlyMech = false )
	{
		BYTE cStartMech = 0;
		BYTE cMechs = 1;

		if ( !bOnlyMech )
			cMechs = 2;

		BYTE cStartDipl, cDiplomacies;
		if ( cDiplFilter == ANY_PARTY ) { cStartDipl = 0; cDiplomacies = 3; }
		else if ( _cStartDipl == 2 )
		{
			if ( cDiplFilter == EDI_ENEMY )
			{
				cCurMech = 1; cMechEnd = 0;
				return;
			}
			else { cStartDipl = 0; cDiplomacies = 2; }
		}
		else if ( cDiplFilter == EDI_FRIEND ) { cStartDipl = _cStartDipl; cDiplomacies = 1; }
		else if ( cDiplFilter == EDI_ENEMY ) { cStartDipl = 1 - _cStartDipl; cDiplomacies = 1; }

		cCurVis = cOnlyOneTypeVisibility;

		InitAll( vCenter, CVec2( fR, fR ), cStartDipl, cDiplomacies, cStartMech, cMechs );
	}
	
	void Init4HighIter( const CVec2 &_vDownLeft, const CVec2 &_vUpRight, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs, BYTE cHighIterVis )
	{
		vDownLeft = _vDownLeft;
		vUpRight = _vUpRight;

		cCurVis = cHighIterVis;

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
	}

	void Iterate()
	{
		bool bFinished = false;
		while ( !bFinished )
		{
			nIter = units.unitsInCells[cCurVis].GetNext( nIter );
			if ( nIter == 0 )
			{
				do
				{
					IterateByCells();
				} while ( !IsFinished() && ( units.nUnitsCell[nYCell][nXCell] == 0 || GetCurCellIter() == 0 ) );

				if ( !IsFinished() )
					nIter = GetCurCellIter();
			}
			bFinished = IsFinished();
			if ( !bFinished ) 
			{
				CObjectBase *pCheckUnit = CastToObjectBase( operator*() );
				bFinished = pCheckUnit && pCheckUnit->IsRefValid();
			}
		}
	}

	class CAIUnit* operator*() const 
	{ 
		return units[units.unitsInCells[cCurVis].GetEl( nIter ).nValue]; 
	}
	const bool IsFinished() const { return cCurMech >= cMechEnd; }
};

template<>
class CUnitsIter<0, 0>
{
	enum
	{
		cOnlyOneTypeVisibility = 0,
		NSize = 0,
	};

	CVec2 vDownLeft;
	CVec2 vUpRight;

	int nDownX, nDownY, nUpX, nUpY;
	BYTE cStartDipl;
	BYTE cCurDipl;
	BYTE cDiplEnd;
	BYTE cCurMech;
	BYTE cMechEnd;
	BYTE cCurVis;

	int nXCell, nYCell;
	int nIter;

	//
	void IterateByCells()
	{
		++nXCell;
		if ( nXCell > nUpX )
		{
			++nYCell;
			nXCell = nDownX;

			if ( nYCell > nUpY )
			{
				if ( !cOnlyOneTypeVisibility && cCurVis < 1)
				 cCurVis = 1;
				else
				{
					if ( !cOnlyOneTypeVisibility )
						cCurVis = 0;

					++cCurDipl;
					if ( cCurDipl >= cDiplEnd )
					{
						cCurDipl = cStartDipl;
						++cCurMech;
						if ( cCurMech >= cMechEnd )
							return;
					}
				}

				nYCell = nDownY;
			}
		}
	}

	int GetCurCellIter() const
	{
		const int nCellID = units.nCell[nYCell][nXCell] * 2 * 3 + 2 * cCurDipl + cCurMech + 1;
		return units.unitsInCells[cCurVis].begin( nCellID );
	}
	
	void Init( BYTE _cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		nDownX = vDownLeft.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nDownY = vDownLeft.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpX = vUpRight.x / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );
		nUpY = vUpRight.y / ( SConsts::TILE_SIZE * SConsts::BIG_CELL_COEFF * ( 1 << NSize ) );

		cStartDipl = _cStartDipl;
		cCurDipl = cStartDipl;
		cDiplEnd = cStartDipl + cDiplomacies;
		cCurMech = cStartMech;
		cMechEnd = cStartMech + cMechs;

		nXCell = nDownX - 1;
		nYCell = nDownY;

		do
		{
			IterateByCells();
		} while ( !IsFinished() && ( units.nUnitsCell[nYCell][nXCell] == 0 || GetCurCellIter() == 0 ) );

		if ( !IsFinished() )
			nIter = GetCurCellIter();
	}

	void InitAll( const CVec2 &vCenter, const CVec2 &vAABBHalfSize, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs )
	{
		vDownLeft.x = Clamp( vCenter.x - vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vDownLeft.y = Clamp( vCenter.y - vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );
		vUpRight.x = Clamp( vCenter.x + vAABBHalfSize.x, 0.0f, float(GetAIMap()->GetSizeX() * SConsts::TILE_SIZE - 1) );
		vUpRight.y = Clamp( vCenter.y + vAABBHalfSize.y, 0.0f, float(GetAIMap()->GetSizeY() * SConsts::TILE_SIZE - 1) );

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
	}
public:
	CUnitsIter() : cMechEnd( 0 ), cCurMech( 1 ) { }
	CUnitsIter( const BYTE _cStartDipl, BYTE cDiplFilter, const CVec2 &vCenter, const float &fR, bool bOnlyMech = false )
	{
		BYTE cStartMech = 0;
		BYTE cMechs = 1;

		if ( !bOnlyMech )
			cMechs = 2;

		BYTE cStartDipl = 0, cDiplomacies = 0;
		if ( cDiplFilter == ANY_PARTY ) { cStartDipl = 0; cDiplomacies = 3; }
		else if ( _cStartDipl == 2 )
		{
			if ( cDiplFilter == EDI_ENEMY )
			{
				cCurMech = 1; cMechEnd = 0;
				return;
			}
			else { cStartDipl = 0; cDiplomacies = 2; }
		}
		else if ( cDiplFilter == EDI_FRIEND ) { cStartDipl = _cStartDipl; cDiplomacies = 1; }
		else if ( cDiplFilter == EDI_ENEMY ) { cStartDipl = 1 - _cStartDipl; cDiplomacies = 1; }

		cCurVis = cOnlyOneTypeVisibility;

		InitAll( vCenter, CVec2( fR, fR ), cStartDipl, cDiplomacies, cStartMech, cMechs );
	}
	
	void Init4HighIter( const CVec2 &_vDownLeft, const CVec2 &_vUpRight, BYTE cStartDipl, BYTE cDiplomacies, BYTE cStartMech, BYTE cMechs, BYTE cHighIterVis )
	{
		vDownLeft = _vDownLeft;
		vUpRight = _vUpRight;

		cCurVis = cHighIterVis;

		Init( cStartDipl, cDiplomacies, cStartMech, cMechs );
	}

	void Iterate()
	{
		bool bFinished = false;
		while ( !bFinished )
		{
			nIter = units.unitsInCells[cCurVis].GetNext( nIter );
			if ( nIter == 0 )
			{
				do
				{
					IterateByCells();
				} while ( !IsFinished() && ( units.nUnitsCell[nYCell][nXCell] == 0 || GetCurCellIter() == 0 ) );

				if ( !IsFinished() )
					nIter = GetCurCellIter();
			}

			bFinished = IsFinished();
			if ( !bFinished ) 
			{
				CObjectBase *pCheckUnit = CastToObjectBase( operator*() );
				bFinished = pCheckUnit && pCheckUnit->IsRefValid();
			}
		}
	}

	class CAIUnit* operator*() const { return units[units.unitsInCells[cCurVis].GetEl( nIter ).nValue]; }
	const bool IsFinished() const { return cCurMech >= cMechEnd; }
};


