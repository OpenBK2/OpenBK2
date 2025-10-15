#include "stdafx.h"
#include "GFont.h"

#include "3Dmotor_export.h"

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
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02321160, CFileFont );

