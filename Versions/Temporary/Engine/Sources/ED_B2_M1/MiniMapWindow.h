#pragma once

#include <afxwin.h>
#include "B2_M1_Terrain/DBTerrain.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

/*
жестко заданный minimap, в виде ромба. вершины ромба лежат РОВНО на серединах сторон окна прямоугольника, 
являющегося minimap'ом:

     x = 0, y = maxY  
          /\
         /  \
x = 0		/    \  x = maxX
y = 0   \    /  y = maxY
         \  /
          \/
     x = maxX, y = 0

для не квадратных окон это выглядит следующим образом:

          /\
         /  \
				/    \
			 /     /
			/     /
      \    /
			 \  /
			  \/
*/

class CMiniMapWindow : public CWnd, public ICommandHandler
{
	CFont fontMiniMap;
	
	bool bMapLoaded;

	CSize	mapInfoEditorSize;

	CDC mapDC;
	CBitmap mapBitmap;
	CSize	mapSize;
	CSize mapAISize;

	CPen	rectWhitePen;
	CPen	rectBlackPen;

protected:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnMouseMove( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonUp( unsigned nFlags, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

	void RecreateImage();

	bool EditorToMiniMap( CVec2 *pvResult, const CVec2 &vEditorPos );
	bool MiniMapToEditor( CVec2 *pvResult, const CVec2 &vMiniMaprPos );

public:
	bool Create( CWnd *parentWindow );
	void Destroy();

	void LoadMap( const NDb::STerrain *pTerrainDesc );
	void SetMapInfoEditorSize( const int nSizeX, const int nSizeY );

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	
	DECLARE_MESSAGE_MAP()
};

