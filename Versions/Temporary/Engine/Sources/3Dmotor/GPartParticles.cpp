#include "StdAfx.h"
#include "GPartParticles.h"
#include "GParts.h"

namespace NGScene
{

//	CParticles

CParticleEffect* CParticles::GetEffect()
{
	pParticles.Refresh();
	return pParticles->GetValue();
}

void CParticles::Unlink()
{
	if ( IsValid( pNode ) )
		pNode->particles.remove( this );
	pNode = 0;
}

void CParticles::SetFade( float fVal )
{
	pParticles.Refresh();
	pParticles->GetValue()->SetFade( fVal );
}

bool CParticles::Update( CVolumeNode *pVolume )
{
	if ( pPlacement.Refresh() )
	{
		TransformBound( &transformedBound, bound, pPlacement->GetValue().forward );
		CVolumeNode *pNewNode = pVolume->SelectNode( pPlacement, bound );
		if ( pNode != pNewNode )
		{
			pNewNode->particles.push_back( this );
			if ( pNode )
				pNode->particles.remove( this );
			pNode = pNewNode;
		}
	}
	pParticles.Refresh();
	return !pParticles->GetValue()->bEnd;
}
}

