#include "StdAfx.h"
#include "GFont.h"

namespace NGScene
{

// CFileFont

void CFileFont::Recalc()
{
	CResourceOpener file( "Fonts", GetKey() );
	if ( file.IsOk() )
		file->Add( 1, &pValue );
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02321160, CFileFont );

