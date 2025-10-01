#pragma once

#include "Common_RTS_AI_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Венгерский метод решения задачи о назначениях. Используется при поиске путей, когда группу юнитов необходимо послать
//в конечную точку, так что бы путь каждого юнита был минимален. При инициализации передается матрица расстояний между
//всеми юнитами и всеми точками, то есть matrix[point][unit] = fabs( unit_pos - point_pos ).
//  Последовательность действий: Init( matrix ), Solve() затем GetResult( unit ) возвращает номер точки в которой стоит
//послать этот юнит.
class COMMON_RTS_AI_EXPORT CHungarianMethod
{
	int nSize;
	CArray2D<float> matrix;
	vector<int> result;

	CArray2D<int> marks;
	vector<int> row, col, markRow, markCol;

	void Minimize();
	void MarkZeros();
	bool FindMatchingInRow( const int nRow );
	bool SwapMarks();
	bool IsSolved() const;
	void UpdateMatrix();
	void BuildResult();

public:
	CHungarianMethod() : nSize( 0 ) {}
	bool Init( const CArray2D<float> &matrix );
	void Solve();
	const int GetSize() const { return nSize; }
	const int GetResult( const int nIndex ) const { return result[nIndex]; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
