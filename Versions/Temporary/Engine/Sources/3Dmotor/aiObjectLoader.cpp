#include "stdafx.h"
#include "aiObjectLoader.h"
#include "3DLib/MemObject.h"
#include "System/BasicShare.h"
//#include "GFileSkin.h"
//#include "PrecalcSpheres.h"
#include "aiGeometryFormat.h"
#include "GAnimFormat.h"
#include "vendor/granny/include/granny.h"
#include "DBScene.h"

namespace NAI
{
	static CBasicShare<CDBPtr<NDb::SAIGeometry>, NAnimation::CGrannyAIGeomLoader, SDBPtrHash> shareGrannyAIGeometries(116);

static CVec3 CalcMassCenter( vector<SMassSphere> &spheres )
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

	vector<CVec3> points;
	vector<STriangle> tris;
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
		pValue->AddPiece( 0, points, tris, 0, vector<SJunction>(), true );//, precalc );
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
	if ( pData->GetValue() && pData->GetValue()->GetData() )
	{
		granny_file_info *pFI = pData->GetValue()->GetData();
		for ( int i = 0; i < pFI->MeshCount; ++i )
		{
			vector<CVec3> points;
			vector<STriangle> tris;	
			granny_mesh *pMesh = pFI->Meshes[i];
			NGScene::ConvertAIGeomVerticesFromGranny( pMesh, &points );
			NGScene::ConvertAIGeomTrisFromGranny( pMesh, &tris );
			pValue->AddPiece( i, points, tris, 0, vector<SJunction>(), true );
		}
		pValue->CalcBound();
	}
}

void CLoadAIGeometryFromGranny::SetKey( const NDb::SAIGeometry *pGeometry )
{
	pData = shareGrannyAIGeometries.Get( pGeometry );
}

// CMemGeometryInfo

struct SMergePoints
{
	vector<CVec3> points;
	vector<STriangle> tris;

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
	const vector<CVec3> &points = pMemObject->GetPoints();
	const vector<STriangle> &tris = pMemObject->GetTris();
	for ( int k = 0; k < tris.size(); ++k )
		p.AddTriangle( points[tris[k].i1], points[tris[k].i2], points[tris[k].i3] );
	pValue->AddPiece( -1, p.points, p.tris, 0 );
	pValue->CalcBound();
}

// CFileSkinPointsLoad

struct SStoredSkin
{
	vector<CVec3> points;
	vector<STriangle> tris;
	vector<NGScene::SLoadVertexWeight> weights;

	int operator&( CStructureSaver &f )
	{ 
		f.Add( 1, &points );
		f.Add( 2, &tris );
		f.Add( 3, &weights );
		return 0;
	}
};
typedef hash_map<int, SStoredSkin> CBodypartsStoredHash;

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
	if ( pData->GetValue() && pData->GetValue()->GetData() )
	{
		granny_file_info *pFI = pData->GetValue()->GetData();
		for ( int i = 0; i < pFI->MeshCount; ++i )
		{
			granny_mesh *pMesh = pFI->Meshes[i];
			vector<CVec3> &pts = pValue->parts[i].points;
			NGScene::ConvertAIGeomVerticesFromGranny( pMesh, &pts );
			vector<STriangle> tris;	
			NGScene::ConvertAIGeomTrisFromGranny( pMesh, &tris );
			pValue->parts[i].edges.GenerateEdgeList( tris, pts );
			granny_skeleton *pSkeleton = NGScene::FindFirstAppropriateModel( pFI, pMesh )->Skeleton;
			NGScene::ConvertWeightsFromGrannyEx( pSkeleton, pMesh, 0, &pValue->parts[i].weights, pts.size() );
		}
	}
}

void CFileSkinPointsLoadFromGranny::SetKey( const NDb::SAIGeometry *pGeometry )
{
	pData = shareGrannyAIGeometries.Get( pGeometry );
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
			while ( pWeight->fWeights[j] && j < 4 )
			{
				blends[ pWeight->cBoneIndices[j] ].RotateHVector( &p, *pMesh );
				*pRes += pWeight->fWeights[j] * p;
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
REGISTER_SAVELOAD_CLASS( 0x012c1160, CLoadAIGeometryFromA5Exporter )
REGISTER_SAVELOAD_CLASS( 0x70493110, CLoadAIGeometryFromGranny )
REGISTER_SAVELOAD_CLASS( 0x012c1161, CMemGeometryInfo )
REGISTER_SAVELOAD_CLASS( 0x014c1110, CFileSkinPointsLoadFromA5Exporter )
REGISTER_SAVELOAD_CLASS( 0x70493111, CFileSkinPointsLoadFromGranny )
REGISTER_SAVELOAD_CLASS( 0x014c1111, CSkinner )
//REGISTER_SAVELOAD_CLASS( 0x73102120, CPrecalcFlipper )
//REGISTER_SAVELOAD_CLASS( 0x73102121, CLoadTwoBSPTrees )

