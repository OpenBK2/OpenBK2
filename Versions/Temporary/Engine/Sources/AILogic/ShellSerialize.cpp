#include "stdafx.h"

#include "Shell.h"

int CFlameThrowerExpl::operator&( IBinSaver &saver )
{
	// Legacy flame saves wrote the base chunks directly, without a nested base chunk.
	CExplosion::operator&( saver );
	saver.Add( 14, &vShooterPos );
	saver.Add( 15, &vTargetPos );

	// Missing primitive chunks are zero-filled, so a marker distinguishes old saves.
	bool bHasFlameEndpoints = true;
	saver.Add( 16, &bHasFlameEndpoints );
	if ( saver.IsReading() && !bHasFlameEndpoints )
	{
		// The old path cannot be recovered; resolve one burst at the saved impact.
		vShooterPos = explCoord;
		vTargetPos = explCoord;
	}
	return 0;
}

int CVisShell::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CShell*>(this) ); 
	saver.Add( 2, &pTraj );
	saver.Add( 3, &center );
	saver.Add( 4, &speed );
	saver.Add( 5, static_cast<CLinkObject*>(this) );
	
	if ( !saver.IsChecksum() )
		saver.Add( 6, &bVisible );
	
	saver.Add( 7, &nPlatform );
	saver.Add( 8, &nOrder );

	return 0;
}

