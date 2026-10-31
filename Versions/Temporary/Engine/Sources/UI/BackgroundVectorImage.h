#pragma once

#include "3Dmotor/GfxBuffers.h"
#include "Background.h"
#include "System/Dg.h"

#include "UI_export.h"

#include <memory>
#include <string>

namespace lunasvg
{
class Document;
}

// A background drawing an SVG image, rasterized at the exact pixel size it
// occupies on screen, so it stays sharp at any resolution.
//
// Every other background draws a fixed bitmap laid out in the virtual 1024x768
// screen, which the UI then stretches to the real one by independent X and Y
// factors with bilinear filtering: a 32x22 flag becomes 80x46 pixels at
// 2560x1600, non-integer in both directions, and blurs. This one converts its
// rectangle to screen pixels first, snaps it to whole pixels, and renders the
// image at exactly that size, so the quad maps one texel to one pixel.
//
// The image keeps its own aspect ratio and is centred in the window rather than
// stretched to it, so a flag has its real proportions whatever the window's
// shape and whatever the screen's aspect ratio does to the virtual one. Every
// image is given the same width, the window's laid-out shape at its height on
// screen, and varies in height instead, so flags of different proportions line
// up in a list (see Visit).
//
// Not initialised from a UI record: whoever wants one creates it and installs it
// with CWindow::SetBackground, the way the game room installs CColorBackground.
// It keeps the background it replaced, because some windows are reused for
// different contents (a combo box's selected line is redrawn by the same viewer
// for every item), and one without an image has to get the original back.
class UI_EXPORT CBackgroundVectorImage : public CBackground
{
	OBJECT_BASIC_METHODS( CBackgroundVectorImage );

public:
	// The rasterized image, redrawn whenever the requested size changes.
	class CRaster : public CPtrFuncBase<NGfx::CTexture>
	{
		OBJECT_NOCOPY_METHODS( CRaster );

		std::string szFileName;
		// Parsed once, on first use; not saved, a load parses it again.
		std::shared_ptr<lunasvg::Document> pDocument;
		bool bLoaded = false;
		int nSizeX = 0;
		int nSizeY = 0;
		bool bNeedUpdate = false;

		const lunasvg::Document *GetDocument();

	protected:
		void Recalc() override;

		bool NeedUpdate() override
		{
			return bNeedUpdate;
		}

	public:
		CRaster() {}

		explicit CRaster( const std::string &_szFileName )
			: szFileName( _szFileName )
		{
		}

		// The image's own width / height, or 0 when it cannot be read.
		float GetAspect();
		void SetSize( int nX, int nY );
	};

private:
	std::string szFileName;
	// The background this one replaced; may be 0.
	CObj<IWindowPart> pReplaced;
	// Built on first Visit, and again after a load, so it is not saved.
	CObj<CRaster> pRaster;

public:
	CBackgroundVectorImage() {}

	// szFileName is a path in the game's file system, such as "Flags/us_48.svg".
	CBackgroundVectorImage( const std::string &_szFileName, IWindowPart *_pReplaced )
		: szFileName( _szFileName )
		, pReplaced( _pReplaced )
	{
	}

	const std::string &GetFileName() const
	{
		return szFileName;
	}

	void SetFileName( const std::string &_szFileName );

	IWindowPart *GetReplaced() const
	{
		return pReplaced;
	}

	void Visit( struct IUIVisitor *pVisitor ) override;
	int operator&( IBinSaver &saver ) override;

	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override
	{
		CBackground::InitByDesc( pDesc );
	}

	// A window with an outline type asks every background for one, every frame
	// (CWindow::Visit); the base class asserts. An image has no outline to add.
	void SetOutline( const CDBID &outlineType ) override {}
};
