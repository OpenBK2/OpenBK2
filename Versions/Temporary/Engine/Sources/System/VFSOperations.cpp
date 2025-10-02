#include "stdafx.h"

#include "VFSOperations.h"

namespace NVFS
{
static CPtr<IVFS> pMainVFS;
static CPtr<IFileCreator> pMainFileCreator;

IVFS *GetMainVFS()
{
	if ( !IsValid( pMainVFS ) )
		pMainVFS = 0;

	return pMainVFS;
}

IFileCreator *GetMainFileCreator()
{
	if ( !IsValid( pMainFileCreator ) )
		pMainFileCreator = 0;

	return pMainFileCreator;
}

void SetMainVFS( IVFS *pVFS )
{
	pMainVFS = pVFS;
}

void SetMainFileCreator( IFileCreator *pFileCreator )
{
	pMainFileCreator = pFileCreator;
}

}


