#include "stdafx.h"

#include "GltfFormat.h"

#include "System/DB.h"
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

void CollectNodeHierarchy( const fastgltf::Asset &asset, std::size_t nodeIndex,
	std::vector<bool> *pVisited, std::vector<std::size_t> *pResult )
{
	if ( nodeIndex >= asset.nodes.size() || (*pVisited)[nodeIndex] )
		return;
	(*pVisited)[nodeIndex] = true;
	pResult->push_back( nodeIndex );
	for ( std::size_t child : asset.nodes[nodeIndex].children )
		CollectNodeHierarchy( asset, child, pVisited, pResult );
}

bool FindUniqueNode( const TGltfFilePtr &file, const std::string &nodeName,
	std::size_t *pResult )
{
	bool found = false;
	for ( std::size_t i = 0; i < file->asset.nodes.size(); ++i )
	{
		if ( std::string(file->asset.nodes[i].name) != nodeName )
			continue;
		if ( found )
		{
			DebugTrace( "glTF: node selector %s is ambiguous in %s", nodeName.c_str(),
				file->sourcePath.c_str() );
			return false;
		}
		found = true;
		*pResult = i;
	}
	if ( !found )
		DebugTrace( "glTF: node selector %s was not found in %s", nodeName.c_str(),
			file->sourcePath.c_str() );
	return found;
}

void CollectSkinIndices( const fastgltf::Asset &asset, std::size_t nodeIndex,
	std::vector<bool> *pVisited, std::unordered_set<std::size_t> *pResult )
{
	if ( nodeIndex >= asset.nodes.size() || (*pVisited)[nodeIndex] )
		return;
	(*pVisited)[nodeIndex] = true;
	const fastgltf::Node &node = asset.nodes[nodeIndex];
	if ( node.skinIndex.has_value() && *node.skinIndex < asset.skins.size() )
		pResult->insert( *node.skinIndex );
	for ( std::size_t child : node.children )
		CollectSkinIndices( asset, child, pVisited, pResult );
}

bool IsAncestorOf( const CGltfFile &file, std::size_t ancestor, std::size_t nodeIndex )
{
	int current = static_cast<int>(nodeIndex);
	while ( current >= 0 && current < static_cast<int>(file.nodeParents.size()) )
	{
		if ( static_cast<std::size_t>(current) == ancestor )
			return true;
		current = file.nodeParents[current];
	}
	return false;
}

bool ResolveSkinIndex( const TGltfFilePtr &file, const std::string &rootNodeName,
	int nFallbackSkin, int *pResult )
{
	if ( rootNodeName.empty() )
	{
		*pResult = nFallbackSkin;
		return true;
	}

	std::size_t rootNode = 0;
	if ( !FindUniqueNode(file, rootNodeName, &rootNode) )
		return false;
	std::unordered_set<std::size_t> candidates;
	std::vector<bool> visited( file->asset.nodes.size(), false );
	// RootJoint may name either the armature/joint hierarchy or its skinned mesh.
	CollectSkinIndices( file->asset, rootNode, &visited, &candidates );
	for ( std::size_t skinIndex = 0; skinIndex < file->asset.skins.size(); ++skinIndex )
	{
		const fastgltf::Skin &skin = file->asset.skins[skinIndex];
		if ( skin.skeleton.has_value() && *skin.skeleton == rootNode )
			candidates.insert( skinIndex );
		for ( std::size_t joint : skin.joints )
		{
			if ( IsAncestorOf(*file, rootNode, joint) )
			{
				candidates.insert( skinIndex );
				break;
			}
		}
	}
	if ( candidates.size() != 1 )
	{
		DebugTrace( "glTF: RootJoint %s resolves to %d skins in %s; exactly one is required",
			rootNodeName.c_str(), static_cast<int>(candidates.size()), file->sourcePath.c_str() );
		return false;
	}
	*pResult = static_cast<int>(*candidates.begin());
	return true;
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

NFile::CFilePath ResolveModelFilePath( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef )
{
	if ( modelFileRef.empty() )
		return NFile::CFilePath();

	NFile::CFilePath normalizedRef;
	NFile::NormalizePath( &normalizedRef, modelFileRef );
	NVFS::IVFS *pVFS = NVFS::GetMainVFS();
	if ( !pOwner || !pVFS )
		return normalizedRef;

	const bool bAbsolute = NFile::IsFolderSeparator(normalizedRef[0]) ||
		(normalizedRef.size() > 2 && normalizedRef[1] == ':' &&
			NFile::IsFolderSeparator(normalizedRef[2]));
	const std::string ownerFolder = NFile::GetFilePath( NDb::GetFileName(pOwner->GetDBID()) );
	const int nOwnerFolderLength = static_cast<int>(ownerFolder.size());
	const bool bAlreadyInOwnerFolder = !ownerFolder.empty() &&
		normalizedRef.size() >= ownerFolder.size() &&
		NFile::ComparePathEq( 0, nOwnerFolderLength, ownerFolder,
			0, nOwnerFolderLength, normalizedRef );
	if ( !bAbsolute && !ownerFolder.empty() && !bAlreadyInOwnerFolder )
	{
		NFile::CFilePath localPath = NFile::JoinPath( ownerFolder, normalizedRef );
		NFile::NormalizePath( &localPath );
		if ( pVFS->DoesFileExist(localPath) )
			return localPath;
	}

	// The VFS also passes Windows drive-qualified paths through to the OS.
	return normalizedRef;
}

bool DoesModelFileExist( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef )
{
	NVFS::IVFS *pVFS = NVFS::GetMainVFS();
	return pVFS && pVFS->DoesFileExist( ResolveModelFilePath(pOwner, modelFileRef) );
}

TGltfFilePtr LoadFile( const NDb::CResource *pOwner,
	const NFile::CFilePath &modelFileRef )
{
	const NFile::CFilePath path = ResolveModelFilePath( pOwner, modelFileRef );
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

bool GetMeshNodes( const TGltfFilePtr &file, const std::string &rootNodeName,
	std::vector<std::size_t> *pResult )
{
	if ( !file || !pResult )
		return false;
	pResult->clear();
	if ( rootNodeName.empty() )
	{
		*pResult = file->meshNodes;
		return true;
	}

	std::size_t rootNode = 0;
	if ( !FindUniqueNode(file, rootNodeName, &rootNode) )
		return false;
	std::vector<bool> visited( file->asset.nodes.size(), false );
	CollectMeshNodes( file->asset, rootNode, &visited, pResult );
	if ( pResult->empty() )
	{
		DebugTrace( "glTF: RootMesh %s has no mesh-bearing nodes in %s",
			rootNodeName.c_str(), file->sourcePath.c_str() );
		return false;
	}
	return true;
}

bool GetMeshBoundingBox( const TGltfFilePtr &file, const std::string &rootNodeName,
	bool bApplyNodeTransformsToSkinnedMeshes, CVec3 *pMin, CVec3 *pMax )
{
	if ( !file || !pMin || !pMax )
		return false;

	std::vector<std::size_t> meshNodes;
	if ( !GetMeshNodes(file, rootNodeName, &meshNodes) )
		return false;

	bool bHaveVertex = false;
	for ( std::size_t nodeIndex : meshNodes )
	{
		if ( nodeIndex >= file->asset.nodes.size() )
			continue;
		const fastgltf::Node &node = file->asset.nodes[nodeIndex];
		if ( !node.meshIndex.has_value() || *node.meshIndex >= file->asset.meshes.size() )
			continue;

		const bool bApplyNodeTransform = bApplyNodeTransformsToSkinnedMeshes || !node.skinIndex.has_value();
		for ( const fastgltf::Primitive &primitive : file->asset.meshes[*node.meshIndex].primitives )
		{
			// These are the primitive types consumed by both GLB geometry loaders.
			if ( primitive.type != fastgltf::PrimitiveType::Triangles &&
				primitive.type != fastgltf::PrimitiveType::TriangleStrip &&
				primitive.type != fastgltf::PrimitiveType::TriangleFan )
				continue;
			const auto position = primitive.findAttribute( "POSITION" );
			if ( position == primitive.attributes.end() || position->accessorIndex >= file->asset.accessors.size() )
				continue;
			const fastgltf::Accessor &positionAccessor = file->asset.accessors[position->accessorIndex];

			for ( const fastgltf::math::fvec3 &source :
				fastgltf::iterateAccessor<fastgltf::math::fvec3>(file->asset, positionAccessor) )
			{
				CVec3 point = ConvertPosition( source );
				if ( bApplyNodeTransform )
				{
					CVec3 transformed;
					file->nodeWorldTransforms[nodeIndex].RotateHVector( &transformed, point );
					point = transformed;
				}

				if ( !bHaveVertex )
				{
					*pMin = *pMax = point;
					bHaveVertex = true;
				}
				else
				{
					if ( point.x < pMin->x ) pMin->x = point.x;
					if ( point.y < pMin->y ) pMin->y = point.y;
					if ( point.z < pMin->z ) pMin->z = point.z;
					if ( point.x > pMax->x ) pMax->x = point.x;
					if ( point.y > pMax->y ) pMax->y = point.y;
					if ( point.z > pMax->z ) pMax->z = point.z;
				}
			}
		}
	}

	if ( !bHaveVertex )
		DebugTrace( "glTF: selected meshes have no supported POSITION vertices in %s",
			file->sourcePath.c_str() );
	return bHaveVertex;
}

int GetMeshCount( const NDb::CResource *pOwner, const NFile::CFilePath &modelFileRef,
	const std::string &rootNodeName )
{
	const TGltfFilePtr file = LoadFile( pOwner, modelFileRef );
	std::vector<std::size_t> meshNodes;
	return GetMeshNodes(file, rootNodeName, &meshNodes)
		? static_cast<int>(meshNodes.size()) : 0;
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

namespace
{
bool BuildNodeSkeleton( const TGltfFilePtr &file, const std::string &rootNodeName,
	SSkeletonDefinition *pResult )
{
	if ( !file || !pResult )
		return false;
	if ( rootNodeName.empty() )
	{
		DebugTrace( "glTF: RootJoint is required for a node-animated file without skins: %s",
			file->sourcePath.c_str() );
		return false;
	}

	std::size_t rootNode = 0;
	if ( !FindUniqueNode(file, rootNodeName, &rootNode) )
		return false;
	std::vector<std::size_t> nodes;
	std::vector<bool> visited( file->asset.nodes.size(), false );
	CollectNodeHierarchy( file->asset, rootNode, &visited, &nodes );
	if ( nodes.empty() || nodes.size() > 256 )
	{
		DebugTrace( "glTF: RootJoint %s in %s has an unsupported node count (%d)",
			rootNodeName.c_str(), file->sourcePath.c_str(), static_cast<int>(nodes.size()) );
		return false;
	}

	pResult->boneNames.clear();
	pResult->parents.assign( nodes.size(), -1 );
	pResult->restPose.resize( nodes.size() );
	pResult->inverseBindMatrices.resize( nodes.size() );
	pResult->boneByName.clear();

	std::unordered_map<std::size_t, int> boneByNode;
	for ( std::size_t i = 0; i < nodes.size(); ++i )
		boneByNode[nodes[i]] = static_cast<int>(i);

	for ( std::size_t i = 0; i < nodes.size(); ++i )
	{
		const std::size_t nodeIndex = nodes[i];
		const fastgltf::Node &node = file->asset.nodes[nodeIndex];
		const std::string name = node.name.empty()
			? "Node_" + std::to_string(nodeIndex) : std::string(node.name);
		if ( pResult->boneByName.find(name) != pResult->boneByName.end() )
		{
			DebugTrace( "glTF: duplicate node name %s in rigid hierarchy %s",
				name.c_str(), file->sourcePath.c_str() );
			return false;
		}
		pResult->boneNames.push_back( name );
		pResult->boneByName[name] = static_cast<int>(i);

		const int parentNode = file->nodeParents[nodeIndex];
		if ( parentNode >= 0 )
		{
			const auto found = boneByNode.find( static_cast<std::size_t>(parentNode) );
			if ( found != boneByNode.end() )
				pResult->parents[i] = found->second;
		}

		SHMatrix local = file->nodeWorldTransforms[nodeIndex];
		if ( pResult->parents[i] >= 0 )
		{
			const std::size_t parentIndex = nodes[pResult->parents[i]];
			SHMatrix inverseParent;
			inverseParent.HomogeneousInverse( file->nodeWorldTransforms[parentIndex] );
			local = inverseParent * local;
		}
		pResult->restPose[i] = MakeBoneTransform( local );

		// Rigid mesh vertices remain baked in model space, so the inverse rest
		// transform makes the synthetic bone's bind-pose composite an identity.
		SHMatrix inverse;
		inverse.HomogeneousInverse( file->nodeWorldTransforms[nodeIndex] );
		pResult->inverseBindMatrices[i] = inverse;
	}
	return true;
}
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

bool BuildSkeleton( const TGltfFilePtr &file, const std::string &rootNodeName,
	int nFallbackSkin, SSkeletonDefinition *pResult )
{
	if ( !file )
		return false;
	// Skinless mechanical models use their node hierarchy as a rigid skeleton.
	// Requiring RootJoint avoids guessing between unrelated scene roots.
	if ( file->asset.skins.empty() )
		return BuildNodeSkeleton( file, rootNodeName, pResult );
	int skinIndex = nFallbackSkin;
	if ( !ResolveSkinIndex(file, rootNodeName, nFallbackSkin, &skinIndex) )
		return false;
	return BuildSkeleton( file, skinIndex, pResult );
}

}
