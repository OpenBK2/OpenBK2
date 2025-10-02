#include "stdafx.h"

#include "Shell.h"

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

	return 0;
}
