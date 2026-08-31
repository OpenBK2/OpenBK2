#include "stdafx.h"
#include "aiObjectLoader.h"
#include "3DLib/MemObject.h"
#include "System/BasicShare.h"
//#include "GFileSkin.h"
//#include "PrecalcSpheres.h"
#include "AIGeometryFormat.h"
#include "GAnimFormat.h"
#include "GltfFormat.h"
#include "vendor/granny/include/granny.h"
#include "DBScene.h"

#include <fastgltf/tools.hpp>

namespace NAI
{
	static CBasicShare<CDBPtr<NDb::SAIGeometry>, NAnimation::CGrannyAIGeomLoader, SDBPtrHash> shareGrannyAIGeometries(116);

std::size_t SAISkinKeyHash::operator()( const SAISkinKey &key ) const
{
	const std::size_t geometryHash = key.pGeometry ? key.pGeometry->GetDBID().GetHashKey() : 0;
	const std::size_t skeletonHash = key.skeletonH.pSkeleton ? key.skeletonH.pSkeleton->GetDBID().GetHashKey() : 0;
	return geometryHash ^ (skeletonHash << 1) ^ static_cast<std::size_t>(key.skeletonH.nModelInFile);
}

namespace
{
template <class TValue>
std::vector<TValue> ReadGltfAccessor( const fastgltf::Asset &asset, std::size_t accessorIndex )
{
	std::vector<TValue> result;
	if ( accessorIndex >= asset.accessors.size() )
		return result;
	const fastgltf::Accessor &accessor = asset.accessors[accessorIndex];
	result.reserve( accessor.count );
	for ( const TValue &value : fastgltf::iterateAccessor<TValue>(asset, accessor) )
		result.push_back( value );
	return result;
}

std::size_t FindAttribute( const fastgltf::Primitive &primitive, const char *pszName )
{
	const auto it = primitive.findAttribute( pszName );
	return it == primitive.attributes.end() ? static_cast<std::size_t>(-1) : it->accessorIndex;
}

void AppendTriangles( const fastgltf::Asset &asset, const fastgltf::Primitive &primitive,
	std::size_t vertexOffset, std::vector<STriangle> *pResult )
{
	if ( !primitive.indicesAccessor.has_value() )
		return;
	const std::vector<std::uint32_t> indices =
		ReadGltfAccessor<std::uint32_t>( asset, *primitive.indicesAccessor );
	if ( primitive.type == fastgltf::PrimitiveType::Triangles )
	{
		for ( std::size_t i = 0; i + 2 < indices.size(); i += 3 )
			pResult->push_back( STriangle(static_cast<int>(vertexOffset + indices[i]),
				static_cast<int>(vertexOffset + indices[i + 2]),
				static_cast<int>(vertexOffset + indices[i + 1])) );
	}
	else if ( primitive.type == fastgltf::PrimitiveType::TriangleStrip )
	{
		for ( std::size_t i = 2; i < indices.size(); ++i )
		{
			const std::uint32_t a = indices[i - 2];
			const std::uint32_t b = indices[i - 1];
			const std::uint32_t c = indices[i];
			if ( a == b || b == c || a == c )
				continue;
			// Strip triangles alternate their source winding; reverse both cases
			// after converting from glTF's handedness.
			pResult->push_back( STriangle(static_cast<int>(vertexOffset + (i & 1 ? b : a)),
				static_cast<int>(vertexOffset + c),
				static_cast<int>(vertexOffset + (i & 1 ? a : b))) );
		}
	}
	else if ( primitive.type == fastgltf::PrimitiveType::TriangleFan )
	{
		for ( std::size_t i = 2; i < indices.size(); ++i )
			pResult->push_back( STriangle(static_cast<int>(vertexOffset + indices[0]),
				static_cast<int>(vertexOffset + indices[i]),
				static_cast<int>(vertexOffset + indices[i - 1])) );
	}
}

bool BuildTargetBoneMap( const SAISkinKey &key,
	NAnimation::CGrannyFileInfo *pGrannySkeletonFile,
	std::unordered_map<std::string, int> *pResult )
{
	pResult->clear();
	if ( !key.skeletonH.pSkeleton )
		return false;
	if ( !key.skeletonH.pSkeleton->szModelFileRef.empty() )
	{
		const NGltf::TGltfFilePtr file = NGltf::LoadFile( key.skeletonH.pSkeleton->szModelFileRef );
		NGltf::SSkeletonDefinition skeleton;
		if ( !NGltf::BuildSkeleton(file, key.skeletonH.pSkeleton->szRootJoint,
			key.skeletonH.nModelInFile, &skeleton) )
			return false;
		*pResult = skeleton.boneByName;
		return true;
	}
	if ( !pGrannySkeletonFile )
		return false;
	const granny_skeleton *pSkeleton =
		NAnimation::GetSkeleton( pGrannySkeletonFile, key.skeletonH.nModelInFile );
	if ( !pSkeleton || pSkeleton->BoneCount > 256 )
		return false;
	for ( int i = 0; i < pSkeleton->BoneCount; ++i )
		(*pResult)[pSkeleton->Bones[i].Name] = i;
	return true;
}

void AppendGltfAIPrimitive( const NGltf::TGltfFilePtr &file, std::size_t nodeIndex,
	const fastgltf::Primitive &primitive, bool bAnimated,
	const std::unordered_map<std::string, int> *pTargetBones,
	std::vector<CVec3> *pPoints, std::vector<STriangle> *pTriangles,
	std::vector<NGScene::SVertexWeight> *pWeights )
{
	if ( primitive.type != fastgltf::PrimitiveType::Triangles &&
		primitive.type != fastgltf::PrimitiveType::TriangleStrip &&
		primitive.type != fastgltf::PrimitiveType::TriangleFan )
		return;
	const std::size_t positionAccessor = FindAttribute( primitive, "POSITION" );
	if ( positionAccessor == static_cast<std::size_t>(-1) )
		return;

	const fastgltf::Asset &asset = file->asset;
	const fastgltf::Node &node = asset.nodes[nodeIndex];
	const std::vector<fastgltf::math::fvec3> positions =
		ReadGltfAccessor<fastgltf::math::fvec3>( asset, positionAccessor );
	const std::size_t vertexOffset = pPoints->size();
	pPoints->reserve( vertexOffset + positions.size() );
	for ( const fastgltf::math::fvec3 &source : positions )
	{
		CVec3 point = NGltf::ConvertPosition( source );
		if ( !bAnimated || !node.skinIndex.has_value() )
		{
			CVec3 transformed;
			file->nodeWorldTransforms[nodeIndex].RotateHVector( &transformed, point );
			point = transformed;
		}
		pPoints->push_back( point );
	}
	AppendTriangles( asset, primitive, vertexOffset, pTriangles );

	if ( !pWeights )
		return;
	pWeights->resize( pPoints->size() );
	for ( std::size_t i = 0; i < positions.size(); ++i )
	{
		NGScene::SVertexWeight &target = (*pWeights)[vertexOffset + i];
		memset( &target, 0, sizeof(target) );
		// An unskinned or incompletely exported vertex follows the root bone.
		target.fWeights[0] = 1.0f;
	}
	if ( !node.skinIndex.has_value() || *node.skinIndex >= asset.skins.size() || !pTargetBones )
		return;
	const std::size_t jointsAccessor = FindAttribute( primitive, "JOINTS_0" );
	const std::size_t weightsAccessor = FindAttribute( primitive, "WEIGHTS_0" );
	if ( jointsAccessor == static_cast<std::size_t>(-1) || weightsAccessor == static_cast<std::size_t>(-1) )
		return;

	const fastgltf::Skin &sourceSkin = asset.skins[*node.skinIndex];
	const std::vector<fastgltf::math::uvec4> joints =
		ReadGltfAccessor<fastgltf::math::uvec4>( asset, jointsAccessor );
	const std::vector<fastgltf::math::fvec4> weights =
		ReadGltfAccessor<fastgltf::math::fvec4>( asset, weightsAccessor );
	for ( std::size_t i = 0; i < positions.size() && i < joints.size() && i < weights.size(); ++i )
	{
		NGScene::SVertexWeight &target = (*pWeights)[vertexOffset + i];
		memset( &target, 0, sizeof(target) );
		int nTargetInfluence = 0;
		float fTotalWeight = 0.0f;
		for ( int influence = 0; influence < 4 && nTargetInfluence < 4; ++influence )
		{
			if ( weights[i][influence] <= 0.0f )
				continue;
			const std::size_t joint = joints[i][influence];
			if ( joint >= sourceSkin.joints.size() )
				continue;
			const std::size_t sourceNodeIndex = sourceSkin.joints[joint];
			if ( sourceNodeIndex >= asset.nodes.size() )
				continue;
			const fastgltf::Node &sourceNode = asset.nodes[sourceNodeIndex];
			const std::string name = sourceNode.name.empty()
				? "Node_" + std::to_string(sourceNodeIndex) : std::string(sourceNode.name);
			const auto found = pTargetBones->find( name );
			if ( found == pTargetBones->end() || found->second < 0 || found->second > 255 )
				continue;
			target.cBoneIndices[nTargetInfluence] = static_cast<uint8_t>(found->second);
			target.fWeights[nTargetInfluence] = weights[i][influence];
			fTotalWeight += target.fWeights[nTargetInfluence++];
		}
		if ( fTotalWeight > FP_EPSILON )
			for ( int influence = 0; influence < nTargetInfluence; ++influence )
				target.fWeights[influence] /= fTotalWeight;
		else
			target.fWeights[0] = 1.0f;
	}
}
}

static CVec3 CalcMassCenter( std::vector<SMassSphere> &spheres )
{
	float fMassSum = 0;
	CVec3 massCenter = VNULL3;
	for ( int i = 0; i < spheres.size(); ++i )
	{
		if ( spheres[i].fMass <= 0 )
			spheres[i].fMass = 1;
		fMassSum += spheres[i].fMass;
		massCenter += spheres[i].fMass * spheres[i].ptCenter;
	}
	massCenter /= fMassSum;
	return massCenter;
}

// CLoadAIGeometryFromA5Exporter

void CLoadAIGeometryFromA5Exporter::Recalc()
{
	pValue = new CGeometryInfo;

	NGScene::CResourceOpener file( "AIGeometries", GetKey() );
	if ( !file.IsOk() )
		return;

	std::vector<CVec3> points;
	std::vector<STriangle> tris;
	//vector<CPtr<CPrecalcSpheres> > precalc;

	CStoredPieceMap pieces;
	file->Add( 1, &points );
	file->Add( 2, &tris );
	file->Add( 4, &pieces );
	file->Add( 6, &pValue->spheres );
	//file->Add( 9, &precalc );

	if ( pieces.empty() )
	{
		//ASSERT( pPrecalc );
		pValue->AddPiece( 0, points, tris, 0, std::vector<SJunction>(), true );//, precalc );
	}
	else
	{
		for ( CStoredPieceMap::const_iterator i = pieces.begin(); i != pieces.end(); ++i )
		{
			//ASSERT( i->second.tris.empty() || ( !i->second.precalc.empty() ) );
			pValue->AddPiece( i->first, i->second.verts, i->second.tris, i->second.fVolume, i->second.juncs, true );//, i->second.precalc );
		}
	}
	pValue->CalcBound();
	pValue->massCenter = CalcMassCenter( pValue->spheres );
}

// CLoadAIGeometryFromGranny

void CLoadAIGeometryFromGranny::Recalc()
{
	pValue = new CGeometryInfo;
	if ( pGeometry && !pGeometry->szModelFileRef.empty() )
	{
		const NGltf::TGltfFilePtr file = NGltf::LoadFile( pGeometry->szModelFileRef );
		std::vector<std::size_t> meshNodes;
		if ( !NGltf::GetMeshNodes(file, pGeometry->szRootMesh, &meshNodes) )
			return;
		for ( std::size_t i = 0; i < meshNodes.size(); ++i )
		{
			const std::size_t nodeIndex = meshNodes[i];
			const fastgltf::Node &node = file->asset.nodes[nodeIndex];
			if ( !node.meshIndex.has_value() || *node.meshIndex >= file->asset.meshes.size() )
				continue;
			std::vector<CVec3> points;
			std::vector<STriangle> triangles;
			for ( const fastgltf::Primitive &primitive : file->asset.meshes[*node.meshIndex].primitives )
				AppendGltfAIPrimitive( file, nodeIndex, primitive, false, 0,
					&points, &triangles, 0 );
			if ( !points.empty() && !triangles.empty() )
				pValue->AddPiece( static_cast<int>(i), points, triangles, 0, std::vector<SJunction>(), true );
		}
		pValue->CalcBound();
		return;
	}
	if ( pData->GetValue() && pData->GetValue()->GetData() )
	{
		granny_file_info *pFI = pData->GetValue()->GetData();
		for ( int i = 0; i < pFI->MeshCount; ++i )
		{
			std::vector<CVec3> points;
			std::vector<STriangle> tris;
			granny_mesh *pMesh = pFI->Meshes[i];
			NGScene::ConvertAIGeomVerticesFromGranny( pMesh, &points );
			NGScene::ConvertAIGeomTrisFromGranny( pMesh, &tris );
			pValue->AddPiece( i, points, tris, 0, std::vector<SJunction>(), true );
		}
		pValue->CalcBound();
	}
}

void CLoadAIGeometryFromGranny::SetKey( const NDb::SAIGeometry *pGeometry )
{
	this->pGeometry = pGeometry;
	pData = pGeometry && pGeometry->szModelFileRef.empty()
		? shareGrannyAIGeometries.Get( pGeometry ) : 0;
}

bool CLoadAIGeometryFromGranny::NeedUpdate()
{
	bool bResult = TParent::NeedUpdate();
	if ( !pGeometry || pGeometry->szModelFileRef.empty() )
		bResult |= pData.Refresh();
	return bResult;
}

// CMemGeometryInfo

struct SMergePoints
{
	std::vector<CVec3> points;
	std::vector<STriangle> tris;

	int GetPointIndex( const CVec3 &a )
	{
		for ( int k = 0; k < points.size(); ++k )
		{
			if ( points[k] == a )
				return k;
		}
		points.push_back( a );
		return points.size() - 1;
	}
	void AddTriangle( const CVec3 &a, const CVec3 &b, const CVec3 &c )
	{
		tris.push_back( STriangle( GetPointIndex(a), GetPointIndex(b), GetPointIndex(c) ) );
	}
};

void CMemGeometryInfo::Recalc()
{
	pValue = new CGeometryInfo;
	SMergePoints p;
	const std::vector<CVec3> &points = pMemObject->GetPoints();
	const std::vector<STriangle> &tris = pMemObject->GetTris();
	for ( int k = 0; k < tris.size(); ++k )
		p.AddTriangle( points[tris[k].i1], points[tris[k].i2], points[tris[k].i3] );
	pValue->AddPiece( -1, p.points, p.tris, 0 );
	pValue->CalcBound();
}

// CFileSkinPointsLoad

struct SStoredSkin
{
	std::vector<CVec3> points;
	std::vector<STriangle> tris;
	std::vector<NGScene::SLoadVertexWeight> weights;

	int operator&( CStructureSaver &f )
	{ 
		f.Add( 1, &points );
		f.Add( 2, &tris );
		f.Add( 3, &weights );
		return 0;
	}
};
typedef std::unordered_map<int, SStoredSkin> CBodypartsStoredHash;

void CFileSkinPointsLoadFromA5Exporter::Recalc()
{
	pValue = new CFileSkinPoints;
	NGScene::CResourceOpener file( "AIGeometries", GetKey() );
	if ( !file.IsOk() )
		return;

	CBodypartsStoredHash data;
	file->Add( 4, &data );
	file->Add( 6, &pValue->spheres );
	pValue->massCenter = CalcMassCenter( pValue->spheres );
	for ( CBodypartsStoredHash::const_iterator i = data.begin(); i != data.end(); ++i )
	{
		const SStoredSkin &src = i->second;
		CFileSkinPoints::SBodypart &r = pValue->parts[i->first];
		r.points = src.points;
		r.edges.GenerateEdgeList( src.tris, src.points );
		NGScene::ConvertWeights( &r.weights, src.weights, src.points.size() );
	}
}

// CFileSkinPointsLoadFromGranny

void CFileSkinPointsLoadFromGranny::Recalc()
{
	pValue = new CFileSkinPoints;
	if ( key.pGeometry && !key.pGeometry->szModelFileRef.empty() )
	{
		const NGltf::TGltfFilePtr file = NGltf::LoadFile( key.pGeometry->szModelFileRef );
		std::vector<std::size_t> meshNodes;
		if ( !NGltf::GetMeshNodes(file, key.pGeometry->szRootMesh, &meshNodes) )
			return;
		std::unordered_map<std::string, int> targetBones;
		NAnimation::CGrannyFileInfo *pGrannySkeleton =
			pSkeletonData ? pSkeletonData->GetValue() : 0;
		const bool bHaveTargetBones = BuildTargetBoneMap( key, pGrannySkeleton, &targetBones );
		for ( std::size_t i = 0; i < meshNodes.size(); ++i )
		{
			const std::size_t nodeIndex = meshNodes[i];
			const fastgltf::Node &node = file->asset.nodes[nodeIndex];
			if ( !node.meshIndex.has_value() || *node.meshIndex >= file->asset.meshes.size() )
				continue;
			CFileSkinPoints::SBodypart &part = pValue->parts[static_cast<int>(i)];
			std::vector<STriangle> triangles;
			for ( const fastgltf::Primitive &primitive : file->asset.meshes[*node.meshIndex].primitives )
				AppendGltfAIPrimitive( file, nodeIndex, primitive, true,
					bHaveTargetBones ? &targetBones : 0, &part.points, &triangles, &part.weights );
			if ( part.points.empty() || triangles.empty() )
				pValue->parts.erase( static_cast<int>(i) );
			else
				part.edges.GenerateEdgeList( triangles, part.points );
		}
		pValue->massCenter = VNULL3;
		return;
	}
	if ( pData->GetValue() && pData->GetValue()->GetData() )
	{
		granny_file_info *pFI = pData->GetValue()->GetData();
		for ( int i = 0; i < pFI->MeshCount; ++i )
		{
			granny_mesh *pMesh = pFI->Meshes[i];
			std::vector<CVec3> &pts = pValue->parts[i].points;
			NGScene::ConvertAIGeomVerticesFromGranny( pMesh, &pts );
			std::vector<STriangle> tris;
			NGScene::ConvertAIGeomTrisFromGranny( pMesh, &tris );
			pValue->parts[i].edges.GenerateEdgeList( tris, pts );
			granny_skeleton *pSkeleton = NGScene::FindFirstAppropriateModel( pFI, pMesh )->Skeleton;
			NGScene::ConvertWeightsFromGrannyEx( pSkeleton, pMesh, 0, &pValue->parts[i].weights, pts.size() );
		}
	}
}

void CFileSkinPointsLoadFromGranny::SetKey( const SAISkinKey &_key )
{
	key = _key;
	pData = key.pGeometry && key.pGeometry->szModelFileRef.empty()
		? shareGrannyAIGeometries.Get( key.pGeometry ) : 0;
	pSkeletonData = key.skeletonH.pSkeleton && key.skeletonH.pSkeleton->szModelFileRef.empty()
		? NAnimation::GetSkeletonFileInfo( key.skeletonH.pSkeleton ) : 0;
}

void CFileSkinPointsLoadFromGranny::SetKey( const NDb::SAIGeometry *pGeometry )
{
	// GR2 stores the source skeleton together with the AI mesh, matching the
	// original cache behavior and serialized key layout.
	SetKey( SAISkinKey(pGeometry, NAnimation::SSkeletonHandle()) );
}

bool CFileSkinPointsLoadFromGranny::NeedUpdate()
{
	bool bResult = TParent::NeedUpdate();
	if ( !key.pGeometry || key.pGeometry->szModelFileRef.empty() )
		bResult |= pData.Refresh();
	if ( key.skeletonH.pSkeleton && key.skeletonH.pSkeleton->szModelFileRef.empty() )
		bResult |= pSkeletonData.Refresh();
	return bResult;
}

// CSkinner

void CSkinner::Recalc()
{
	const CFileSkinPoints &src = *pSkin->GetValue();
	if ( !IsValid( pValue ) )
	{
		pValue = new CGeometryInfo;
		for ( CFileSkinPoints::CBodypartsHash::const_iterator i = src.parts.begin(); i != src.parts.end(); ++i )
		{
			SPiece &dst = pValue->pieces[i->first];
			const CFileSkinPoints::SBodypart &src = i->second;
			dst.edges = src.edges;
			ASSERT( src.edges.IsClosed() );
			dst.points.resize( src.points.size() );
		}
	}
	//pValue->points.resize( nVertices );
	typedef CVec3 SVertex;
	using NGScene::SVertexWeight;
	
	for ( CFileSkinPoints::CBodypartsHash::const_iterator i = src.parts.begin(); i != src.parts.end(); ++i )
	{
		SPiece &dst = pValue->pieces[i->first];
		const CFileSkinPoints::SBodypart &src = i->second;
		ASSERT( src.points.size() == dst.points.size() );
		int nVertices = src.points.size();
		SVertex *pRes = &dst.points[0];
		const SVertex *pMesh = &src.points[0];
		const SVertexWeight *pWeight = &src.weights[0];
		const NGScene::SSkeletonMatrices &blends = pAnimation->GetValue();
		memset( pRes, 0, sizeof(SVertex) * nVertices );
		CVec3 p;
		for ( int i = 0; i < nVertices; ++i )
		{
			int j = 0;
			while ( j < 4 && pWeight->fWeights[j] )
			{
				const unsigned int boneIndex = pWeight->cBoneIndices[j];
				if ( boneIndex < blends.size() )
				{
					blends[boneIndex].RotateHVector( &p, *pMesh );
					*pRes += pWeight->fWeights[j] * p;
				}
				++j;
			}
			pRes++;
			pMesh++;
			pWeight++;
		}
	}
	pValue->CalcBound();	
	/*if ( bDoPrecalc )
		pValue->PrecalcCollideInfo();
	else if ( !setTrees.empty() )
	{
		pValue->SetCollideInfo( setTrees );
	}*/
}

} // namespace
using namespace NAI;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x012c1160, CLoadAIGeometryFromA5Exporter )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x70493110, CLoadAIGeometryFromGranny )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x012c1161, CMemGeometryInfo )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x014c1110, CFileSkinPointsLoadFromA5Exporter )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x70493111, CFileSkinPointsLoadFromGranny )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x014c1111, CSkinner )
//REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x73102120, CPrecalcFlipper )
//REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x73102121, CLoadTwoBSPTrees )

