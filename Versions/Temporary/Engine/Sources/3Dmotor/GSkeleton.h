#pragma once
#include "System/Dg.h"

// typedefs required for animation etc

namespace granny {
	struct local_pose;

	struct file;
}

struct granny_skeleton;

typedef granny::local_pose granny_local_pose;

namespace NDb
{
	struct SSkeleton;
}

namespace NGScene
{
typedef vector<SHMatrix> SSkeletonMatrices;
}

namespace NAnimation
{	

struct SGrannySkeletonPose
{
	float poseGlobal[16];
	granny_local_pose *pPose;
	SGrannySkeletonPose() : pPose(0) {}
};

struct SGrannySkeletonHandle
{
	ZDATA
	CDBPtr<NDb::SSkeleton> pSkeleton;
	int nModelInFile;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSkeleton); f.Add(3,&nModelInFile); return 0; }

	SGrannySkeletonHandle() {}
	SGrannySkeletonHandle( const NDb::SSkeleton *_pSkeleton, int _nModelInFile ) : pSkeleton(_pSkeleton), nModelInFile(_nModelInFile) {}
};

class CGrannyFileInfo;
granny_skeleton *GetSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile );
const char *GetModelNameOfSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile );
CPtrFuncBase<CGrannyFileInfo> *GetSkeletonFileInfo( const NDb::SSkeleton *pSkeleton );

struct IGetBone
{
	virtual int GetBoneIndex( const char *pszName ) = 0;
	virtual void GetBoneNames( vector<string> *pBoneNames ) = 0;
};

}


