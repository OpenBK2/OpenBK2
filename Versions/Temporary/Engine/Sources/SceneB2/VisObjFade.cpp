#include "stdafx.h"

#include "VisObjDesc.h"
#include "SceneInternal.h"
#include "3Dmotor/GPostProcessors.h"
#include "3Dmotor/GSceneUtils.h"

const float FADE_VALUE_UNDER_CURSOR = 0.5f;

void CScene::SetFadedObjects( const std::list<int> &objects )
{
	std::unordered_map<int, bool> old_fade;
	old_fade.swap( data[eScene]->fadeModes );
	for ( std::list<int>::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		data[eScene]->fadeModes[*it] = true;
	}
	// включим 	
	for ( std::unordered_map<int, bool>::iterator it = data[eScene]->fadeModes.begin(); it != data[eScene]->fadeModes.end(); ++it )
	{
		if ( old_fade.find( it->first ) == old_fade.end() )
		{
			FadeObject( it->first, true );
		}
	}
	// выключим старое
	for ( std::unordered_map<int, bool>::iterator it = old_fade.begin(); it != old_fade.end(); ++it )
	{
		if ( data[eScene]->fadeModes.find( it->first ) == data[eScene]->fadeModes.end() )
		{
			FadeObject( it->first, false );
		}
	}
}


void CScene::SetFadedObjects( const std::list<int> &objects, float fFade )
{
	for ( std::list<int>::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		 FadeObject( *it, fFade );
	}
}


void CScene::FadeObject( int nID, bool bFade )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;

	if ( pos->second->pObj )
	{
		float fFade = bFade ? FADE_VALUE_UNDER_CURSOR : 1.0f;
		pos->second->SetFade( fFade );
		pos->second->ReCreateObject( data[eScene]->GetGScene(), data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	}
}


void CScene::FadeObject( int nID, float fFade )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;

	if ( pos->second->pObj )
	{
		pos->second->SetFade( fFade );
		pos->second->ReCreateObject( data[eScene]->GetGScene(), data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	}
}


void CScene::ClearPostEffectObjects()
{
	data[eScene]->postEffects.clear();
}

void CScene::AddPostEffectObjects( const std::list<int> &objects, const CVec4 &vColor )
{
	std::vector< CObjectBase* > filterObjects;
	filterObjects.reserve( 100 );
	for ( std::list<int>::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( *it );
		if ( pos != data[eScene]->visObjects.end() )
		{
			if ( pos->second->pObj )
			{
				filterObjects.push_back( pos->second->pObj );
			}
		}
	}
	data[eScene]->postEffects.push_back( data[eScene]->GetGScene()->AddPostFilter( filterObjects, 
		new NGScene::COccludedColorer( new NGScene::CCVec4( vColor ) ) ) );
}


