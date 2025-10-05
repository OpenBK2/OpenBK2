#include "stdafx.h"
#include "3Dmotor/rectlayout.h"
#include "uivisitor.h"
#include "WindowPlayer.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"


REGISTER_SAVELOAD_CLASS(0x170A7B82, CWindowPlayer);

// CWindowPlayer

int CWindowPlayer::SMovie::operator&( IXmlSaver &saver )
{
	saver.Add( "FileName", &szFileName );
	saver.Add( "CanSkipMovie", &bCanSkipMovie );
	saver.Add( "CanSkipSequence", &bCanSkipSequence );
	saver.Add( "SlideShowMode", &bSlideShowMode );
	return 0;
}
int CWindowPlayer::SMovie::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bCanSkipMovie );
	saver.Add( 3, &bCanSkipSequence );
	saver.Add( 4, &bSlideShowMode );
	saver.Add( 5, &szFileName );
	return 0;
}


int CWindowPlayer::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pInstance );
	saver.Add( 2, &pShared );
	saver.Add( 3, &pPlayer );
	saver.Add( 4, &movies );
	saver.Add( 5, &nCurrMovie );
	saver.Add( 6, static_cast<CWindow*>( this ) );
	return 0;
}

void CWindowPlayer::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowPlayer *pDesc( checked_cast<const NDb::SWindowPlayer*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowPlayerShared *>( pDesc->pShared );
	bPaused = false;
	nCurrMovie = 0;
	if ( !pInstance->szSequenceName.empty() )
		SetSequence( pInstance->szSequenceName );
}

void CWindowPlayer::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
}

void CWindowPlayer::Visit( struct IUIVisitor *pVisitor )
{
	if ( !IsVisible() )
		return;

	CWindow::Visit( pVisitor );

	UpdatePlayer();
	if ( !pPlayer )
		return;
	
	CTPoint<int> textureSize;
	pPlayer->GetSize( &textureSize );
	const CTRect<float> textureRect = CTRect<float>( 0.0f, 0.0f, textureSize.x, textureSize.y );

	CTRect<float> dstRect;
	VirtualToScreen( GetWindowRect(), &dstRect );

	if ( pInstance->bMaintainAspectRatio )
	{
		float fScale = Min( dstRect.Width() / textureRect.Width(), dstRect.Height() / textureRect.Height() );
		CTPoint<float> shift( ( dstRect.Width() - textureRect.Width() * fScale ) / 2, 
			( dstRect.Height() - textureRect.Height() * fScale ) / 2 );

		CRectLayout black_rects;
		black_rects.AddRect( dstRect.x1, dstRect.y1, shift.x, dstRect.Height(), CTRect<float>( 0, 0, 0, 0 ), NGfx::SPixel8888( 0, 0, 0, 0xFF ) );
		black_rects.AddRect( dstRect.x1, dstRect.y1, dstRect.Width(), shift.y, CTRect<float>( 0, 0, 0, 0 ), NGfx::SPixel8888( 0, 0, 0, 0xFF ) );
		black_rects.AddRect( dstRect.x1 + dstRect.Width() - shift.x, dstRect.y1, shift.x, dstRect.Height(), CTRect<float>( 0, 0, 0, 0 ), NGfx::SPixel8888( 0, 0, 0, 0xFF ) );
		black_rects.AddRect( dstRect.x1, dstRect.y1 + dstRect.Height() - shift.y, dstRect.Width(), shift.y, CTRect<float>( 0, 0, 0, 0 ), NGfx::SPixel8888( 0, 0, 0, 0xFF ) );
		pVisitor->VisitUITextureRect( (CPtrFuncBase<NGfx::CTexture>*)0, 3, black_rects );

		CRectLayout rects;
		rects.AddRect( dstRect.x1 + shift.x, dstRect.y1 + shift.y, textureRect.Width() * fScale, textureRect.Height() * fScale, textureRect, 0xFFFFFFFF );
		//pVisitor->VisitUITextureRect( pPlayer, 3, rects );
	}
	else
	{
		CRectLayout rects;
		rects.AddRect( dstRect.x1, dstRect.y1, dstRect.Width(), dstRect.Height(), textureRect, 0xFFFFFFFF );
		//pVisitor->VisitUITextureRect( pPlayer, 3, rects );
	}
}

void CWindowPlayer::SetSequence( const std::string &szFileName )
{
	CFileStream stream( NVFS::GetMainVFS(), szFileName );
	if ( stream.IsOk() )
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( "Movies", &movies );
	}
	nCurrMovie = 0;
}

void CWindowPlayer::Play()
{
	bPaused = false;
	UpdatePlayer();
	if ( pPlayer && !pPlayer->IsPlaying() )
		pPlayer->Play();
}

bool CWindowPlayer::Stop()
{
	if ( pPlayer )
	{
		pPlayer->Stop();
		pPlayer = 0;
	}
	nCurrMovie = 0;
	return true;
}

bool CWindowPlayer::Pause( bool bPause )
{
	UpdatePlayer();
	if ( pPlayer )
	{
		bPaused = pPlayer->Pause( bPause );
		return bPaused;
	}
	return false;
}

bool CWindowPlayer::IsPlaying() const
{
	return pPlayer != 0;
}

bool CWindowPlayer::IsPaused() const
{
	return bPaused;
}

void CWindowPlayer::SkipMovie()
{
	if ( nCurrMovie >= movies.size() )
		return;
	if ( !movies[nCurrMovie].bCanSkipMovie )
		return;
		
	if ( pPlayer )
	{
		pPlayer->Stop();
		pPlayer = 0;
	}
	nCurrMovie++;
	UpdatePlayer();
}

void CWindowPlayer::SkipSequence()
{
	if ( nCurrMovie >= movies.size() )
		return;
	if ( !movies[nCurrMovie].bCanSkipSequence )
	{
		SkipMovie();
		return;
	}
	if ( pPlayer )
	{
		pPlayer->Stop();
		pPlayer = 0;
	}
	nCurrMovie = movies.size();
}

int CWindowPlayer::GetCurrentFrame() const
{
	if ( !pPlayer )
		return 0;
	return pPlayer->GetCurrentFrame();
}

void CWindowPlayer::SetCurrentFrame( int nFrame )
{
	if ( !pPlayer )
		return;
	return pPlayer->SetCurrentFrame( nFrame );
}

int CWindowPlayer::GetNumFrames() const
{
	if ( !pPlayer )
		return 0;
	return pPlayer->GetNumFrames();
}

void CWindowPlayer::UpdatePlayer()
{
	if ( bPaused )
		return;
	if ( pPlayer )
	{
		if ( pPlayer->IsPlaying() )
			return;
		pPlayer = 0;
		nCurrMovie++;
	}
	if ( nCurrMovie < movies.size() )
	{
		pPlayer = NGScene::CreateVideoPlayer( movies[nCurrMovie].szFileName + ".bik", GetSceneFlags() );
		if ( pPlayer )
			pPlayer->Play();
	}
}

DWORD CWindowPlayer::GetSceneFlags()
{
	DWORD dwFlags = 0;
	dwFlags |= NGScene::IVideoPlayer::COPY_ALL;
	//dwFlags |= NGScene::IVideoPlayer::PLAY_LOOPED;
	//dwFlags |= NGScene::IVideoPlayer::PLAY_NO_TIME_UPDATE;
	//dwFlags |= NGScene::IVideoPlayer::PLAY_FROM_MEMORY;
	if ( nCurrMovie < movies.size() && movies[nCurrMovie].bSlideShowMode )
		dwFlags |= NGScene::IVideoPlayer::PLAY_NO_TIME_UPDATE;
	else
		dwFlags |= NGScene::IVideoPlayer::PLAY_WITH_SOUND;
	return dwFlags;
}

void CWindowPlayer::PlayFragment( int nStartFrame, int nEndFrame, int nFrameSkip )
{
	bPaused = false;
	UpdatePlayer();
	if ( pPlayer )
	{
		if ( nStartFrame != nEndFrame )
			pPlayer->PlayFragment( nStartFrame, nEndFrame, nFrameSkip );
		else
		{
			pPlayer->SetCurrentFrame( nStartFrame );
			pPlayer->Pause( true );
		}

	}
}


