#include "stdafx.h"
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
	{
		std::vector<CPtr<CParticles> >::iterator i = std::find( pNode->particles.begin(), pNode->particles.end(), this );
		if ( i != pNode->particles.end() )
		{
			// Rendering may unlink this particle while iterating the vector; defer compaction until that pass ends.
			*i = 0;
		}
	}
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
			{
				std::vector<CPtr<CParticles> >::iterator i = std::find( pNode->particles.begin(), pNode->particles.end(), this );
				if ( i != pNode->particles.end() )
					pNode->particles.erase( i );
			}
			pNode = pNewNode;
		}
	}
	pParticles.Refresh();
	return !pParticles->GetValue()->bEnd;
}
}
