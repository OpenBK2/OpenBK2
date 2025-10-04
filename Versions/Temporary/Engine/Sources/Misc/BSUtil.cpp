#include "stdafx.h"
#include "BSUtil.h"

namespace NBSU
{

// do we need ignore ASSERT int this file:line
bool IsIgnore( const SIgnoresList &ignores, const char *pszFileName, int nLineNumber )
{
	for ( SIgnoresList::const_iterator pos = ignores.begin(); pos != ignores.end(); ++pos )
	{
		if ( pos->dwFlags & IGNORE_THIS )
		{
			if ( (pos->szFileName == pszFileName) && (pos->nLineNumber == nLineNumber) )
				return true;
		}
		else if ( pos->dwFlags & IGNORE_NON_THIS )
		{
			if ( (pos->szFileName != pszFileName) || (pos->nLineNumber != nLineNumber) )
				return true;
		}
		else if ( pos->dwFlags & IGNORE_FILE )
		{
			if ( pos->szFileName == pszFileName )
				return true;
		}
		else if ( pos->dwFlags & IGNORE_NON_FILE )
		{
			if ( pos->szFileName != pszFileName )
				return true;
		}
		else if ( pos->dwFlags & IGNORE_ALL )
			return true;
	}
	//
	return false;
}
}

