#include "stdafx.h"
#include "GSceneParticles.h"
#include "GfxUtils.h"


namespace NGScene
{

// CParticlesTriList

void CParticlesTriList::Recalc()
{
	bNeedUpdate = false;
	value.resize( particles.size() );
	int nOffset = 0;
	for ( int i = 0; i < particles.size(); ++i )
	{
		NGfx::MakeQuadTriList( particles[i], &value[i] );
		value[i].nBaseIndex = nOffset * 4;
		nOffset += particles[i];
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x01561150, CParticlesTriList )
REGISTER_SAVELOAD_CLASS( 0x01561151, CShaderParticlesGeometry )
REGISTER_SAVELOAD_CLASS( 0x01561152, CTnLParticlesGeometry )

