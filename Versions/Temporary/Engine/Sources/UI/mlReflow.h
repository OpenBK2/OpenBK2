#pragma once

namespace NML
{

class IReflowObject;
class IVisReflowObject;

enum EHAlign
{
	EHA_LEFT,
	EHA_RIGHT,
	EHA_CENTER,
	EHA_NOWRAP,
	EHA_JUSTIFY,
	EHA_WRAP_LEFT,
	EHA_WRAP_RIGHT
};

enum EVAlign
{
	EVA_TOP,
	EVA_BOTTOM,
	EVA_MIDDLE
};

const int
	FONT_SIZE_MASK			= 0x00FFFFFF,
	FONT_SIZE_PIXELS		= 0x10000000,
	FONT_SIZE_POINTS		= 0x20000000;

// SState

struct SState
{
	ZDATA
	//// reflow
	EHAlign nHAlign;
	EVAlign nVAlign;
	//// font
	int nMinFontSize;
	int nOutlineBorder;
	bool bForceFontSize;
	NGScene::SFont font;
	NGfx::SPixel8888 color;
	NGfx::SPixel8888 outlineColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nHAlign); f.Add(3,&nVAlign); f.Add(4,&nMinFontSize); f.Add(5,&nOutlineBorder); f.Add(6,&bForceFontSize); f.Add(7,&font); f.Add(8,&color); f.Add(9,&outlineColor); return 0; }

	SState(): nHAlign(EHA_LEFT), nVAlign(EVA_MIDDLE), nMinFontSize(0), nOutlineBorder(0), bForceFontSize(false), font(16|FONT_SIZE_POINTS, "System"), color(0xFF, 0xFF, 0xFF, 0xFF), outlineColor(0xFF, 0xFF, 0xFF, 0xFF) {}
};

// IReflowState

class IReflowState
{
public:
	virtual const SState& GetState() const = 0;
	virtual void SetState( const SState &state ) = 0;

	virtual void AddObject( IVisReflowObject *pObject ) = 0;
	virtual void CreateLine( bool bEndBlock ) = 0;

	virtual NGScene::ILayoutFakeView* GetScene() const = 0;
};

// IReflowObject

class IReflowObject: public CObjectBase
{
public:
	virtual void Update( IReflowState *pState ) {}

	virtual bool IsSpace() const { return false; }
};

// IVisReflowObject

class IVisReflowObject: public IReflowObject
{
public:
	virtual const CTPoint<float>& GetPosition() const = 0;
	virtual void SetPosition( const CTPoint<float> &sPosition ) = 0;

	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window ) = 0;
	virtual void Render( list<CTRect<float> > *pRender, const CTPoint<float> &globalPosition, const CTRect<float> &window ) = 0;

	virtual const CTPoint<float>& GetSize() const = 0;
};

// IReflowLayout

class IReflowLayout: public CObjectBase
{
public:
	virtual void AddObject( IReflowObject *pObject ) = 0;

	virtual void Generate( NGScene::ILayoutFakeView *pView, float fWidth ) = 0;

	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window ) = 0;
	virtual void Render( list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window ) = 0;

	virtual const CTPoint<float>& GetSize() const = 0;
};

IReflowLayout* CreateReflowLayout();

} // NAMESPACE

