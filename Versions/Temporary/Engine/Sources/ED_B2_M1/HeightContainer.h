#if !defined(__MAPINFO_HEIGHT_CONTAINER__)
#define __MAPINFO_HEIGHT_CONTAINER__
#pragma once

#include "../MapeditorLib/Tools_FreeIDCollector.h"
#include "VSOManager.h"


class CHeightContainer
{
	static int STACK_SIZE;
	static int TRACE_IMAGE_TILE_SIZE;
	//
	CTPoint<int> planeSize;														// размер однобитной плоскости
	int nStackCount;																	// количество стеков
	float fTileSize;																	// размер сетки
	vector<CArray2D<DWORD> > blackPlaneStackList;			// стеки однобитных плоскостей ( по sizeof(DWORD) на стек ) (черные)
	vector<CArray2D<DWORD> > redPlaneStackList;				// стеки однобитных плоскостей ( по sizeof(DWORD) на стек ) (красные)
	vector<DWORD> markedBitslList;										// набор битов помарканной точки (берется из черной плоскости)
	vector<DWORD> filledBitsList;											// набор битов указывающий заполненность ( если последнее число == 0, то можно сократить количество плоскостей ) (общее для обоих типов плоскостей)
	CFreeIDCollector freePlaneIndexCollector;					// номера свободных плоскостей
	hash_map<int, int> polygonID2PlaneIndexMap;				// PolygonID -> PlaneIndex (0...nCount)
	//
	bool bTraceToImage;																// для тестовых целей
	//
	void AddStack();																											// добавить новый набор плоскостей
	void EraseStack();																										// удалить неиспользуемый набор плоскостей
	int AddPlane( int nPolygonID );																				// добавить плоскость в массив плосковтей
	void ErasePlane( int nPolygonID );																		// удалить плоскость из списка областей
	//
	void ClearPlane( int nPlaneIndex );																		// обнулить плоскость
	void FillPlane( int nPlaneIndex,
									const vector<CVec2> &rBlackPolygon,
									const vector<CVec2> &rRedPolygon );										// закрасить плоскость
	//
	void GetBits( vector<DWORD> *pBitsList, const int x, const int y );
	void AddBitsToString( string *pszMessage, const DWORD dwBits ) const;
	//
	void MarkTraceImageTile( CArray2D<DWORD> *pImage, int x, int y, DWORD dwColor );
	void MarkTraceImageGrid( CArray2D<DWORD> *pImage, DWORD dwColor );

public:
	CHeightContainer( const float _fTileSize );
	//
	void Clear();
	void SetSize( const int x, const int y, bool _bTraceToImage );
	inline bool IsValid() { return ( fTileSize > 0.0f ) && ( nStackCount > 0 ) && ( planeSize.x > 0 ) && ( planeSize.y > 0 ); }
	//
	void Mark( const int x, const int y );																// сохранить точку
	bool Compare( const int x, const int y );															// true - identical, false - not identical
	//
	void InsertPolygon( const vector<CVec2> &rBlackPolygon,
											const vector<CVec2> &rRedPolygon,
											int nPolygonID );																	// добавить или обновить полигон
	void ErasePolygon( int nPolygonID );																	// удалить полигон
	bool GetBlackRedBallance( const CTRect<int> &rRect );
	//
	void InsertVSO( const NDb::SVSOInstance &rVSO );
	//
	void Trace();
};

#endif // !defined(__MAPINFO_HEIGHT_CONTAINER__)

