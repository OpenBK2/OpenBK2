#include "stdafx.h"

#include "3DLib_export.h"

#include "GGeometry.h"
#include "Bound.h"
//#include "..\Misc\StrProc.h"
//#include "..\System\Commands.h"
#include "GGeometryUtil.h"
//#include "..\Misc\HPTimer.h"

#include <cstdint>

namespace NGfx
{
	_3DLIB_EXPORT int nVCacheSize = 10;
}

namespace NGScene
{

void FilterTrinagles( std::vector<STriangle> *pRes, const std::vector<uint16_t> &filter )
{
	int nTarget = 0;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		const STriangle &src = (*pRes)[k];
		STriangle &res = (*pRes)[nTarget];
		res.i1 = filter[ src.i1 ];
		res.i2 = filter[ src.i2 ];
		res.i3 = filter[ src.i3 ];
		nTarget += (res.i1 != res.i2) & (res.i1 != res.i3) & (res.i2 != res.i3);
	}
	pRes->resize( nTarget );
}

void MergePositions( std::vector<uint16_t> *pMatches, std::vector<CVec3> *pPositions )
{
	std::vector<CVec3> mergedPositions;
	std::vector<CVec3> &positions = *pPositions;
	std::vector<uint16_t> &posIndices = *pMatches;
	posIndices.resize( positions.size() );
	mergedPositions.reserve( pPositions->size() );
	typedef std::unordered_map<CVec3,int,SVec3Hash> CPosHash;
	CPosHash posHash;
	for ( int k = 0; k < positions.size(); ++k )
	{
		int nRes;
		CPosHash::iterator i = posHash.find( positions[k] );
		if ( i == posHash.end() )
		{
			nRes = mergedPositions.size();
			mergedPositions.push_back( positions[k] );
			posHash[ positions[k] ] = nRes;
		}
		else
			nRes = i->second;
		posIndices[k] = nRes;
	}
	positions = mergedPositions;
}

// CObjectInfo 

struct SRefTracker
{
	std::vector<int> temp;

	void SetSize( int n )
	{
		temp.resize( n );
		for ( int k = 0; k < temp.size(); ++k )
			temp[k] = -1;
	}
	int GetRef( int nIndex, int nPos )
	{
		int &nRef = temp[ nIndex ];
		if ( nRef == -1 )
		{
			nRef = nPos;
			return nPos;
		}
		return nRef;
	}
};

void CObjectInfo::EstablishRefs()
{
	SRefTracker rt, rtPos;
	rt.SetSize( positions.size() );
	vertRefPositions.resize( verts.size() );
	for ( int k = 0; k < vertRefPositions.size(); ++k )
		vertRefPositions[k] = rt.GetRef( posIndices[k], k );
}

struct SCompoundPosKey
{
	CVec3 v;
	SRealVertexWeight w;

	SCompoundPosKey() {}
	SCompoundPosKey( const CVec3 &_v, const SRealVertexWeight &_w ) : v(_v), w(_w) {}
	bool operator== ( const SCompoundPosKey &k ) const { return v == k.v && w == k.w; }
};
struct CalcCompoundKeyHash
{
	int operator()( const SCompoundPosKey &k ) const { SVec3Hash v; return v(k.v); }
};
void CObjectInfo::MergePositions()
{
	// merge positions
	if ( weights.size() != positions.size() )
	{
		NGScene::MergePositions( &posIndices, &positions );
		return;
	}
	std::vector<CVec3> mergedPositions;
	std::vector<SRealVertexWeight> mergedWeights;
	mergedPositions.reserve( positions.size() );
	mergedWeights.reserve( weights.size() );
	posIndices.resize( positions.size() );
	typedef std::unordered_map<SCompoundPosKey,int,CalcCompoundKeyHash> CPosHash;
	CPosHash posHash;
	for ( int k = 0; k < positions.size(); ++k )
	{
		int nRes;
		SCompoundPosKey key( positions[k], weights[k] );
		CPosHash::iterator i = posHash.find( key );
		if ( i == posHash.end() )
		{
			nRes = mergedPositions.size();
			mergedPositions.push_back( positions[k] );
			mergedWeights.push_back( weights[k] );
			posHash[ key ] = nRes;
		}
		else
			nRes = i->second;
		posIndices[k] = nRes;
	}
	positions = mergedPositions;
	weights = mergedWeights;
}

void CObjectInfo::AssignGeometry( const SData &data, std::vector<STriangle> *pGeom )
{
	// initialize stuff
	positions.resize( data.verts.size() );
	verts.resize( data.verts.size() );
	weights.resize( data.weights.size() );
	for ( int k = 0; k < data.weights.size(); ++k )
	{
		SRealVertexWeight &res = weights[k];
		const SVertexWeight &src = data.weights[k];
		for ( int i = 0; i < 4; ++i )
		{
			res.fWeights[i] = src.fWeights[i];
			res.cBoneIndices[i] = src.cBoneIndices[i];
			//ASSERT( src.fWeights[i] >= 0 && src.fWeights[i] <= 1 );
			res.nWeights[i] = Clamp( Float2Int( src.fWeights[i] * 256 ), 0, 255 );
		}
		// sort them, we are not in a hurry :)
		for ( int i = 1; i < 4; ++i )
		{
			for ( int k = i - 1; k >= 0; --k )
			{
				if ( res.fWeights[k] < res.fWeights[k+1] )
				{
					std::swap( res.fWeights[k], res.fWeights[k+1] );
					std::swap( res.nWeights[k], res.nWeights[k+1] );
					std::swap( res.cBoneIndices[k], res.cBoneIndices[k+1] );
				}
				else
					break;
			}
		}
		ASSERT( res.fWeights[0] >= res.fWeights[1] && res.fWeights[1] >= res.fWeights[2] && res.fWeights[2] >= res.fWeights[3] );
	}
	for ( int k = 0; k < data.verts.size(); ++k )
	{
		const SVertex &v = data.verts[k];
		SUVInfo &res = verts[k];
		positions[k] = v.pos;
		res.tex.nU = Float2Int( v.tex.u * N_VERTEX_TEX_SIZE );
		res.tex.nV = Float2Int( v.tex.v * N_VERTEX_TEX_SIZE );
		res.texLM.nU = 0;
		res.texLM.nV = 0;
		res.normal = v.normal;
		res.texU = v.texU;
		res.texV = v.texV;
	}
	ASSERT( data.secondTex.size() <= verts.size() );
	for ( int k = 0; k < data.secondTex.size(); ++k )
	{
		const CVec2 &vSrc = data.secondTex[k];
		SUVInfo &res = verts[k];
		res.texLM.nU = Float2Int( vSrc.u * N_VERTEX_TEX_SIZE );
		res.texLM.nV = Float2Int( vSrc.v * N_VERTEX_TEX_SIZE );
	}
	attributes = data.attributes;
	geometry.swap( *pGeom );
}

template<class T>
static void Reorder( std::vector<T> *pRes, const std::vector<int> &newPlaces )
{
	std::vector<T> tmp( pRes->size() );
	for ( int k = 0; k < tmp.size(); ++k )
		tmp[k] = (*pRes)[ newPlaces[k] ];
	*pRes = tmp;
}

void CObjectInfo::CalcAverageTriArea()
{
	std::vector<STriangle> tris;
	GetPosTriangles( &tris );
	if ( tris.empty() )
	{
		fAverageTriArea = 0;
		return;
	}
	float fArea = 0;
	for ( int i = 0; i < tris.size(); ++i )
	{
		const STriangle &t = tris[i];
		fArea += fabs( ( positions[t.i1] - positions[t.i2] ) ^ ( positions[t.i3] - positions[t.i2] ) ) * 0.5f;
	}
	fAverageTriArea = fArea / tris.size();
}

static std::vector<uint32_t> emptyAttribute;
const std::vector<uint32_t> &CObjectInfo::GetAttribute( int nID )
{
	for ( int k = 0; k < attributes.size(); ++k )
	{
		if ( attributes[k].nID == nID )
			return attributes[k].data;
	}
	return emptyAttribute;
}

void CObjectInfo::SetAttribute( int nID, const std::vector<uint32_t> &attr )
{
	for ( int k = 0; k < attributes.size(); ++k )
	{
		if ( attributes[k].nID == nID )
		{
			attributes[k].data = attr;
			return;
		}
	}
	std::vector<SStream>::iterator it = attributes.insert( attributes.end(), SStream() );
	it->nID = nID;
	it->data = attr;
}

template<class T>
static void Shuffle( std::vector<T> *pRes, const std::vector<T> &src, const std::vector<uint16_t> &vxReorder, int nResVertices )
{
	pRes->resize( nResVertices );
	ASSERT( vxReorder.size() <= src.size() );
  for ( int k = 0; k < vxReorder.size(); ++k )
	{
		if ( vxReorder[k] < nResVertices )
			(*pRes)[ vxReorder[k] ] = src[k];
	}
}
void CObjectInfo::Assign( const SData &data, std::vector<STriangle> *pGeom, bool bOptimizeVCache )
{
	if ( bOptimizeVCache )
	{
		SData optimizedData;
		if ( data.geometry.size() > 0  )
		{
			// optimize for vertex cache
			std::vector<STriangle> &tris = optimizedData.geometry;
			tris = data.geometry;
			CTriVertexCacheOptimizer vxOptimize;
			std::vector<uint16_t> vxReorder;
			int nResVertices;
			//NHPTimer::STime tStart;
			//NHPTimer::GetTime( &tStart );
			vxOptimize.Optimize( &tris, &vxReorder, &nResVertices, NGfx::nVCacheSize );
			//float fPassed = NHPTimer::GetTimePassed( &tStart );
			//DebugTrace( "%d tris, %g msec passed\n", tris.size(), fPassed * 1000000 );
			ASSERT( nResVertices <= data.verts.size() );
			FilterTrinagles( &tris, vxReorder );
			Shuffle( &optimizedData.verts, data.verts, vxReorder, nResVertices );
			if ( !data.weights.empty() )
			{
				ASSERT( data.weights.size() == data.verts.size() );
				Shuffle( &optimizedData.weights, data.weights, vxReorder, nResVertices );
			}
			if ( !data.secondTex.empty() )
			{
				ASSERT( data.secondTex.size() == data.verts.size() );
				Shuffle( &optimizedData.secondTex, data.secondTex, vxReorder, nResVertices );
			}
			optimizedData.attributes.resize( data.attributes.size() );
			for ( int k = 0; k < optimizedData.attributes.size(); ++k )
			{
				const SStream &strSrc = data.attributes[k];
				ASSERT( strSrc.data.size() == data.verts.size() );
				SStream &str = optimizedData.attributes[k];
				str.nID = strSrc.nID;
				Shuffle( &str.data, strSrc.data, vxReorder, nResVertices );
			}
		}
		AssignGeometry( optimizedData, &optimizedData.geometry );
	}
	else
		AssignGeometry( data, pGeom );
	MergePositions();
	nTris = geometry.size();
	EstablishRefs();
	CalcAverageTriArea();
}

void CObjectInfo::Assign( SData *pData, bool bOptimizeVCache )
{
	Assign( *pData, &pData->geometry, bOptimizeVCache );
}

void CObjectInfo::Assign( const SData &data, bool bOptimizeVCache )
{
	std::vector<STriangle> geom( data.geometry );
	Assign( data, &geom, bOptimizeVCache );
}

void CObjectInfo::AssignFast( SData *pData )
{
	AssignGeometry( *pData, &pData->geometry );
	nTris = geometry.size();
	posIndices.resize( verts.size() );
	for ( int k = 0; k < posIndices.size(); ++k )
		posIndices[k] = k;
	fAverageTriArea = 0;
}

void CObjectInfo::GetVxPositionTriangles( std::vector<STriangle> *pRes ) const
{
	*pRes = geometry;
	if ( !vertRefPositions.empty() )
		FilterTrinagles( pRes, vertRefPositions );
}

void CObjectInfo::GetPosTriangles( std::vector<STriangle> *pRes ) const
{
	*pRes = geometry;
	if ( !posIndices.empty() )
		FilterTrinagles( pRes, posIndices );
}

void CObjectInfo::CalcBound( SBound *pRes )
{
	if ( verts.empty() )
		pRes->BoxInit( VNULL3, VNULL3 );
	else
		::CalcBound( pRes, positions, SGetSelf<CVec3>() );
}

void CObjectInfo::CalcBound( SSphere *pRes )
{
	::CalcBound( pRes, positions, SGetSelf<CVec3>() );
}

// SplitWrapping

struct SVertexShift
{
	int nVertex;
	int nXShift, nYShift;
	SVertexShift( int _nVertex, int _nXShift, int _nYShift ) : nVertex(_nVertex), nXShift(_nXShift), nYShift(_nYShift) {}
};
inline bool operator==( const SVertexShift &a, const SVertexShift &b ) 
{
	return a.nVertex == b.nVertex && a.nXShift == b.nXShift && a.nYShift == b.nYShift;
}
struct SVertexShiftHash
{
	int operator()( const SVertexShift &a ) const { return a.nVertex ^ (( a.nXShift + a.nYShift ) << 16); }
};

template<class TGetTex>
static int GetVertex( CObjectInfo::SData *pData, std::unordered_map<SVertexShift,int,SVertexShiftHash> *pShifted,
	int _nXShift, int _nYShift, int _nVertex, TGetTex f )
{
	if ( _nXShift == 0 && _nYShift == 0 )
		return _nVertex;
	SVertexShift idx( _nVertex, _nXShift, _nYShift );
	std::unordered_map<SVertexShift,int,SVertexShiftHash>::iterator i = pShifted->find( idx );
	if ( i != pShifted->end() )
		return i->second;
	int nRes = pData->verts.size();
	(*pShifted)[idx] = nRes;
	SVertex v = pData->verts[_nVertex];
	pData->verts.push_back( v );
	if ( !pData->weights.empty() )
		pData->weights.push_back( pData->weights[_nVertex] );
	if ( !pData->secondTex.empty() )
		pData->secondTex.push_back( pData->secondTex[_nVertex] );
	CVec2 &tex = f.GetTex( pData, pData->verts.size() - 1 );
	tex.u -= _nXShift;
	tex.v -= _nYShift;
	return nRes;
}

static int GetShift( float fX1, float fX2, float fX3 )
{
	int nX1 = Float2Int( fX1 - 0.5f );//* ( 0.5f / F_MAX_TEX_WRAP ) );
	int nX2 = Float2Int( fX2 - 0.5f );//* ( 0.5f / F_MAX_TEX_WRAP ) );
	int nX3 = Float2Int( fX3 - 0.5f );//* ( 0.5f / F_MAX_TEX_WRAP ) );
	int nXMin = (std::min)( (std::min)( nX1, nX2 ), nX3 );
	int nXMax = (std::max)( (std::max)( nX1, nX2 ), nX3 );
	int nDif = nXMax - nXMin;
	if ( nDif < N_MAX_TEX_WRAP )
		return ( nXMin & ~(N_MAX_TEX_WRAP - 1) ) + N_MAX_TEX_WRAP;// ;
	else if ( nDif < N_MAX_TEX_WRAP * 2 )
		return nXMin + N_MAX_TEX_WRAP;
  ASSERT(0);
	return nXMin + N_MAX_TEX_WRAP * 2 - 1;
}

template<class TGetTex>
void SplitWrapping( CObjectInfo::SData *pData, TGetTex f )
{
	// for test
	//for ( int k = 0; k < pData->verts.size(); ++k )
	//	pData->verts[k].tex *= 20;
	bool bHasLargeUV = false;
	for ( int k = 0; k < pData->verts.size(); ++k )
	{
		CVec2 &tex = f.GetTex( pData, k );
		if ( fabs( tex.x ) > N_MAX_TEX_WRAP || fabs( tex.y ) > N_MAX_TEX_WRAP )
		{
			bHasLargeUV = true;
			break;
		}
	}
	if ( !bHasLargeUV )
		return;
	// "noramlize" to resolve simple cases
	float fXMin = 1e30f, fYMin = 1e30f, fXMax = -1e30f, fYMax = -1e30f;
	for ( int k = 0; k < pData->verts.size(); ++k )
	{
		const CVec2 &tex = f.GetTex( pData, k );
		float fX = tex.x;
		float fY = tex.y;
		fXMin = (std::min)( fXMin, fX );
		fYMin = (std::min)( fYMin, fY );
		fXMax = (std::max)( fXMax, fX );
		fYMax = (std::max)( fYMax, fY );
	}
	fXMin = floor( fXMin );
	fYMin = floor( fYMin );
	bool bNeedSplit = ( (fXMax - fXMin) > N_MAX_TEX_WRAP * 2 ) | ( (fYMax - fYMin) > N_MAX_TEX_WRAP );
	if ( !bNeedSplit )
	{
		fXMin += N_MAX_TEX_WRAP;
		fYMin += N_MAX_TEX_WRAP;
		for ( int k = 0; k < pData->verts.size(); ++k )
		{
			CVec2 &tex = f.GetTex( pData, k );
			tex.x -= fXMin;
			tex.y -= fYMin;
		}
		return;
	}
	// resolve complex cases
	CObjectInfo::SData res( *pData );
	std::vector<STriangle> &tris = res.geometry;
	tris = pData->geometry;
	std::unordered_map<SVertexShift,int,SVertexShiftHash> shifted;
	for ( int k = 0; k < tris.size(); ++k )
	{
		STriangle &t = tris[k];
		CVec2 &tex1 = f.GetTex( pData, t.i1 );
		CVec2 &tex2 = f.GetTex( pData, t.i2 );
		CVec2 &tex3 = f.GetTex( pData, t.i3 );
		int nXShift = GetShift( tex1.x, tex2.x, tex3.x );
		int nYShift = GetShift( tex1.y, tex2.y, tex3.y );

		t.i1 = GetVertex( &res, &shifted, nXShift, nYShift, t.i1, f );
		t.i2 = GetVertex( &res, &shifted, nXShift, nYShift, t.i2, f );
		t.i3 = GetVertex( &res, &shifted, nXShift, nYShift, t.i3, f );
	}
	*pData = res;
}

struct SGetTex
{
	CVec2& GetTex( CObjectInfo::SData *pData, int k ) { return pData->verts[k].tex; }
};

struct SGetTex2
{
	CVec2& GetTex( CObjectInfo::SData *pData, int k ) { return pData->secondTex[k]; }
};

void SplitWrapping( CObjectInfo::SData *pData )
{
  if ( pData->verts.empty() )
    return;
	SplitWrapping( pData, SGetTex() );
}

void SplitWrapping2( CObjectInfo::SData *pData )
{
  if ( pData->secondTex.empty() )
    return;
	SplitWrapping( pData, SGetTex2() );
}
}

