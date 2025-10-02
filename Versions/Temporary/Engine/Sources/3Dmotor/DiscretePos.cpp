#include "StdAfx.h"
#include "DiscretePos.h"
#include "..\3DLib\Transform.h"

static CObj<CFBTransform> pIdentity;
static struct SInitDiscretePos
{
	SInitDiscretePos()
	{
		SFBTransform p;
		Identity( &p.forward );
		Identity( &p.backward );
		pIdentity = new CFBTransform( p );
	}
} SInitDiscretePosval;

void SDiscretePos::MakeMatrix( SFBTransform *pRes ) const
{
	switch( nRotation )
	{
		case TURN_0:
			*pRes = MakeTransform( ptMove );
			break;
		case TURN_90:
			*pRes = MakeTransform( ptMove, 90 );
			break;
		case TURN_180:
			*pRes = MakeTransform( ptMove, 180 );
			break;
		case TURN_270:
			*pRes = MakeTransform( ptMove, 270 );
			break;
		case FLIP:
			*pRes = MakeTransform( ptMove, 0 );
			break;
	}
	if ( pTransform )//->IsValid() ) // CRAP - not enough ref counts for building DiscretePos
		*pRes = pTransform->pos * *pRes;
}

CFBTransform* SDiscretePos::GetTransform() const
{
	if ( pTransform )//->IsValid() ) // CRAP - not enough ref counts for building DiscretePos
		return pTransform;
	return pIdentity;
}

REGISTER_SAVELOAD_CLASS( 0x025a1130, CFBTransform );


