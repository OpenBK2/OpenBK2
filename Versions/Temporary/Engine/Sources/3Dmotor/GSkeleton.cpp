#include "StdAfx.h"
#include "../vendor/Granny/include/granny.h"
#include "GAnimFormat.h"
#include "../System/BasicShare.h"
#include "GSkeleton.h"
#include "DBScene.h"
namespace NAnimation
{

static CBasicShare<CDBPtr<NDb::SSkeleton>, CGrannySkeletonLoader, SDBPtrHash> shareGrannySkeletons(119);

CPtrFuncBase<CGrannyFileInfo>* GetSkeletonFileInfo( const NDb::SSkeleton *pSkeleton )
{
	if ( !pSkeleton )
		return 0;
	return shareGrannySkeletons.Get( pSkeleton );
}

granny_skeleton *GetSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile )
{
	granny_skeleton *pSkeleton = 0;
	if ( pGrannyFI && pGrannyFI->GetData() )
	{
		granny_file_info *pFI = pGrannyFI->GetData();
		if ( nModelInFile >= 0 && nModelInFile < pFI->ModelCount )
			pSkeleton = pFI->Models[ nModelInFile ]->Skeleton;
	}
	return pSkeleton;
}

char *GetModelNameOfSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile )
{
	if ( pGrannyFI && pGrannyFI->GetData() )
	{
		granny_file_info *pFI = pGrannyFI->GetData();
		if ( nModelInFile >= 0 && nModelInFile < pFI->ModelCount )
			return pFI->Models[ nModelInFile ]->Name;
	}
	return 0;
}

};

