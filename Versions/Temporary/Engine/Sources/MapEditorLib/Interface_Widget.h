#pragma once

#include "Misc/Geom.h"

#include <cstdint>
#include <string>

// Toolkit-neutral primitives for the editor's MVC interface layer.
//
// The Interface_*.h files are Nival's own MVC decomposition and are already
// almost free of toolkit types: they speak std::string, CTPoint<int>, CDBID and
// the editor's own structs. What leaked through were a handful of MFC handles
// used as opaque tokens, and those tokens are what tie the whole editor to one
// toolkit. These stand in for them, so that a front-end is something that
// implements these interfaces rather than something the interfaces name.
//
// IWidget, IImage and IImageList are pure tokens. Nothing above the front-end
// may look inside one: they exist so a pointer can be carried from one part of
// a front-end to another part of the same front-end through interface code that
// has no business knowing what it is. IPaintContext is the exception, because
// the editor genuinely draws through it.


// A widget belonging to whichever front-end is running.
//
// Every use of this in the interface layer is a conduit: the editor hands a
// front-end's widget back to that same front-end as a dialog parent, as the
// contents of a docking pane, or as the window a control should send its
// messages to. The interface layer only carries it.
//
// GetNativeWidget is a deliberate handshake rather than a bare tag plus a
// static_cast at the far end. A static_cast would still compile, and silently
// mean nothing, the day a second front-end passes its own IWidget through the
// same call. Going through one virtual keeps the conversion in one place per
// front-end, which is where a wrong one can be caught.
struct IWidget
{
	virtual ~IWidget() {}
	// The front-end's own widget object. Only the front-end that created this
	// widget may interpret the result.
	virtual void* GetNativeWidget() = 0;
};


// A single image belonging to the front-end. Used by the object collector to
// hand back the icons it renders for database objects.
struct IImage
{
	virtual ~IImage() {}
	virtual void* GetNativeImage() = 0;
};


// An indexed collection of same-sized images, which list and tree widgets are
// given so their items can carry icons.
struct IImageList
{
	virtual ~IImageList() {}
	virtual void* GetNativeImageList() = 0;
};


// Packed 0x00BBGGRR, which is what the editor's colour constants already are.
typedef uint32_t TWidgetColor;


enum EPenStyle
{
	PEN_SOLID	= 0,
	PEN_DOT		= 1,
	PEN_DASH	= 2,
};


// How drawing combines with what is already on the surface. The editor uses
// only DRAW_COPY and DRAW_NOT, the latter for rubber-band selection that has to
// be undrawn by drawing it again.
enum EDrawMode
{
	DRAW_COPY	= 0,
	DRAW_NOT	= 1,
	DRAW_XOR	= 2,
};


// The 2D overlay the editor's input states draw on top of the 3D viewport.
//
// This is small on purpose and it is not a general 2D API. Thirty-three input
// states take a paint context and exactly three of them use it: the rest draw
// through CSceneDrawTool, in the scene, and ignore this entirely. So the whole
// surface is the ten operations those three files call, restated so that a pen
// is a description rather than an object the caller has to create, select and
// put back.
struct IPaintContext
{
	virtual ~IPaintContext() {}
	// Line drawing. SetPen replaces the create/SelectObject/restore dance; the
	// context owns whatever it needs to make one.
	virtual void SetPen( EPenStyle ePenStyle, int nWidth, TWidgetColor color ) = 0;
	virtual void SetDrawMode( EDrawMode eDrawMode ) = 0;
	virtual void MoveTo( int nX, int nY ) = 0;
	virtual void LineTo( int nX, int nY ) = 0;
	// Rectangles. FillRect paints the interior, FrameRect outlines it.
	virtual void FillRect( const CTRect<int> &rRect, TWidgetColor color ) = 0;
	virtual void FrameRect( const CTRect<int> &rRect, TWidgetColor color ) = 0;
	// Text.
	virtual void SetTextColor( TWidgetColor color ) = 0;
	virtual TWidgetColor GetTextColor() const = 0;
	// false leaves whatever is behind the glyphs alone, which is what every
	// caller wants over a rendered viewport.
	virtual void SetTextBackgroundOpaque( bool bOpaque ) = 0;
	virtual void TextOut( int nX, int nY, const std::string &rszText ) = 0;
};
