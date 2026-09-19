#include "stdafx.h"

#include "BusyCursor.h"

namespace
{
	CBusyCursor::TCursorHandler s_pfnBegin = nullptr;
	CBusyCursor::TCursorHandler s_pfnEnd = nullptr;
}


CBusyCursor::CBusyCursor()
{
	if ( ( s_pfnBegin != nullptr ) && ( s_pfnEnd != nullptr ) )
	{
		s_pfnBegin();
		bShown = true;
	}
}


CBusyCursor::~CBusyCursor()
{
	if ( bShown && ( s_pfnEnd != nullptr ) )
	{
		s_pfnEnd();
	}
}


void CBusyCursor::SetHandlers( TCursorHandler pfnBegin, TCursorHandler pfnEnd )
{
	s_pfnBegin = pfnBegin;
	s_pfnEnd = pfnEnd;
}
