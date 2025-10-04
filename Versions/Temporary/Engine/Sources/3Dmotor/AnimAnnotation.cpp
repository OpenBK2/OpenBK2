#include "stdafx.h"

#include "AnimAnnotation.h"
#include "System/BasicShare.h"
#include "GAnimFormat.h"
#include "GAnimation.hpp"
#include "vendor/granny/include/granny.h"
#include "DBScene.h"

namespace NAnimation
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	extern CBasicShare<CDBPtr<NDb::SAnimBase>, CGrannyAnimationLoader, SDBPtrHash> shareAnimations;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const granny_animation * GetGrannyAnimation( CGrannyFileInfo *pGrannyFileInfo, const SAnimHandle &animHandle )
	{
		granny_file_info *pFileInfo = pGrannyFileInfo->GetData();
		ASSERT( pFileInfo && animHandle.nAnimNumber >= 0 && animHandle.nAnimNumber < pFileInfo->AnimationCount );
		if ( pFileInfo && animHandle.nAnimNumber >= 0 && animHandle.nAnimNumber < pFileInfo->AnimationCount )
		{
			return pFileInfo->Animations[ animHandle.nAnimNumber ];
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// this is very primitive for now -- it's get first available text track
	const granny_text_track * GetTextTrack( const granny_animation *pAnimation, const string &szTrackName )
	{
		if ( pAnimation->TrackGroupCount > 0 && pAnimation->TrackGroups[0]->TextTrackCount > 0 )
		{
			return &pAnimation->TrackGroups[0]->TextTracks[0];
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	unsigned int GetMarkTimes( vector<float> *pResult, const SAnimHandle &animHandle, const string &szTrackName, const string &szMarkName )
	{
		CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pGrannyFileLoader = shareAnimations.Get( animHandle.pAnimFile );
		pGrannyFileLoader.Refresh();
		NI_ASSERT( pGrannyFileLoader->GetValue(), "Can't load anim resource" );

		if ( pGrannyFileLoader->GetValue() )
		{
			const granny_animation *pAnimation = GetGrannyAnimation( pGrannyFileLoader->GetValue(), animHandle );
			if ( pAnimation )
			{
				const granny_text_track *pTrack = GetTextTrack( pAnimation, szTrackName );
				if ( pTrack )
				{
					for ( int i = 0; i < pTrack->EntryCount; ++i )
					{
						const granny_text_track_entry &entry = pTrack->Entries[i];
						if ( szMarkName == entry.Text )
						{
							pResult->push_back( entry.TimeStamp );
						}
					}
				}
			}
		}
		return 0;
	}
}

