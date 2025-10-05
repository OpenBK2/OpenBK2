#include "stdafx.h"
#include "GDecal.h"
#include "GSceneUtils.h"
#include "GSceneInternal.h"

namespace NGScene
{

// CDecal

CDecal::CDecal( CDecalsManager *_pOwner, CDecalTarget *_pTarget, IMaterial *_pMaterial ) : pOwner(_pOwner), pTarget(_pTarget), pMaterial(_pMaterial)
{
	// create target geometry
	if ( !pTarget->parts.empty() )
	{
		std::vector<CPtr<ISomePart> > &parts = pTarget->parts;
		for ( int k = 0; k < parts.size(); ++k )
		{
			ISomePart *pPart = parts[k];
			if ( !IsValid( pPart ) )
				continue;
			const SFullGroupInfo &fg = pPart->GetFullGroupInfo();
			SDecalTargetPart tp( fg.pUser, fg.nUserID );
			// calc source position
			bool bAddToTargets = true;
			if ( pPart->HasLoadedObjectInfo() )
			{
				CDGPtr<CPtrFuncBase<CObjectInfo> > pSource( pPart->GetObjectInfoNode() );
				pSource.Refresh();
				if ( pSource->GetValue() )
				{
					SSrcPosInfo srcPos( tp.pUser, tp.nUserID, pSource );
					TransformPart( pPart, &pTarget->srcPositions[srcPos], 0 );
					if ( !OnCreate( pOwner->GetScene(), pPart, srcPos ) )
						bAddToTargets = false;
				}
			}
			if ( bAddToTargets )
				pTarget->targetParts.push_back( tp );
		}
		parts.clear();
	}
	pOwner->Register( this, pTarget );
}

CDecal::~CDecal()
{
	if ( IsValid(pOwner) )
		pOwner->Unregister( this, pTarget );
}

template<class T>
static void WalkVector( std::vector<T> *pRes )
{
	int nRes = 0;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		if ( IsValid( (*pRes)[k] ) )
		{
			if ( nRes != k )
				(*pRes)[ nRes++ ] = (*pRes)[k];
			else
				++nRes;
		}
	}
	pRes->resize( nRes );
}

bool CDecal::OnCreate( IDecalQuery *pScene, ISomePart *pNew, const SSrcPosInfo &tp )
{
	CDecalTarget::CSrcPosHash::iterator i = pTarget->srcPositions.find( tp );
	CObjectBase *pRes;
	if ( i != pTarget->srcPositions.end() )
		pRes = pScene->CreateDecal( pNew, i->second, pTarget->mapInfo, pMaterial );
	else
		pRes = pScene->CreateDecal( pNew, std::vector<CVec3>(), pTarget->mapInfo, pMaterial );
	if ( !pRes )
		return false;
	if ( ( pNew->decals.size() & 31 ) == 0 )
		WalkVector( &pNew->decals );
	pNew->decals.push_back( pRes );
	decals.push_back( pRes );
	return true;
}

void CDecal::Walk()
{
	WalkVector( &decals );
}

// CDecalsManager

void CDecalsManager::OnCreate( ISomePart *pNew )
{
	const SFullGroupInfo &fg = pNew->GetFullGroupInfo();
	SDecalTargetPart tp( fg.pUser, fg.nUserID );
	SSrcPosInfo srcPos( tp.pUser, tp.nUserID, pNew->GetObjectInfoNode() );
	CPerUserHash::iterator i = decalsPerUser.find( tp );
	if ( i != decalsPerUser.end() )
	{
		// add decal on each CDecal
		std::vector<CPtr<CDecal> > &decals = i->second;
		for ( int k = 0; k < decals.size(); ++k )
		{
			ASSERT( IsValid(decals[k]) );
			if ( IsValid(decals[k]) )
				decals[k]->OnCreate( pScene, pNew, srcPos );
		}
	}
}

CDecalTarget* CDecalsManager::CreateDecalTarget( const std::vector<CObjectBase*> &_targets, const SDecalMappingInfo &_info )
{
	CDecalTarget *pRes = new CDecalTarget( _info );
	CObjectBaseSet targets;
	for ( int k = 0; k < _targets.size(); ++k )
		targets[ _targets[k] ] = 1;
	pScene->GetPartsList( _info, targets, &pRes->parts );
	return pRes;
}

CDecal* CDecalsManager::CreateDecal( CDecalTarget *pTarget, IMaterial *pMaterial )
{
	if ( !IsValid(pTarget) )
	{
		ASSERT(0);
		return 0;
	}
	return new CDecal( this, pTarget, pMaterial );
}

void CDecalsManager::Register( CDecal *pDecal, CDecalTarget *pTarget )
{
	ASSERT( IsValid( pTarget ) );
	const std::vector<SDecalTargetPart> &targetParts = pTarget->targetParts;
	for ( int k = 0; k < targetParts.size(); ++k )
	{
		bool bFound = false;
		std::vector<CPtr<CDecal> > &decals = decalsPerUser[ targetParts[k] ];
		for ( int i = 0; i < decals.size(); ++i )
		{
			if ( decals[i] == pDecal )
			{
				bFound = true;
				break;
			}
		}
		if ( !bFound )
			decals.push_back( pDecal );
	}
}

void CDecalsManager::Unregister( CDecal *pDecal, CDecalTarget *pTarget )
{
	ASSERT( IsValid( pTarget ) );
	const std::vector<SDecalTargetPart> &targetParts = pTarget->targetParts;
	for ( int k = 0; k < targetParts.size(); ++k )
	{
		CPerUserHash::iterator i = decalsPerUser.find( targetParts[k] );
		if ( i == decalsPerUser.end() )
			continue;
		std::vector<CPtr<CDecal> > &d = i->second;
		int nRes = 0;
		for ( int m = 0; m < d.size(); ++m )
		{
			if ( d[m] != pDecal )
				d[ nRes++ ] = d[m];
		}
		if ( nRes == 0 )
			decalsPerUser.erase( i );
		else
			d.resize( nRes );
	}
}

void CDecalsManager::Walk()
{
	for ( CPerUserHash::iterator i = decalsPerUser.begin(); i != decalsPerUser.end(); ++i )
	{
		std::vector<CPtr<CDecal> > &r = i->second;
		for ( int k = 0; k < r.size(); ++k )
		{
			if ( IsValid(r[k]) )
				r[k]->Walk();
			else
				ASSERT(0);
		}
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x003c2140, CDecal )
REGISTER_SAVELOAD_CLASS( 0x003c2141, CDecalsManager )
REGISTER_SAVELOAD_CLASS( 0x003c2142, CDecalTarget )

