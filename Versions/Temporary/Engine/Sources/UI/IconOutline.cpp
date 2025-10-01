#include "StdAfx.h"
#include "..\3dmotor\g2dview.h"
#include "IconOutline.h"
#include "..\3DMotor\2DSceneSW.h"
#include "..\3DMotor\SWTexture.h"
#include "..\3dmotor\DBScene.h"

#include "UIML.h"
#include "..\3dmotor\GTexture.h"
#include "..\System\BasicShare.h"
#include "..\3DMotor\GRects.h"
//////////////////////////////////////////////////////////////////////////
namespace NGScene
{
typedef CBasicShare<STextureKey, CFileTexture, STextureKeyHash> CTexShare;
EXTERNVAR _3DMOTOR_EXPORT CTexShare shareTextures;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SRLBuilder : public NGScene::ILayoutFakeView
{
	NGScene::ISW2DScene *pScene;

	void CreateDynamicRects( const NDb::STexture *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
	{
		CRectLayout rl;
		for ( int nTemp = 0; nTemp < sLayout.rects.size(); nTemp++ )
		{
			const CRectLayout::SRect &sRect = sLayout.rects[nTemp];
			CTRect<float> sClippedRect;
			CRectLayout::STextureCoord sTex;

			if ( !NGScene::ClipRect( &sClippedRect, &sTex, sRect, sPosition, sWindow ) )
				continue;

			CRectLayout::SRect r;
			int d = ( nTemp == sLayout.rects.size()-1 )?1:0;
			r.fX = sClippedRect.x1 * CIconOutliner::GetTextStretchX() + d; r.fSizeX = sClippedRect.Width();
			r.fY = sClippedRect.y1; r.fSizeY = sClippedRect.Height();
			r.sTex = sTex;
			r.sColor = sRect.sColor;
			rl.AddRect( r );
		}
		pScene->CreateRects( NGScene::GetSWTex( pTexture ), rl );
	}
	void CreateDynamicRects( const NDb::STexture *pTexture, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture ) 
	{
		NI_ASSERT( 0, "not implemented" ); 
	}
	void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *_pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
	{
		const NGScene::CTexShare::CDataHash &allTex = NGScene::shareTextures.GetAll();
		for ( NGScene::CTexShare::CDataHash::const_iterator i = allTex.begin(); i != allTex.end(); ++i )
		{
			if ( i->second == _pTexture )
			{
				CreateDynamicRects( i->first.pTexture, sLayout, sPosition, sWindow );
				return;
			}
		}
		ASSERT(0);
	}
	SRLBuilder( NGScene::ISW2DScene *_pScene ) : pScene(_pScene) {}
};
//////////////////////////////////////////////////////////////////////////
//vector<int> CIconOutliner::OutlineIDs;
//////////////////////////////////////////////////////////////////////////
CIconOutliner::CIconOutliner( CDBID _nOutlineType, NGfx::ETextureUsage _eUsage, const CTPoint<int> &_size )
: bNeedUpdate(true), fixedSize(_size), eUsage( _eUsage ), nOutlineType(_nOutlineType), size(1,1)
{
	//ASSERT( OutlineIDs.size() ); //call before SetOutlineIDs 
}
//////////////////////////////////////////////////////////////////////////
/*void CIconOutliner::SetOutlineIDs( const vector<int> &_OutlineIDs ) 
{
	if ( OutlineIDs.size() )
		return;

	for ( int i=0; i<_OutlineIDs.size(); ++i )
	{
		OutlineIDs.push_back( _OutlineIDs[i] );
	}
}*/
//////////////////////////////////////////////////////////////////////////
bool CIconOutliner::NeedUpdate() 
{ 
	bool bRes = bNeedUpdate; 
	bNeedUpdate = false; 
	return bRes; 
}
//////////////////////////////////////////////////////////////////////////
CVec2 CIconOutliner::GetOutlineAdd( const CDBID &nOutlineType )
{
	const NDb::STexture *pMask = NDb::Get<NDb::STexture>( nOutlineType );
	return pMask? CVec2( pMask->nWidth, pMask->nWidth ):CVec2();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CIconOutliner::Recalc()
{	
	NGfx::EPixelFormat pf = NGfx::Is16BitTextures() ? NGfx::CF_A4R4G4B4 : NGfx::CF_A8R8G8B8;
	pValue = NGfx::MakeTexture( size.x, size.y, 1, pf, eUsage, NGfx::CLAMP );
	if ( !IsValid(pValue) )
	{
		csSystem << "error texture in CIconOutliner::Recalc, size:" << size.x << ", " << size.y << "\n";
		return;
	}

	CPtr<NGScene::ISW2DScene> p2DScene = NGScene::Make2DSWScene();
	for ( int k = 0; k < fmtProcessed.cmds.size(); ++k )
	{
		const SFormattingInfo::SCmd &cmd = fmtProcessed.cmds[k];
		switch ( cmd.type )
		{
		case SFormattingInfo::ELEM_OUTLINE:
			{
				const NDb::STexture *pMask = NDb::Get<NDb::STexture>( cmd.nOutlineType );
				if ( pMask )
					p2DScene->AddPostFilter( NGScene::GetSWTex( pMask ), pMask->nWidth/ 2, pMask->nHeight/ 2 );
			}
			break;
		case SFormattingInfo::ELEM_PICTURE:
			{
				CRectLayout rl = MakeLayout( cmd.vPos.x, cmd.vPos.y, cmd.vSize.x, cmd.vSize.y, 0, 0, cmd.pTexture->nWidth, cmd.pTexture->nHeight, cmd.dwColor );
				p2DScene->CreateRects( NGScene::GetSWTex( cmd.pTexture.GetPtr() ), rl );
			}
			break;
		case SFormattingInfo::ELEM_TEXT:
			{
				SRLBuilder rb( p2DScene );
				gfxTexts[k]->Render( &rb, CTPoint<float>( cmd.vPos.x, cmd.vPos.y ),  CTRect<float>( 0, 0, size.x, size.y ) );
			}
			break;
		case SFormattingInfo::ELEM_GRAY_FILTER:
			p2DScene->AddGrayingFilter( NGfx::GetCVec4Color( cmd.dwColor ) );
			break;
		default:
			ASSERT(0);
			break;
		}
	}
	p2DScene->Draw( pValue, size, true );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CIconOutliner::SetFormat( const SFormattingInfo &_fmt )
{
	if ( fmt == _fmt )
		return;
	fmt = _fmt;
	gfxTexts.resize( fmt.cmds.size() );

	fmtProcessed = fmt;

	if ( !nOutlineType.IsEmpty() )
		fmtProcessed.AddOutline( nOutlineType );

	bool bIsFixedSize = fixedSize.x != 0 && fixedSize.y != 0;
	int nSizeXAdd = (eUsage==NGfx::TEXTURE_2D)?GetSizeAdd().x:0; 
	int nSizeYAdd = (eUsage==NGfx::TEXTURE_2D)?GetSizeAdd().y:0;
	CTPoint<int> resSize = fixedSize;
	for ( int k = 0; k < fmtProcessed.cmds.size(); ++k )
	{
		SFormattingInfo::SCmd &cmd = fmtProcessed.cmds[k];
		if ( cmd.nChainX >= 0 )
		{
			SFormattingInfo::SCmd &ch = fmtProcessed.cmds[ cmd.nChainX ];
			cmd.vPos.x = ch.vPos.x + ch.vSize.x;
		}
		switch ( cmd.type )
		{
		case SFormattingInfo::ELEM_OUTLINE:
			{
				const NDb::STexture *pMask = NDb::Get<NDb::STexture>( cmd.nOutlineType );
				// enlarge size estimate
				if ( pMask && eUsage==NGfx::TEXTURE_2D )
				{
					nSizeXAdd = Max( nSizeXAdd, pMask->nWidth );
					nSizeYAdd = Max( nSizeYAdd, pMask->nHeight );
				}
				//else
				//	ASSERT( 0 );
			}
			break;
		case SFormattingInfo::ELEM_PICTURE:
			{
				if ( cmd.vSize.x == 0 && cmd.vSize.y == 0 )
				{
					cmd.vSize.x = cmd.pTexture->nWidth;
					cmd.vSize.y = cmd.pTexture->nHeight;
				}
				cmd.vPos.x += nSizeXAdd / 2;
				cmd.vPos.y += nSizeYAdd / 2;
				int nMinX = Min( cmd.vPos.x, cmd.vPos.x + cmd.vSize.x );
				int nMaxX = Max( cmd.vPos.x, cmd.vPos.x + cmd.vSize.x );
				int nMinY = Min( cmd.vPos.y, cmd.vPos.y + cmd.vSize.y );
				int nMaxY = Max( cmd.vPos.y, cmd.vPos.y + cmd.vSize.y );
				ASSERT( nMinY >= 0 && nMinY >= 0 );
				if ( !bIsFixedSize )
				{
					resSize.x = Max( resSize.x, nMaxX );
					resSize.y = Max( resSize.y, nMaxY );
				}
				else
				{
					ASSERT( nMaxX <= resSize.x );
					ASSERT( nMaxY <= resSize.y );
				}
			}
			break;
		case SFormattingInfo::ELEM_TEXT:
			{
				IML *pML = CreateML();
				gfxTexts[k] = pML;
				pML->SetText( cmd.szText, 0 );
				if ( bIsFixedSize )
				{
					pML->Generate( resSize.x - cmd.vPos.x );
					CTPoint<int> sz = pML->GetSize();
					cmd.vSize.x = sz.x;
					cmd.vSize.y = sz.y; 
					ASSERT( sz.x <= resSize.x - cmd.vPos.x );
					ASSERT( sz.y <= resSize.y - cmd.vPos.y );
				}
				else
				{
					pML->Generate( 512 - 64 );
					CTPoint<int> sz = pML->GetSize();
					cmd.vSize.x = sz.x;
					cmd.vSize.y = sz.y;
					resSize.x = Max( resSize.x, Float2Int( cmd.vPos.x + cmd.vSize.x ) );
					resSize.y = Max( resSize.y, Float2Int( cmd.vPos.y + cmd.vSize.y ) );
				}
				resSize.x *= CIconOutliner::GetTextStretchX();
				cmd.vSize.x *= CIconOutliner::GetTextStretchX();
			}
			break;
		case SFormattingInfo::ELEM_GRAY_FILTER:
			break;
		default:
			ASSERT(0);
			break;
		}
	}
	// process centered flags
	for ( int k = 0; k < fmtProcessed.cmds.size(); ++k )
	{
		SFormattingInfo::SCmd &cmd = fmtProcessed.cmds[k];
		if ( cmd.nFlags == 0 )
			continue;
		if ( cmd.nFlags & SFormattingInfo::CENTER_X )
			cmd.vPos.x = resSize.x / 2 - cmd.vSize.x / 2;
		if ( cmd.nFlags & SFormattingInfo::CENTER_Y )
			cmd.vPos.y = resSize.y / 2 - cmd.vSize.y / 2;
	}

	// account outlining in size
	//if ( !bIsFixedSize )
	{
		for ( int k = 0; k < fmtProcessed.cmds.size(); ++k )
		{
			SFormattingInfo::SCmd &cmd = fmtProcessed.cmds[k];
			cmd.vPos.x += nSizeXAdd / 2;
			cmd.vPos.y += nSizeYAdd / 2;
		}
		resSize.x += nSizeXAdd;
		resSize.y += nSizeYAdd;
	}
	size = resSize;

	bNeedUpdate = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x20147BC0, CIconOutliner )
