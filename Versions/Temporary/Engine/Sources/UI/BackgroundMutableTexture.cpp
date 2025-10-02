#include "StdAfx.h"
#include "..\3dmotor\rectlayout.h"
#include "BackgroundMutableTexture.h"
#include "UIVisitor.h"

REGISTER_SAVELOAD_CLASS( 0x19117340, CBackgroundMutableTexture )
REGISTER_SAVELOAD_CLASS_NM( 0x17191C00, CTextureData, CBackgroundMutableTexture )

//CBackgroundMutableTexture

void CBackgroundMutableTexture::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CBackground::InitByDesc( _pDesc );
}

void CBackgroundMutableTexture::Visit( interface IUIVisitor* pVisitor )
{
	if ( !pData ) 
		return;
	if ( pData->picture.IsEmpty() ) 
		return;

	CRectLayout rects;
	const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, pData->picture.GetSizeX(), pData->picture.GetSizeY() );
	rects.AddRect( pos.x1, pos.y1, pos.Width(), pos.Height(), rectTexture );

	VirtualToScreen( &rects );
	pVisitor->VisitUITextureRect( pData, 0, rects );
}

void CBackgroundMutableTexture::Set( const CArray2D<NGfx::SPixel8888> &src )
{
	if ( !IsValid( pData ) ) 
	{
		pData = new CTextureData; 
		pos.SetEmpty();
	}
	pData->Set( src );
}

int CBackgroundMutableTexture::operator&( interface IBinSaver &saver )
{
	saver.Add( 1, static_cast<CBackground*>( this ) );
	saver.Add( 2, &pData );
	return 0;
}

//CBackgroundMutableTexture::CTextureData

void CBackgroundMutableTexture::CTextureData::Set( const CArray2D<NGfx::SPixel8888> &src )
{
	picture = src;
	pValue = 0;
	bNeedUpdate = true;
}

void CBackgroundMutableTexture::CTextureData::Recalc()
{
	if ( picture.IsEmpty() || picture.GetSizeX() == 0 || picture.GetSizeY() == 0 )
	{
		picture.SetSizes( 1, 1 );
		picture[0][0] = 0;
	}

	pValue = NGfx::MakeTexture( GetNextPow2( picture.GetSizeX() ), GetNextPow2( picture.GetSizeY() ), 
		1, NGfx::SPixel8888::ID, NGfx::DYNAMIC_TEXTURE, NGfx::CLAMP );

	if ( !IsValid( pValue ) )
		return;

	NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, 0, NGfx::INPLACE );

	for ( int nTempY = 0; nTempY < picture.GetSizeY(); nTempY++ )
	{
		for ( int nTempX = 0; nTempX < picture.GetSizeX(); nTempX++ )
		{
			sLock[nTempY][nTempX] = picture[nTempY][nTempX];
			sLock[nTempY][nTempX].a = 0xFF;
		}
	}

	bNeedUpdate = false;
}

