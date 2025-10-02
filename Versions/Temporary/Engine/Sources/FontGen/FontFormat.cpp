#include "StdAfx.h"

#include "FontFormat.h"

int CFontFormatInfo::operator&( CStructureSaver &f )
{
	f.Add( 10, &nHeight );
	f.Add( 11, &nExternalLeading );
  f.Add( 12, &nAveCharWidth );
  f.Add( 13, &nMaxCharWidth );
  f.Add( 14, &cCharSet );
	f.Add( 15, &wDefaultChar );
	f.Add( 1, &chars );
	f.Add( 2, &kerns );
	return 0;	
}

REGISTER_SAVELOAD_CLASS( 0x02321170, CFontFormatInfo );
