#ifndef __GRID_CREATION_H__
#define __GRID_CREATION_H__

#pragma ONCE

class CSortedGridUnits
{
	vector<class CCommonUnit*> units;
	int nUnits;

	CVec2 vAverageDir;

	struct SUnitsCompare
	{
		static CSortedGridUnits *pSortedUnits;
		bool operator()( const int a, const int b ) const;
	};

	vector<int> sortedUnitsNums;
	vector<CVec2> rotatedUnitsCoord;

	bool bSorted;
public:
	CSortedGridUnits();
	void AddUnit( class CCommonUnit* pUnit );

	void Sort();

	const CVec2 GetAverageDir() const;
	void SetDir( const CVec2 &vDir ) { vAverageDir = vDir; }
	const int GetSize() const { return nUnits; }
	const CVec2 GetAABB( const int n ) const;
	const int GetUnitByOrderNumber( const int n ) const;

	CCommonUnit* GetUnit( const int n ) const;

	bool IsFormation( const int n ) const;

	friend struct SUnitsCompare;
};

class CGrid
{
	struct SColumnCompare
	{
		static CGrid *pGrid;
		bool operator()( const int a, const int b ) const;
	};
	
	typedef vector<int> CSortedColumns;
	ZDATA
	CSortedColumns sortedColumns;
	vector<float> columns;

	CVec2 vCenter;

	vector<CVec2> newCenters;
	float fMaxWidth;

	CSortedGridUnits sortedUnits;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&sortedColumns); f.Add(3,&columns); f.Add(4,&vCenter); f.Add(5,&newCenters); f.Add(6,&fMaxWidth); f.Add(7,&sortedUnits); return 0; }
	//
	bool CanPlaceUnitToColumn( const int nColumn, const CVec2 &vAABBUnitSize );
	void SetUnitCenterByColumn( const int nUnit, const int nColumn );
	
	void MoveCenterToRealWorld( CVec2 *pCenter );
	void CalculateMaxWidth();

	const float GetBWColumnsSpace() const;
	const float GetRowHeight( const float fUnitHeight ) const;
	const float GetColumnWidth() const;

	void CreateGrid();
public:
	CGrid( const CVec2 &vGridCenter, const int nGroup, const CVec2 &vGridDir );

	const CVec2 GetDir() const { return sortedUnits.GetAverageDir(); }

	const int GetNUnitsInGrid() const;
	const CVec2 GetUnitCenter( const int n ) const;
	class CCommonUnit* GetUnit( const int n ) const;

	friend struct SColumnCompare;
};

#endif // __GRID_CREATION_H__
