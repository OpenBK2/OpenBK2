#include "StdAfx.h"
#include "../vendor/Granny/include/granny.h"
#include "GAnimFormat.h"
#include "../System/BasicShare.h"

namespace NAnimation
{

void CGrannyFileInfo::InitMe() 
{ 
	if ( pFile ) 
		pData = GrannyGetFileInfo( pFile ); 
	else
		pData = 0;
}

static CBasicShare< NGScene::SResKey<NGScene::SGrannyFileLoaderInfo>, NGScene::CGrannyMemFileLoader, NGScene::SGrannyFileLoaderInfoHash> shareAnimGrannyFiles(105);

void CGrannyBaseStuffLoader::TrueSetKey( const NGScene::SResKey<NGScene::SGrannyFileLoaderInfo> &key )
{
	pGrannyFile = shareAnimGrannyFiles.Get( key );
}

void CGrannyBaseStuffLoader::Recalc()
{
	if ( pGrannyFile->GetValue() == 0 )
		pValue = 0;
	else
		pValue = new CGrannyFileInfo( pGrannyFile->GetValue()->pFile );
}

} // namespace NAnimation
using namespace NAnimation;
REGISTER_SAVELOAD_CLASS( 0x13173BC2, CGrannyBaseStuffLoader )
REGISTER_SAVELOAD_CLASS( 0x70683130, CGrannyAnimationLoader )
REGISTER_SAVELOAD_CLASS( 0x70493150, CGrannyAIGeomLoader )
REGISTER_SAVELOAD_CLASS( 0x100A2440, CGrannySkeletonLoader )
BASIC_REGISTER_CLASS( CGrannyFileInfo )
