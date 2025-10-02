#pragma once

#include "../3DMotor/G2DView.h"
#include "../3DMotor/RectLayout.h"
#include "../UI/UIVisitor.h"
#include "../UI/UIML.h"

interface IUIVisitorCmd : public CObjectBase
{
	virtual void Visit( IUIVisitor *pVisitor ) = 0;
};

class CClipSetCmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CClipSetCmd )
	//
	CTRect<float> rClip;
public:
	CClipSetCmd() {}
	CClipSetCmd( const CTRect<float> &_rClip ): rClip( _rClip ) {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->ClipSet( rClip );
	}
};

class CClipRestoreCmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CClipRestoreCmd )
public:
	CClipRestoreCmd() {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->ClipRestore();
	}
};

class CVisitZClearRectCmd : public IUIVisitorCmd
{ 
	OBJECT_NOCOPY_METHODS( CVisitZClearRectCmd )
	//
	CRectLayout sLayout;
	CTPoint<float> sPosition;
	CTRect<float> sClipWindow;
	float fZ;
public:
	CVisitZClearRectCmd() {}
	CVisitZClearRectCmd( const CRectLayout &_sLayout, const CTPoint<float> &_sPosition, const CTRect<float> &_sClipWindow, float _fZ )
		: sLayout( _sLayout ), sPosition( _sPosition ), sClipWindow( _sClipWindow ), fZ( _fZ ) {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->VisitZClearRect( sLayout, sPosition, sClipWindow, fZ );
	}
};

class CVisitUIRectCmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CVisitUIRectCmd )
	//
	CDBPtr<NDb::STexture> pTexture;
	int nShadingEffect;
	CRectLayout rects;
public:
	CVisitUIRectCmd() {}
	CVisitUIRectCmd( const NDb::STexture *_pTexture, const int _nShadingEffect, const class CRectLayout &_rects )
		: pTexture( _pTexture ), nShadingEffect( _nShadingEffect ), rects( _rects ) {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->VisitUIRect( pTexture, nShadingEffect, rects );
	}
};

class CVisitUIRect2Cmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CVisitUIRect2Cmd )
	//
	CDBPtr<NDb::STexture> pTexture;
	int nShadingEffect;
	CVec2 pPos4[4];
	NGfx::SPixel8888 pColors4[4];
	CTRect<float> rectTexture;
public:
	CVisitUIRect2Cmd() {}
	CVisitUIRect2Cmd( const NDb::STexture *_pTexture, const int _nShadingEffect, 
		const CVec2 *_pPos4, const NGfx::SPixel8888 *_pColors4, const CTRect<float> &_rectTexture )
		: pTexture( _pTexture ), nShadingEffect( _nShadingEffect ), rectTexture( _rectTexture ) 
	{
		memcpy( pPos4, _pPos4, 4 * sizeof(pPos4[0]) );
		memcpy( pColors4, _pColors4, 4 * sizeof(pColors4[0]) );
	}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->VisitUIRect( pTexture, nShadingEffect, pPos4, pColors4, rectTexture );
	}
};

class CVisitUITextureRectCmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CVisitUITextureRectCmd )
	//
	CPtr< CPtrFuncBase<NGfx::CTexture> > pTexture;
	int nShadingEffect;
	CRectLayout rects;
public:
	CVisitUITextureRectCmd() {}
	CVisitUITextureRectCmd( CPtrFuncBase<NGfx::CTexture> *_pTexture, const int _nShadingEffect, const class CRectLayout &_rects )
		: pTexture( _pTexture ), nShadingEffect( _nShadingEffect ), rects( _rects ) {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->VisitUITextureRect( pTexture, nShadingEffect, rects );
	}
};

class CVisitUITextCmd : public IUIVisitorCmd
{
	OBJECT_NOCOPY_METHODS( CVisitUITextCmd )
	//
	CPtr<IML> pML;
	CTPoint<float> sPosition;
	CTRect<float> sWindow;
public:
	CVisitUITextCmd() {}
	CVisitUITextCmd( IML *_pML, const CTPoint<float> &_sPosition, const CTRect<float> &_sWindow )
		: pML( _pML ), sPosition( _sPosition ), sWindow( _sWindow ) {}
	//
	void Visit( IUIVisitor *pVisitor )
	{
		pVisitor->VisitUIText( pML, sPosition, sWindow );
	}
};

// ************************************************************************************************************************ //
// **
// ** UI visitor itself
// **
// **
// **
// ************************************************************************************************************************ //

class CUIVisitor : public IUIVisitor
{
	CObj<NGScene::I2DGameView> pGFX;
	//
	CTRect<float> rcClip;
	list< CTRect<float> > rcStoredClip;
	bool bClipApplied;
	bool bLowerLevel;
	//
	typedef list< CPtr<IUIVisitorCmd> > CCommandsList;
	CCommandsList upperLevelCommands;
	//
	virtual void ClipRestore()
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CClipRestoreCmd() );
			return;
		}
		NI_ASSERT( !rcStoredClip.empty(), "restore empty clip" );
		rcClip = rcStoredClip.back();
		rcStoredClip.pop_back();
		bClipApplied = !rcStoredClip.empty();
	}
	//
	virtual void ClipSet( const CTRect<float> &_rClip )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CClipSetCmd(_rClip) );
			return;
		}
		rcStoredClip.push_back( rcClip );
		rcClip.Intersect(_rClip);
		bClipApplied = true;
	}
public:
	CUIVisitor() : rcClip(0, 0, 1024, 768 ), bClipApplied( false ), bLowerLevel( true ) { }
	CUIVisitor( NGScene::I2DGameView *p2D ): bLowerLevel( true )
	{ 
		SetGView( p2D );
	}
	void ClearCommandsList() { upperLevelCommands.clear(); }
	void SetLowerLevel() { bLowerLevel = true; }
	void SetUpperLevel() { bLowerLevel = false; }
	void FlushUpperLevelData()
	{
		bool bStoredLowerLevel = bLowerLevel;
		bLowerLevel = true; 
		//
		for ( CCommandsList::iterator it = upperLevelCommands.begin(); it != upperLevelCommands.end(); ++it )
			(*it)->Visit( this );
		upperLevelCommands.clear();
		//
		bLowerLevel = bStoredLowerLevel;
	}
	//
	void SetGView( NGScene::I2DGameView *_p2D ) 
	{
		const CVec2 vScreenSize( _p2D->GetViewportSize() );
		pGFX = _p2D;
		rcClip.x1 = 0;
		rcClip.x2 = vScreenSize.x;
		rcClip.y1 = 0;
		rcClip.y2 = vScreenSize.y;
		bClipApplied = false;
	}
	//
	void VisitZClearRect( const CRectLayout &_sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow, float fZ=1.0f )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CVisitZClearRectCmd( _sLayout, sPosition, sClipWindow, fZ ) );
			return;
		}
		//
		if ( rcClip.IsEmpty() )
			return;

		CRectLayout sLayout = _sLayout;
		sLayout.fZ = fZ;
		pGFX->CreateDynamicClearRects( sLayout, sPosition, sClipWindow );
	}
	//
	void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, const class CRectLayout &rects )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CVisitUIRectCmd( pTexture, nShadingEffect, rects ) );
			return;
		}
		// recalc to new size
		if ( rcClip.IsEmpty() )
			return;
		pGFX->CreateDynamicRects( pTexture, rects, CTPoint<float>( 0,0 ), rcClip );
	}
	//
	void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CVisitUIRect2Cmd( pTexture, nShadingEffect, pPos4, pColors4, rectTexture ) );
			return;
		}
		// recalc to new size
		if ( rcClip.IsEmpty() )
			return;
		pGFX->CreateDynamicRects( pTexture, pPos4, pColors4, rectTexture );
	}
	//
	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *pTexture, const int nShadingEffect, const class CRectLayout &rects )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CVisitUITextureRectCmd( pTexture, nShadingEffect, rects ) );
			return;
		}
		if ( rcClip.IsEmpty() )
			return;
		pGFX->CreateDynamicRects( pTexture, rects, CTPoint<float>( 0,0 ), rcClip );
	}
	//
	void VisitUIText( IML *pML, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
	{
		if ( !bLowerLevel )
		{
			upperLevelCommands.push_back( new CVisitUITextCmd( pML, sPosition, sWindow ) );
			return;
		}
		//
		if ( bClipApplied )
		{
			CTRect<float> clipRect = sWindow;
			clipRect.Intersect( rcClip );
			if ( clipRect.IsEmpty() )
				return;
			pML->Render( pGFX, sPosition, clipRect );
		}
		else
		{
			pML->Render( pGFX, sPosition, sWindow );
		}
	}
};

