#pragma once

#include "GSkeleton.h"
#include "System/FilePath.h"

#include <fastgltf/types.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

TGltfFilePtr LoadFile( const NFile::CFilePath &path );
int GetMeshCount( const NFile::CFilePath &path );
bool BuildSkeleton( const TGltfFilePtr &file, int nSkin, SSkeletonDefinition *pResult );

CVec3 ConvertPosition( const fastgltf::math::fvec3 &value );
CVec3 ConvertDirection( const fastgltf::math::fvec3 &value );
CQuat ConvertRotation( const fastgltf::math::fquat &value );
CVec3 ConvertScale( const fastgltf::math::fvec3 &value );
SHMatrix ConvertMatrix( const fastgltf::math::fmat4x4 &value );
SHMatrix MakeLocalMatrix( const NAnimation::SBoneTransform &value );
NAnimation::SBoneTransform MakeBoneTransform( const SHMatrix &value );

}
