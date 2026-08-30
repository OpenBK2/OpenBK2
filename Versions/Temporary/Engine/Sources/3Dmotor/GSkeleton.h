#pragma once
#include "System/Dg.h"

#include <cstring>

struct granny_skeleton;

namespace NDb
{
	struct SSkeleton;
}

namespace NGScene
{
typedef std::vector<SHMatrix> SSkeletonMatrices;
}

namespace NAnimation
{	

// Engine-owned animation pose.  Format-specific animators translate their data
// into this representation, so renderers and gameplay mutators never see GR2
// or glTF implementation types.
struct SBoneTransform
{
	enum EFlags
	{
		HAS_POSITION = 1,
		HAS_ORIENTATION = 2,
		HAS_SCALE_SHEAR = 4
	};

	unsigned int Flags;
	float Position[3];
	float Orientation[4];
	float ScaleShear[3][3];

	SBoneTransform() :
		Flags( HAS_POSITION | HAS_ORIENTATION | HAS_SCALE_SHEAR )
	{
		Position[0] = Position[1] = Position[2] = 0.0f;
		Orientation[0] = Orientation[1] = Orientation[2] = 0.0f;
		Orientation[3] = 1.0f;
		memset( ScaleShear, 0, sizeof(ScaleShear) );
		ScaleShear[0][0] = ScaleShear[1][1] = ScaleShear[2][2] = 1.0f;
	}
};

struct SSkeletonPose
{
	// Kept as Granny-compatible column-major storage to preserve saved games.
	float poseGlobal[16];
	std::vector<SBoneTransform> localPose;
	NGScene::SSkeletonMatrices worldPose;
	NGScene::SSkeletonMatrices compositePose;

	SSkeletonPose()
	{
		memset( poseGlobal, 0, sizeof(poseGlobal) );
		poseGlobal[0] = poseGlobal[5] = poseGlobal[10] = poseGlobal[15] = 1.0f;
	}

	SBoneTransform *GetBone( int nBone )
	{
		return nBone >= 0 && nBone < static_cast<int>(localPose.size()) ? &localPose[nBone] : 0;
	}
	const SBoneTransform *GetBone( int nBone ) const
	{
		return nBone >= 0 && nBone < static_cast<int>(localPose.size()) ? &localPose[nBone] : 0;
	}
};

struct SSkeletonHandle
{
	ZDATA
	CDBPtr<NDb::SSkeleton> pSkeleton;
	int nModelInFile;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSkeleton); f.Add(3,&nModelInFile); return 0; }

	SSkeletonHandle() : nModelInFile(0) {}
	SSkeletonHandle( const NDb::SSkeleton *_pSkeleton, int _nModelInFile ) : pSkeleton(_pSkeleton), nModelInFile(_nModelInFile) {}
};

// Source compatibility for modules and old save data written before the
// format-neutral animator was introduced.
using SGrannySkeletonPose = SSkeletonPose;
using SGrannySkeletonHandle = SSkeletonHandle;

class CGrannyFileInfo;
granny_skeleton *GetSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile );
const char *GetModelNameOfSkeleton( CGrannyFileInfo *pGrannyFI, const int nModelInFile );
CPtrFuncBase<CGrannyFileInfo> *GetSkeletonFileInfo( const NDb::SSkeleton *pSkeleton );

struct IGetBone
{
	virtual int GetBoneIndex( const char *pszName ) = 0;
	virtual void GetBoneNames( std::vector<std::string> *pBoneNames ) = 0;
};

}


