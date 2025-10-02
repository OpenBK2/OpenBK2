#pragma once

#include "Main_export.h"


interface IAICmdsAutoMagic : public CObjectBase
{
	MAIN_EXPORT virtual IBinSaver *MakeCommandSerializer( CMemoryStream *pStream, ESaverMode mode );
	virtual int GetCommandID( CObjectBase *p ) = 0;
	virtual CObjectBase *MakeCommand( int nID ) = 0;
	virtual int GetIDSize() const = 0;
};
IAICmdsAutoMagic *CreateDefaultCmdsCreator();

