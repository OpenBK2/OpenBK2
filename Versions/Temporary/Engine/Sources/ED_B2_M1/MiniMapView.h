#pragma once

#include "B2_M1_Terrain/DBTerrain.h"
#include "MapEditorLib/Interface_Widget.h"

#include <cstdint>
#include <vector>

// The map editor's minimap, behind a boundary that names no toolkit.
//
// A docking pane holding the map as a diamond -- the terrain's tile colours
// turned 45 degrees -- with the outline of what the camera sees drawn over
// it. A click or a drag on it moves the camera there; its context menu makes a
// new minimap image for the map. CMapInfoEditor::CreateControls makes the pane
// and puts this inside it.
//
// As with the movies editor, the pane is the frame's and what is in it is
// whichever toolkit the session runs. The picture and the arithmetic between
// the minimap and the view are not drawing, and exist once, below, for both.
namespace NMiniMapView
{
	class IView
	{
	public:
		virtual ~IView() {}

		// Builds the window inside pPane, and registers it as the
		// CHID_MAPINFO_MINIMAP_WINDOW command handler, which its context menu's
		// command reaches it by.
		virtual bool Create( IWidget *pPane ) = 0;
		// Unregisters and destroys the window. The pane is not touched.
		virtual void Destroy() = 0;
		virtual void Show( bool bShow ) = 0;

		// What the pane is given as its contents. Null before Create.
		virtual IWidget* GetWidget() = 0;

		// Makes the picture from the terrain the scene holds; null forgets it.
		// Does not repaint, as CMiniMapWindow::LoadMap did not.
		virtual void LoadMap( const NDb::STerrain *pTerrainDesc ) = 0;
		// The size of the editor's view, whose corners the outline is drawn
		// through. Repaints.
		virtual void SetMapInfoEditorSize( int nSizeX, int nSizeY ) = 0;
		// Paints everything now: what RedrawWindow did.
		virtual void Redraw() = 0;
		// Paints what is waiting to be painted: what UpdateWindow did.
		virtual void Update() = 0;
	};


	// Which implementation the pane gets: wx, unless OBK2_WX_DIALOGS=0. The caller owns
	// the result and destroys it with delete.
	IView* Create();

	// Named so the dispatcher can reach them; not for anything else to call.
	IView* CreateMfc();
#ifdef OBK2_WITH_WX
	IView* CreateWx();
#endif


	// The minimap's picture: a square nSide pixels across, rows top down, each
	// pixel 0x00RRGGBB -- what a 32-bit DIB holds.
	struct SImage
	{
		// The map's size in visual tiles.
		int nMapSizeX = 0;
		int nMapSizeY = 0;
		int nSide = 0;
		std::vector<uint32_t> pixels;

		bool IsEmpty() const
		{
			return nSide <= 0;
		}
	};

	// The picture CMiniMapWindow::LoadMap drew from the scene's terrain. False,
	// with the image left empty, when there is no terrain to draw.
	bool BuildImage( const NDb::STerrain *pTerrainDesc, SImage *pImage );

	// Where a point of the editor's view falls on a minimap nWidth by nHeight
	// pixels, through the camera's ray to the ground.
	CVec2 EditorToMiniMap( const SImage &rImage, int nWidth, int nHeight, const CVec2 &vEditorPos );
	// The ground position under a point of the minimap, clamped to the map.
	CVec2 MiniMapToEditor( const SImage &rImage, int nWidth, int nHeight, const CVec2 &vMiniMapPos );
}
