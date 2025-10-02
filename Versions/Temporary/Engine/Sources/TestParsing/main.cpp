#include "stdafx.h"

#include <conio.h>
#include "../Parser/ParseOperations.h"
#include "../System/FileUtils.h"

EXTERNVAR bool byySuccess;

int main( int argc, char* argv[] )
{
	if ( argc < 2 || argc > 3 )
	{
		printf( "usage: parser.exe <dir> [<test mode 1|0>]" );
		return 0;
	}

	string szCurDir = NFile::GetCurrDir();
	string szDir = szCurDir + '\\' + argv[1];
	NLang::Parse( szDir, "*.h", argc == 3 && *(argv[2]) == '1' );

	return byySuccess ? 0 : 1;
}

