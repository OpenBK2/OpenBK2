#pragma once

#include "System_export.h"

struct IPointerSerialization : virtual public CObjectBase
{
	virtual int GetObjectID( CObjectBase *p ) = 0;
	virtual CObjectBase *GetObject( int nID ) = 0;
};

class CMemoryStream;
SYSTEM_EXPORT IBinSaver *CreateChunklessSaver( IPointerSerialization *pPtr, CMemoryStream *pStream, ESaverMode mode );

