#include "StdAfx.h"
#include "GFont.h"
#include "GLocale.h"
#include "Locale.h"
#include "GTexture.h"
#include "../System/BasicShare.h"
#include "DBScene.h"
#include "2DScene.h"
#include "G2DView.h"

namespace NGScene
{

// share objects
CBasicShare<SIntResKey, CFileFont> shareFonts(106);
extern CBasicShare<STextureKey, CFileTexture, STextureKeyHash> shareTextures;

class C2DGameView: public I2DGameView
{
	OBJECT_BASIC_METHODS(C2DGameView);
private:
	ZDATA
	CObj<I2DScene> pScene;
	CObj<CTextLocaleInfo> pLocale;
	CVec2 vWindowSize;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pScene); f.Add(3,&pLocale); f.Add( 4, &vWindowSize ); return 0; }

public:
	C2DGameView();

	void CreateDynamicRects( const NDb::STexture *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );
	void CreateDynamicRects( const NDb::STexture *pTexture, const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, 
		const CTRect<float> &rectTexture );
	void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow );
	void CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow );
	CObjectBase* CreateTexture( const NGScene::STextureKey &key );

	CVec2 GetViewportSize() { return NGfx::GetScreenRect(); }
	CTextLocaleInfo* GetLocaleInfo() const { return pLocale; }

	void StartNewFrame( NGfx::CTexture *pTarget );
	void StartNewFrame( EAlphaMode2D _AlphaMode2D );
	void Flush();
	void SetWindowSize( const CVec2 &_vSize );
};

// C2DGameView

C2DGameView::C2DGameView()
{
	pScene = Make2DScene();
	///
	pLocale = GetTextLocaleInfo();//new CTextLocaleInfo;
	pLocale->Setup( NGfx::GetScreenRect() );
}

CObjectBase* C2DGameView::CreateTexture( const NGScene::STextureKey &key )
{
	return shareTextures.Get( key );
}

void C2DGameView::CreateDynamicRects( const NDb::STexture *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	if ( !pTexture )//CObjectBase *pRes = pScene->Create
		pScene->CreateDynamicRects( 0, sLayout, sPosition, sWindow );
	else
		pScene->CreateDynamicRects( shareTextures.Get( STextureKey( pTexture ) ), sLayout, sPosition, sWindow );
}

void C2DGameView::CreateDynamicRects( const NDb::STexture *pTexture, 
	const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture )
{
	if ( !pTexture )//CObjectBase *pRes = pScene->Create
		pScene->CreateDynamicRects( 0, pPos4, pColors4, rectTexture );
	else
		pScene->CreateDynamicRects( shareTextures.Get( STextureKey( pTexture ) ), pPos4, pColors4, rectTexture );
}

void C2DGameView::CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	pScene->CreateDynamicRects( pTexture, sLayout, sPosition, sWindow );
}

void C2DGameView::CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow )
{
	pScene->CreateDynamicClearRects( sLayout, sPosition, sClipWindow );
}

void C2DGameView::SetWindowSize( const CVec2 &_vSize )
{
	vWindowSize = _vSize;
}

void C2DGameView::StartNewFrame( NGfx::CTexture *pTarget )
{
	pLocale->Setup( NGfx::GetScreenRect() );
	pScene->StartNewFrame( pTarget, NGfx::GetScreenRect(), AM2D_NORMAL );
}

void C2DGameView::StartNewFrame( EAlphaMode2D _AlphaMode2D )
{
	pLocale->Setup( NGfx::GetScreenRect() );
	pScene->StartNewFrame( 0, NGfx::GetScreenRect(), _AlphaMode2D );
}

void C2DGameView::Flush()
{
	pScene->Flush();
}

// Create 2D View

I2DGameView* CreateNew2DView()
{
	return new C2DGameView;
}

}
using namespace NGScene;
BASIC_REGISTER_CLASS( I2DGameView );
REGISTER_SAVELOAD_CLASS( 0xF1741142, C2DGameView );

