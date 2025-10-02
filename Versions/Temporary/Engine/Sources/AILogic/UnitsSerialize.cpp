#include "stdafx.h"

#include "UnitsIterators.h"

int CUnits::operator&( IBinSaver &saver )
{
	saver.Add( 1, &units );
	saver.Add( 2, &nUnitsCell );
	saver.Add( 3, &nCell );
	saver.Add( 6, &cellsIds );
	saver.Add( 7, &cellIdToCoord );

	saver.Add( 8, &planes );
	saver.Add( 9, &sizes );
	saver.Add( 10, &formations );

	int nCheck = -2;
	saver.Add( 11, &nCheck );

	int cnt = 12;
	if ( saver.IsReading() && nCheck != -2 )
	{
		const int nBigCellsSizeX = nUnitsCell.GetSizeX();
		const int nBigCellsSizeY = nUnitsCell.GetSizeY();
		for ( int k = 0; k < 2; ++k )
		{
			for ( int nCellLevel = 0; nCellLevel < N_CELLS_LEVELS; ++nCellLevel )
			{
				for ( int nDipl = 0; nDipl < 3; ++nDipl )
				{
					for ( int nType = 0; nType < 2; ++nType )
					{
						numUnits[k][nCellLevel][nDipl][nType].SetSizes
						(
							nBigCellsSizeX / ( 1 << (nCellLevel+1) ) + 1,
							nBigCellsSizeY / ( 1 << (nCellLevel+1) ) + 1
						);

						numUnits[k][nCellLevel][nDipl][nType].FillZero();
					}
				}
			}
		}
	}
	else
	{
		for ( int nVis = 0; nVis < 2; ++nVis )
		{
			for ( int i = 0; i < N_CELLS_LEVELS; ++i )
			{
				for ( int j = 0; j < 3; ++j )
				{
					for ( int k = 0; k < 2; ++k )
						saver.Add( cnt++, &(numUnits[nVis][i][j][k]) );
				}
			}
		}
	}
	saver.Add( 100, &unitsInCellsSet );
	saver.Add( 101, &nUnitsOfType );
	saver.Add( 102, &posUnitInCell );
	saver.Add( 103, &unitsInCells );
	saver.Add( 104, &nBigCellsSizeX );
	saver.Add( 105, &nBigCellsSizeY );
	saver.Add( 106, &idsRemap );
	
	return 0;
}

int CPlanesIter::operator&( IBinSaver &saver )
{
	

	//saver.Add( 1, &iter );

	return 0;
}

