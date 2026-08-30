#include "stdafx.h"

#include "GltfFormat.h"

#include "System/Streams.h"
#include "System/VFSOperations.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>
#include <mutex>
#include <unordered_set>

namespace NGltf
{
namespace
{
std::mutex fileCacheMutex;
// Parsing also copies GLB buffer data, so keep one immutable document per VFS
// path instead of reparsing it independently for every mesh/material part.
std::unordered_map<NFile::CFilePath, TGltfFilePtr> fileCache;

bool ReadVfsFile( const std::string &path, std::vector<std::byte> *pBytes )
{
	CFileStream stream( NVFS::GetMainVFS(), path );
	if ( !stream.IsOk() || !stream.CanRead() )
		return false;
	pBytes->resize( stream.GetSize() );
	if ( !pBytes->empty() )
		memcpy( pBytes->data(), stream.GetBuffer(), pBytes->size() );
	return true;
}

SHMatrix IdentityMatrix()
{
	SHMatrix result;
	Identity( &result );
	return result;
}

void CollectMeshNodes( const fastgltf::Asset &asset, std::size_t nodeIndex,
	std::vector<bool> *pVisited, std::vector<std::size_t> *pResult )
{
	if ( nodeIndex >= asset.nodes.size() || (*pVisited)[nodeIndex] )
		return;
	(*pVisited)[nodeIndex] = true;
	const fastgltf::Node &node = asset.nodes[nodeIndex];
	if ( node.meshIndex.has_value() )
		pResult->push_back( nodeIndex );
	for ( std::size_t child : node.children )
		CollectMeshNodes( asset, child, pVisited, pResult );
}

SHMatrix ResolveNodeWorld( const fastgltf::Asset &asset, const std::vector<int> &parents,
	std::size_t nodeIndex, std::vector<SHMatrix> *pWorld, std::vector<unsigned char> *pState )
{
	if ( (*pState)[nodeIndex] == 2 )
		return (*pWorld)[nodeIndex];
	if ( (*pState)[nodeIndex] == 1 )
		return IdentityMatrix();
	(*pState)[nodeIndex] = 1;

	const fastgltf::TRS &trs = std::get<fastgltf::TRS>( asset.nodes[nodeIndex].transform );
	NAnimation::SBoneTransform bone;
	const CVec3 position = ConvertPosition( trs.translation );
	const CQuat rotation = ConvertRotation( trs.rotation );
	const CVec3 scale = ConvertScale( trs.scale );
	memcpy( bone.Position, &position, sizeof(bone.Position) );
	memcpy( bone.Orientation, &rotation, sizeof(bone.Orientation) );
	bone.ScaleShear[0][0] = scale.x;
	bone.ScaleShear[1][1] = scale.y;
	bone.ScaleShear[2][2] = scale.z;
	SHMatrix local = MakeLocalMatrix( bone );

	const int parent = parents[nodeIndex];
	(*pWorld)[nodeIndex] = parent >= 0
		? ResolveNodeWorld( asset, parents, parent, pWorld, pState ) * local
		: local;
	(*pState)[nodeIndex] = 2;
	return (*pWorld)[nodeIndex];
}
}

CGltfFile::CGltfFile( fastgltf::Asset &&_asset, const NFile::CFilePath &_sourcePath ) :
	asset( std::move(_asset) ),
	sourcePath( _sourcePath ),
	nodeParents( asset.nodes.size(), -1 ),
	nodeWorldTransforms( asset.nodes.size() )
{
	for ( std::size_t parent = 0; parent < asset.nodes.size(); ++parent )
		for ( std::size_t child : asset.nodes[parent].children )
			if ( child < nodeParents.size() )
				nodeParents[child] = static_cast<int>(parent);

	std::vector<unsigned char> state( asset.nodes.size(), 0 );
	for ( std::size_t i = 0; i < asset.nodes.size(); ++i )
		ResolveNodeWorld( asset, nodeParents, i, &nodeWorldTransforms, &state );

	std::vector<bool> visited( asset.nodes.size(), false );
	if ( asset.defaultScene.has_value() && *asset.defaultScene < asset.scenes.size() )
	{
		for ( std::size_t root : asset.scenes[*asset.defaultScene].nodeIndices )
			CollectMeshNodes( asset, root, &visited, &meshNodes );
	}
	else if ( !asset.scenes.empty() )
	{
		for ( std::size_t root : asset.scenes[0].nodeIndices )
			CollectMeshNodes( asset, root, &visited, &meshNodes );
	}
	else
	{
		for ( std::size_t i = 0; i < asset.nodes.size(); ++i )
			if ( asset.nodes[i].meshIndex.has_value() )
				meshNodes.push_back( i );
	}
}

int SSkeletonDefinition::FindBone( const std::string &name ) const
{
	const auto it = boneByName.find( name );
	return it == boneByName.end() ? -1 : it->second;
}

TGltfFilePtr LoadFile( const NFile::CFilePath &path )
{
	if ( path.empty() )
		return TGltfFilePtr();
	{
		std::lock_guard<std::mutex> lock( fileCacheMutex );
		const auto it = fileCache.find( path );
		if ( it != fileCache.end() )
			return it->second;
	}

	std::vector<std::byte> bytes;
	if ( !ReadVfsFile( path, &bytes ) )
	{
		DebugTrace( "glTF: could not open %s through the main VFS", path.c_str() );
		return TGltfFilePtr();
	}

	auto dataResult = fastgltf::GltfDataBuffer::FromBytes( bytes.data(), bytes.size() );
	if ( !dataResult )
	{
		DebugTrace( "glTF: invalid input buffer for %s (error %llu)", path.c_str(),
			static_cast<unsigned long long>(dataResult.error()) );
		return TGltfFilePtr();
	}

	fastgltf::Parser parser;
	const fastgltf::Options options =
		fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::GenerateMeshIndices;
	const fastgltf::Category categories =
		fastgltf::Category::Asset | fastgltf::Category::Buffers |
		fastgltf::Category::BufferViews | fastgltf::Category::Accessors |
		fastgltf::Category::Animations | fastgltf::Category::Meshes |
		fastgltf::Category::Skins | fastgltf::Category::Nodes |
		fastgltf::Category::Scenes;
	fastgltf::GltfDataBuffer data = std::move( dataResult.get() );
	auto assetResult = parser.loadGltf( data, std::filesystem::path(), options, categories );
	if ( !assetResult )
	{
		DebugTrace( "glTF: failed to parse %s (error %llu)", path.c_str(),
			static_cast<unsigned long long>(assetResult.error()) );
		return TGltfFilePtr();
	}

	fastgltf::Asset asset = std::move( assetResult.get() );
	const std::string basePath = NFile::GetFilePath( path );
	for ( fastgltf::Buffer &buffer : asset.buffers )
	{
		fastgltf::sources::URI *pUri = std::get_if<fastgltf::sources::URI>( &buffer.data );
		if ( !pUri )
			continue;
		if ( !pUri->uri.isLocalPath() )
		{
			DebugTrace( "glTF: unsupported non-local buffer URI in %s", path.c_str() );
			return TGltfFilePtr();
		}
		std::vector<std::byte> externalBytes;
		const std::string bufferPath = NFile::JoinPath( basePath, std::string(pUri->uri.path()) );
		if ( !ReadVfsFile( bufferPath, &externalBytes ) || pUri->fileByteOffset > externalBytes.size() )
		{
			DebugTrace( "glTF: could not open buffer %s through the main VFS", bufferPath.c_str() );
			return TGltfFilePtr();
		}
		if ( pUri->fileByteOffset != 0 )
			externalBytes.erase( externalBytes.begin(), externalBytes.begin() + pUri->fileByteOffset );
		buffer.data = fastgltf::sources::Vector{ std::move(externalBytes), pUri->mimeType };
	}

	TGltfFilePtr result = std::make_shared<CGltfFile>( std::move(asset), path );
	{
		std::lock_guard<std::mutex> lock( fileCacheMutex );
		fileCache[path] = result;
	}
	return result;
}

int GetMeshCount( const NFile::CFilePath &path )
{
	const TGltfFilePtr file = LoadFile( path );
	return file ? static_cast<int>(file->meshNodes.size()) : 0;
}

CVec3 ConvertPosition( const fastgltf::math::fvec3 &value )
{
	// glTF: right-handed, Y up. Engine: left-handed, Z up.
	return CVec3( value[0], value[2], value[1] );
}

CVec3 ConvertDirection( const fastgltf::math::fvec3 &value )
{
	return ConvertPosition( value );
}

CQuat ConvertRotation( const fastgltf::math::fquat &value )
{
	CQuat result;
	result.FromComponents( -value[0], -value[2], -value[1], value[3] );
	result.Normalize();
	return result;
}

CVec3 ConvertScale( const fastgltf::math::fvec3 &value )
{
	return CVec3( value[0], value[2], value[1] );
}

SHMatrix ConvertMatrix( const fastgltf::math::fmat4x4 &value )
{
	SHMatrix source;
	source.Set(
		value[0][0], value[1][0], value[2][0], value[3][0],
		value[0][1], value[1][1], value[2][1], value[3][1],
		value[0][2], value[1][2], value[2][2], value[3][2],
		value[0][3], value[1][3], value[2][3], value[3][3] );
	SHMatrix basis = IdentityMatrix();
	basis._22 = basis._33 = 0.0f;
	basis._23 = basis._32 = 1.0f;
	return basis * source * basis;
}

SHMatrix MakeLocalMatrix( const NAnimation::SBoneTransform &value )
{
	CVec3 position( value.Position[0], value.Position[1], value.Position[2] );
	CQuat rotation;
	rotation.FromComponents( value.Orientation[0], value.Orientation[1],
		value.Orientation[2], value.Orientation[3] );
	SHMatrix result( position, rotation );
	MultiplyScale( &result, result, value.ScaleShear[0][0],
		value.ScaleShear[1][1], value.ScaleShear[2][2] );
	return result;
}

NAnimation::SBoneTransform MakeBoneTransform( const SHMatrix &value )
{
	NAnimation::SBoneTransform result;
	const CVec3 position = value.GetTranslation();
	memcpy( result.Position, &position, sizeof(result.Position) );

	const float sx = sqrtf( value._11 * value._11 + value._21 * value._21 + value._31 * value._31 );
	const float sy = sqrtf( value._12 * value._12 + value._22 * value._22 + value._32 * value._32 );
	const float sz = sqrtf( value._13 * value._13 + value._23 * value._23 + value._33 * value._33 );
	result.ScaleShear[0][0] = sx;
	result.ScaleShear[1][1] = sy;
	result.ScaleShear[2][2] = sz;

	SHMatrix rotation = value;
	rotation._14 = rotation._24 = rotation._34 = 0.0f;
	rotation._41 = rotation._42 = rotation._43 = 0.0f;
	rotation._44 = 1.0f;
	if ( sx > FP_EPSILON )
		rotation._11 /= sx, rotation._21 /= sx, rotation._31 /= sx;
	if ( sy > FP_EPSILON )
		rotation._12 /= sy, rotation._22 /= sy, rotation._32 /= sy;
	if ( sz > FP_EPSILON )
		rotation._13 /= sz, rotation._23 /= sz, rotation._33 /= sz;
	CQuat quaternion;
	quaternion.FromEulerMatrix( rotation );
	quaternion.Normalize();
	memcpy( result.Orientation, &quaternion, sizeof(result.Orientation) );
	return result;
}

bool BuildSkeleton( const TGltfFilePtr &file, int nSkin, SSkeletonDefinition *pResult )
{
	if ( !file || !pResult || nSkin < 0 || nSkin >= static_cast<int>(file->asset.skins.size()) )
		return false;
	const fastgltf::Skin &skin = file->asset.skins[nSkin];
	if ( skin.joints.empty() || skin.joints.size() > 256 )
	{
		DebugTrace( "glTF: skin %d in %s has an unsupported joint count", nSkin, file->sourcePath.c_str() );
		return false;
	}

	pResult->boneNames.clear();
	pResult->parents.assign( skin.joints.size(), -1 );
	pResult->restPose.resize( skin.joints.size() );
	pResult->inverseBindMatrices.resize( skin.joints.size() );
	pResult->boneByName.clear();

	std::unordered_map<std::size_t, int> boneByNode;
	for ( std::size_t i = 0; i < skin.joints.size(); ++i )
		boneByNode[skin.joints[i]] = static_cast<int>(i);

	for ( std::size_t i = 0; i < skin.joints.size(); ++i )
	{
		const std::size_t nodeIndex = skin.joints[i];
		if ( nodeIndex >= file->asset.nodes.size() )
			return false;
		const fastgltf::Node &node = file->asset.nodes[nodeIndex];
		std::string name = node.name.empty() ? "Node_" + std::to_string(nodeIndex) : std::string(node.name);
		if ( pResult->boneByName.find(name) != pResult->boneByName.end() )
		{
			DebugTrace( "glTF: duplicate joint name %s in %s", name.c_str(), file->sourcePath.c_str() );
			return false;
		}
		pResult->boneNames.push_back( name );
		pResult->boneByName[name] = static_cast<int>(i);

		int parentNode = file->nodeParents[nodeIndex];
		while ( parentNode >= 0 )
		{
			const auto found = boneByNode.find( static_cast<std::size_t>(parentNode) );
			if ( found != boneByNode.end() )
			{
				pResult->parents[i] = found->second;
				break;
			}
			parentNode = file->nodeParents[parentNode];
		}

		SHMatrix local = file->nodeWorldTransforms[nodeIndex];
		if ( pResult->parents[i] >= 0 )
		{
			const std::size_t parentJoint = skin.joints[pResult->parents[i]];
			SHMatrix inverseParent;
			inverseParent.HomogeneousInverse( file->nodeWorldTransforms[parentJoint] );
			local = inverseParent * local;
		}
		pResult->restPose[i] = MakeBoneTransform( local );

		SHMatrix inverse;
		inverse.HomogeneousInverse( file->nodeWorldTransforms[nodeIndex] );
		pResult->inverseBindMatrices[i] = inverse;
	}

	if ( skin.inverseBindMatrices.has_value() &&
		*skin.inverseBindMatrices < file->asset.accessors.size() )
	{
		const fastgltf::Accessor &accessor = file->asset.accessors[*skin.inverseBindMatrices];
		std::size_t index = 0;
		for ( const fastgltf::math::fmat4x4 &matrix :
			fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(file->asset, accessor) )
		{
			if ( index >= pResult->inverseBindMatrices.size() )
				break;
			pResult->inverseBindMatrices[index++] = ConvertMatrix( matrix );
		}
	}
	return true;
}

}
