#pragma once

#include "GSkeleton.h"
#include "System/FilePath.h"

#include <fastgltf/types.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace NDb
{
	class CResource;
}

namespace NGltf
{

// Parsed documents are immutable and shared by geometry, skeleton and animation
// loaders. Materials and images deliberately have no engine representation here.
class CGltfFile
{
public:
	fastgltf::Asset asset;
	NFile::CFilePath sourcePath;
	std::vector<int> nodeParents;
	std::vector<SHMatrix> nodeWorldTransforms;
	std::vector<std::size_t> meshNodes;

	CGltfFile( fastgltf::Asset &&_asset, const NFile::CFilePath &_sourcePath );
};

typedef std::shared_ptr<CGltfFile> TGltfFilePtr;

struct SSkeletonDefinition
{
	std::vector<std::string> boneNames;
	std::vector<int> parents;
	std::vector<NAnimation::SBoneTransform> restPose;
	std::vector<SHMatrix> inverseBindMatrices;
	std::unordered_map<std::string, int> boneByName;

	int FindBone( const std::string &name ) const;
};

// ModelFileRef first searches beside its owning DB resource. If that candidate
// is absent, the supplied path is used directly (including absolute paths).
NFile::CFilePath ResolveModelFilePath( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef );
bool DoesModelFileExist( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef );
TGltfFilePtr LoadFile( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef );
// A non-empty selector is matched case-sensitively and includes mesh-bearing descendants.
bool GetMeshNodes( const TGltfFilePtr &file, const std::string &rootNodeName,
	std::vector<std::size_t> *pResult );
// Calculates bounds in the same engine vertex space used by the GLB loaders.
// Static AI geometry applies every node transform, while render geometry keeps
// skinned vertices in their bind-pose mesh space for the animator.
bool GetMeshBoundingBox( const TGltfFilePtr &file, const std::string &rootNodeName,
	bool bApplyNodeTransformsToSkinnedMeshes, CVec3 *pMin, CVec3 *pMax );
int GetMeshCount( const NDb::CResource *pOwner, const NFile::CFilePath &modelFileRef,
	const std::string &rootNodeName );
bool BuildSkeleton( const TGltfFilePtr &file, int nSkin, SSkeletonDefinition *pResult );
// With no glTF skins, RootJoint selects a rigid node hierarchy instead.
bool BuildSkeleton( const TGltfFilePtr &file, const std::string &rootNodeName,
	int nFallbackSkin, SSkeletonDefinition *pResult );

CVec3 ConvertPosition( const fastgltf::math::fvec3 &value );
CVec3 ConvertDirection( const fastgltf::math::fvec3 &value );
CQuat ConvertRotation( const fastgltf::math::fquat &value );
CVec3 ConvertScale( const fastgltf::math::fvec3 &value );
SHMatrix ConvertMatrix( const fastgltf::math::fmat4x4 &value );
SHMatrix MakeLocalMatrix( const NAnimation::SBoneTransform &value );
NAnimation::SBoneTransform MakeBoneTransform( const SHMatrix &value );

}
