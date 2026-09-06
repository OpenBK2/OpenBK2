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


// A dockable panel on the main frame: the minimap, a shortcut bar, the movies
// editor. Created through IMainFrame::CreateControlBar and then driven by the
// domain editor that asked for it.
//
// IsAlive is here because the editors test ::IsWindow on the handle before
// destroying it. A panel outlives the pointer to it only in the sense that the
// pointer stays readable, so this answers whether the thing behind it is still
// there.
struct IDockPanel : public IWidget
{
	virtual ~IDockPanel() {}
	// Show tells the frame, so the docking layout is recalculated.
	// ShowWithoutLayout only shows or hides the panel's own window. The
	// editors do both -- the toggle commands want the first and the creation
	// paths used the second -- and they are not interchangeable.
	virtual void Show( bool bShow ) = 0;
	virtual void ShowWithoutLayout( bool bShow ) = 0;
	virtual bool IsVisible() const = 0;
	virtual bool IsAlive() const = 0;
	virtual void Destroy() = 0;
	virtual void Redraw() = 0;
};


// A toolbar on the main frame, looked up by id. Editors only show, hide and
// query these; the buttons on them are the toolbar manager's business.
struct IToolBar
{
	virtual ~IToolBar() {}
	virtual void Show( bool bShow ) = 0;
	virtual bool IsVisible() const = 0;
};


// A document window. Named IFrameWindow and not IChildFrame because
// Interface_ChildFrame.h already has an IChildFrame and it means something
// different: a registered child-frame *type*, with Create/Enter/Leave. This is
// the window itself.
struct IFrameWindow : public IWidget
{
	virtual ~IFrameWindow() {}
	virtual void Show( bool bShow ) = 0;
	virtual void Maximize() = 0;
	virtual void Focus() = 0;
	virtual void Destroy() = 0;
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


// Which stock face text is drawn in. The editor only ever asked for two, both
// through CFont::CreateStockObject, so this names them rather than carrying a
// font descriptor no caller would fill in differently.
enum EFontKind
{
	FONT_LABEL	= 0,
	FONT_SMALL	= 1,
};


// The 2D overlay the editor's input states draw on top of the 3D viewport.
//
// This is small on purpose and it is not a general 2D API. Sixty-three input
// states take a paint context and four of them draw with it: the rest draw
// through CSceneDrawTool, in the scene, and ignore this entirely. The surface
// here is the union of what those four and NDrawToolsDC actually call, restated
// so that a pen, brush or font is a description rather than an object the caller
// has to create, select and put back.
//
// Pen, brush, font, draw mode, text colour and text background are context
// state. FillRect and FrameRect take their colour directly instead, because
// every caller of those built a one-use brush for the call and threw it away.
struct IPaintContext
{
	virtual ~IPaintContext() {}

	// Save and restore the whole of the state above. The front-end draws on the
	// same surface either side of a state's Draw, so a state that changes the
	// pen has to put it back; this is that, without naming what "it" is.
	virtual void SaveState() = 0;
	virtual void RestoreState() = 0;

	// State.
	virtual void SetPen( EPenStyle ePenStyle, int nWidth, TWidgetColor color ) = 0;
	virtual void SetBrush( TWidgetColor color ) = 0;
	virtual void SetFont( EFontKind eFontKind ) = 0;
	virtual void SetDrawMode( EDrawMode eDrawMode ) = 0;
	virtual void SetTextColor( TWidgetColor color ) = 0;
	virtual TWidgetColor GetTextColor() const = 0;
	// false leaves whatever is behind the glyphs alone, which is what every
	// caller wants over a rendered viewport.
	virtual void SetTextBackgroundOpaque( bool bOpaque ) = 0;

	// Lines, drawn with the current pen.
	virtual void MoveTo( int nX, int nY ) = 0;
	virtual void LineTo( int nX, int nY ) = 0;

	// Rectangles. Rectangle fills with the current brush and outlines with the
	// current pen; the other two ignore both and use the colour given.
	virtual void Rectangle( const CTRect<int> &rRect ) = 0;
	virtual void FillRect( const CTRect<int> &rRect, TWidgetColor color ) = 0;
	virtual void FrameRect( const CTRect<int> &rRect, TWidgetColor color ) = 0;

	// Text, in the current font and text colour.
	// Named DrawString rather than TextOut because <windows.h> defines TextOut
	// as a macro for TextOutA, which would quietly rename this method and every
	// call to it.
	virtual void DrawString( int nX, int nY, const std::string &rszText ) = 0;
	// The box rszText would occupy if drawn at the origin. Used to size a label
	// before drawing its background.
	virtual CTRect<int> MeasureText( const std::string &rszText ) const = 0;
};
