#include "stdafx.h"
#include "HungarianMethod.h"

bool CHungarianMethod::Init( const CArray2D<float> &_matrix )
{
	NI_ASSERT( _matrix.GetSizeX() == _matrix.GetSizeY(), "CHungarianMethod can work only with sqare matricies" )

	if ( _matrix.GetSizeX() != _matrix.GetSizeY() )
		return false;

	matrix = _matrix;
	nSize = matrix.GetSizeX();
	result.resize( nSize );
	result.assign( nSize, 0 );

	marks.SetSizes( nSize, nSize );
	row.resize( nSize );
	col.resize( nSize );
	markRow.resize( nSize );
	markCol.resize( nSize );

	return true;
}

void CHungarianMethod::Minimize()
{
	for ( int i = 0; i < nSize; ++i )
	{
		float fMin = matrix[0][i];
		for ( int j = 1; j < nSize; ++j ) 
		{
			if ( matrix[j][i] < fMin )
				fMin = matrix[j][i];
		}
		for ( int j = 0; j < nSize; ++j )
			matrix[j][i] -= fMin;
	}

	// шаг 2. Формирование нулей по столбцам
	for ( int j = 0; j < nSize; ++j )
	{
		float fMin = matrix[j][0];
		for ( int i = 1; i < nSize; ++i ) 
		{
			if ( matrix[j][i] < fMin )
				fMin = matrix[j][i];
		}
		for ( int i = 0; i < nSize; ++i )
			matrix[j][i] -= fMin;
	}
}

void CHungarianMethod::MarkZeros()
{
	marks.FillZero();
	row.assign( nSize, 0 );
	col.assign( nSize, 0 );
	for ( int i = 0; i < nSize; ++i )
	{
		for ( int j = 0; j < nSize; ++j )
		{
			if ( matrix[j][i] == 0 )
			{
				if ( row[i] == 0 && col[j] == 0 )
				{
					row[i] = 1;
					col[j] = 1;
					marks[j][i] = 1;
				}
				else
					marks[j][i] = -1;
			}
		}
	}
}

bool CHungarianMethod::FindMatchingInRow( const int nRow )
{
	int nCol = 0;
	while ( marks[nCol][nRow] != 1 )
		nCol++;
	for ( int i = 0; i < nSize; ++i )
	{
		if ( marks[nCol][i] == -1 && markRow[i] == 0 )
		{
			if ( row[i] == 0 )
			{
				row[i] = 1;
				marks[nCol][i] = 1;
				marks[nCol][nRow] = -1;
				return true;
			}
			else
			{
				markRow[i] = 1;
				if ( FindMatchingInRow( i ) )
				{
					marks[nCol][i] = 1;
					marks[nCol][nRow] = -1;
					return true;
				}
			}
		}
	}
	return false;
}

bool CHungarianMethod::SwapMarks()
{
	markRow.assign( nSize, 0 );
	for ( int j = 0; j < nSize; ++j )
	{
		if ( col[j] == 0 )
		{
			for ( int i = 0; i < nSize; ++i )
			{
				if ( marks[j][i] == -1 )
				{
					markRow[i] = 1;
					if ( FindMatchingInRow( i ) )
					{
						col[j] = 1;
						marks[j][i] = 1;
						return true;
					}
					else
						markRow[i] = 1;
				}
			}
		}
	}
	return false;
}

bool CHungarianMethod::IsSolved() const
{
	for ( int i = 0; i < nSize; ++i )
		if ( row[i] == 0 )
			return false;
	return true;
}

void CHungarianMethod::UpdateMatrix()
{
	for ( int i = 0; i < nSize; ++i )
	{
		markRow[i] = 1 - row[i];
		markCol[i] = 0;
	}

	bool bFound = false;
	while ( !bFound )
	{
		bFound = true;
		for ( int i = 0; i < nSize; ++i )
		{
			if ( markRow[i] == 1 )
			{
				for ( int j = 0; j < nSize; ++j )
				{
					if ( marks[j][i] == -1 )
						markCol[j] = 1;
				}
			}
		}
		for ( int j = 0; j < nSize; ++j )
		{
			if ( markCol[j] == 1 )
			{
				for ( int i = 0; i < nSize; ++i )
				{
					if ( marks[j][i] == 1 && markRow[i] == 0 )
					{
						bFound = false;
						markRow[i] = 1;
					}
				}
			}
		}
	}

	float fDelta = FP_MAX_VALUE;
	for ( int i = 0; i < nSize; ++i )
	{
		if ( markRow[i] == 1 )
		{
			for ( int j = 0; j < nSize; ++j )  
			{
				if ( markCol[j] == 0 )
				{
					if ( fDelta > matrix[j][i] )
						fDelta = matrix[j][i];
				}

			}
		}
	}

	for ( int i = 0; i < nSize; ++i )
	{
		if ( markRow[i] == 0 )
		{
			for ( int j = 0; j < nSize; ++j )
				matrix[j][i] += fDelta;
		}
	}
	for ( int j = 0; j < nSize; ++j )
	{
		if ( markCol[j] == 0 )
		{
			for ( int i = 0; i < nSize; ++i )
				matrix[j][i] -= fDelta;
		}
	}
}

void CHungarianMethod::BuildResult()
{
	for ( int i = 0; i < nSize; ++i )
	{
		for( int j = 0; j < nSize; ++j )
		{
			if ( marks[j][i] == 1 )
			{
				result[i] = j;
				break;
			}
		}
	}
}

void CHungarianMethod::Solve()
{
	Minimize();
	while ( true )
	{
		MarkZeros();
		while ( SwapMarks() );
		if ( IsSolved() )
			break;
		UpdateMatrix();
	}
	BuildResult();
}


