
#pragma once
#include "Misc/2Darray.h"
#include "IntPair.h"


typedef std::list<SIntThree/*hearable cell coordinate*/> CHearableCells;
typedef CArray2D<CHearableCells> CConglomerate;
typedef std::vector<CConglomerate> CConglomerates;


class CCellsConglomerateContainer
{
	struct SAddCellEnumerator
	{
		const SIntThree vCenter;
		CConglomerates *pConglomerates;
		SAddCellEnumerator( const SIntThree &_vCenter ) : vCenter( _vCenter ), pConglomerates( 0 ) {  }
		void operator()( const int nCurRank, const int nX, const int nY ) 
		{ 
			(*pConglomerates)[nCurRank][nY][nX].push_back( vCenter ); 
		}
		int GetSizeX() const { return (*pConglomerates)[0].GetSizeX(); }
		int GetSizeY() const { return (*pConglomerates)[0].GetSizeY(); }
	};
	
	struct SRemoveCellEnumerator
	{
		const SIntThree vCenter;
		CConglomerates *pConglomerates;
		SRemoveCellEnumerator( const SIntThree &_vCenter ) : vCenter( _vCenter ), pConglomerates( 0 ) {  }
		void operator()( const int nCurRank, const int nX, const int nY ) 
		{ 
			(*pConglomerates)[nCurRank][nY][nX].remove( vCenter ); 
		}
		int GetSizeX() const { return (*pConglomerates)[0].GetSizeX(); }
		int GetSizeY() const { return (*pConglomerates)[0].GetSizeY(); }
	};

	static const int MAX_RANK;					// maximum allowed rank
	int nMaxRank;												// current rank
	std::vector<CConglomerates> conglomeratesHeight;
	bool bInitted;
	// do not need to store phs map in levels less than nMinZ
	int nMinZ;
public:
	CCellsConglomerateContainer() : bInitted( false ) {  }
	void Init( const int nMaxX, const int nMaxY, const int nMinZ, const int nMaxZ );
	
	void AddHearCell( const SIntThree &vSourceCell, const int nRadius );
	void RemoveHearCell( const SIntThree &vSourceCell, const int nRadius );
	bool IsInitted() const { return bInitted; }

	void Clear();

	// для всех клеток, звуки из которых слышны
	template <class TEnumFunc> 
		void EnumHearableCells( TEnumFunc func, const SIntThree &center )
	{
		for ( int z = 0; z < conglomeratesHeight.size(); ++z )
		{
			CConglomerates &conglomerates = conglomeratesHeight[z];

			for ( int nRank = 0; nRank <= nMaxRank; ++nRank )
			{
				const int nConglomerateSize = 1<<nRank;
				const int nX = center.x / nConglomerateSize;
				const int nY = center.y / nConglomerateSize;
				const CConglomerate &conglomerate = conglomerates[nRank];
				const CHearableCells & cells = conglomerate[nY][nX];
				for ( CHearableCells::const_iterator it = cells.begin(); it != cells.end(); ++it )
					func( abs( center.x - it->x ) + abs( center.y - it->y ) + abs( center.z - it->z ),
								*it );
			}
		}
	}
	int operator&( IBinSaver &saver );
};


