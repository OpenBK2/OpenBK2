#include "stdafx.h"

#include "AllAnimationsPlayer.h"
#include "MapObj.h"
#include "../Main/GameTimer.h"

CAllAnimationsPlayer::CAllAnimationsPlayer( const hash_map<int, CObj<CMapObj> > &_objects )
: objects( _objects ), bSwitchToNextAnimation( true )
{
}

void CAllAnimationsPlayer::Update()
{
	NTimer::STime curTime = Singleton<IGameTimer>()->GetGameTime();
	list<int> toDelete;
	for ( hash_map<int, CObj<CMapObj> >::iterator iter = objects.begin(); iter != objects.end(); ++iter )
	{
		SAnimationInfo &animInfo = playingAnimations[iter->first];
		if ( bSwitchToNextAnimation || ( animInfo.nStartNextAnimTime < curTime && !animInfo.bLooped ) )
		{
			CMapObj *pObj = iter->second;

			pair<int, bool> animDuration;
			int nStartAnim = animInfo.nAnimation;
			do
			{
				if ( bSwitchToNextAnimation )
					++animInfo.nAnimation;

				animDuration = pObj->PlayAnimation( animInfo.nAnimation );

				NI_ASSERT( animDuration.first > 0 || bSwitchToNextAnimation, "Wrong animation length" );

				if ( animDuration.first == -1 )
					animInfo.nAnimation = -1;
			} 
			while ( animDuration.first <= 0 && animInfo.nAnimation != nStartAnim );

			if ( animDuration.first > 0 )
			{
				animInfo.nStartNextAnimTime = curTime + animDuration.first + 1000;
				animInfo.bLooped = animDuration.second;
			}
			else
				toDelete.push_back( iter->first );
		}
	}

	while ( !toDelete.empty() )
	{
		objects.erase( toDelete.front() );
		toDelete.pop_front();
	}

	bSwitchToNextAnimation = false;
}

REGISTER_SAVELOAD_CLASS( 0x300BA500, CAllAnimationsPlayer )
