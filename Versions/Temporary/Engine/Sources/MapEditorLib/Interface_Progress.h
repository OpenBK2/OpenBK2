#pragma once

#include "MapEditorLib_export.h"

namespace NProgress
{
	MAPEDITORLIB_EXPORT void Create( bool bShow );
	MAPEDITORLIB_EXPORT void Destroy();

	void SetTitle( const std::string &rszTitle );
	MAPEDITORLIB_EXPORT void SetMessage( const std::string &rszMessage );
	MAPEDITORLIB_EXPORT void SetRange( int nStart, int nFinish );
	MAPEDITORLIB_EXPORT void SetPosition( int nPosition );
	MAPEDITORLIB_EXPORT void IteratePosition();
};



