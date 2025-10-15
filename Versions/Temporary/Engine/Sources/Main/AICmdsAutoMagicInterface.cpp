#include "stdafx.h"
#include "AICmdsAutoMagicInterface.h"
#include "System/ChunklessSaver.h"


IBinSaver *IAICmdsAutoMagic::MakeCommandSerializer( CMemoryStream *pStream, ESaverMode mode )
{
	return CreateChunklessSaver( 0, pStream, mode );
}

class CDefaultAICmdsAutoMagic : public IAICmdsAutoMagic
{
	OBJECT_NOCOPY_METHODS(CDefaultAICmdsAutoMagic);
	int GetCommandID( CObjectBase *p ) { return NObjectFactory::GetObjectTypeID( p ); }
	CObjectBase *MakeCommand( int nID ) { return NObjectFactory::MakeObject( nID ); }
	int GetIDSize() const { return 4; }
};
IAICmdsAutoMagic *CreateDefaultCmdsCreator()
{
	return new CDefaultAICmdsAutoMagic ;
}
BASIC_REGISTER_CLASS( MAIN, IAICmdsAutoMagic )
REGISTER_SAVELOAD_CLASS( MAIN, 0x20129340, CDefaultAICmdsAutoMagic )

