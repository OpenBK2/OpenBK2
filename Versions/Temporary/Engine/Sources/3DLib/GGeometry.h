#pragma once

#include "3DLib_export.h"

#include "3Dmotor/GPixelFormat.h"

#include <cstdint>

namespace NGScene
{
const int N_MAX_TEX_WRAP = 8;

struct SVertex
{
	CVec3 pos;
	NGfx::SCompactVector normal, texU, texV;
	CVec2 tex;
};

const int N_VERTEX_TEX_SIZE = 2048;
struct SUVInfo
{
	NGfx::SShortTextureUV tex, texLM;
	NGfx::SCompactVector normal, texU, texV;
};

struct SVertexWeight
{
	float fWeights[4];
	uint8_t cBoneIndices[4];
	bool operator==( const SVertexWeight &v ) const { return memcmp( this, &v, sizeof(*this) ) == 0; }
};

struct SRealVertexWeight
{
	float fWeights[4];
	uint8_t nWeights[4];
	uint8_t cBoneIndices[4];
	bool operator==( const SRealVertexWeight &v ) const { return memcmp( this, &v, sizeof(*this) ) == 0; }
};

#pragma warning( disable: 4701 )
inline SVertexWeight GetWeight( const SRealVertexWeight &a )
{
	SVertexWeight res;
	for ( int k = 0; k < 4; ++k ) { res.fWeights[k] = a.fWeights[k]; res.cBoneIndices[k] = a.cBoneIndices[k]; }
	return res;
}
#pragma warning( default: 4701 )

class _3DLIB_EXPORT CObjectInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS(CObjectInfo);
public:
	struct SStream
	{
		ZDATA
		int nID;
		std::vector<uint32_t> data;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&data); return 0; }
	};
	struct SData
	{
		ZDATA
		std::vector<SVertex> verts;
		std::vector<STriangle> geometry;
		std::vector<SVertexWeight> weights;
		std::vector<CVec2> secondTex;
		std::vector<SStream> attributes;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&verts); f.Add(3,&geometry); f.Add(4,&weights); f.Add(5,&secondTex); f.Add(6,&attributes); return 0; }
	};
	struct SBinData
	{
		ZDATA
		std::vector<CVec3> positions;
		std::vector<SUVInfo> verts;
		std::vector<SRealVertexWeight> weights;
		std::vector<uint16_t> posIndices; // references to positions for each vert
		std::vector<uint16_t> vertRefPositions; // number of first position encounter for each vertex
		std::vector<STriangle> geometry;
		std::vector<SStream> attributes;
		int nTris;
		float fAverageTriArea;
		bool bIsLightmappable;
		ZEND int operator&( IBinSaver &f ) { 
			f.Add(2,&positions); 
			f.Add(3,&verts); 
			f.Add(4,&weights); 
			f.Add(5,&posIndices); 
			f.Add(6,&vertRefPositions); 
			f.Add(7,&geometry); 
			f.Add(8,&attributes); 
			f.Add(9,&nTris); 
			f.Add(10,&fAverageTriArea);
			f.Add(11,&bIsLightmappable);
			return 0; }
	};

	std::vector<CVec3> positions;
	std::vector<SUVInfo> verts;
	std::vector<SRealVertexWeight> weights;
	std::vector<uint16_t> posIndices; // references to positions for each vert
	std::vector<uint16_t> vertRefPositions; // number of first position encounter for each vertex
	std::vector<STriangle> geometry;
	std::vector<SStream> attributes;
	int nTris;
	float fAverageTriArea;
	bool bIsLightmappable;
	
private:
	void EstablishRefs();
	void MergePositions();
	void AssignGeometry( const SData &data, std::vector<STriangle> *pGeom );
	void CalcAverageTriArea();
	void Assign( const SData &data, std::vector<STriangle> *pGeom, bool bOptimizeVCache );

public:
	
	void AssignDestructive(SBinData *pData)
	{
		std::swap( positions, pData->positions );
		std::swap( verts, pData->verts );
		std::swap( weights, pData->weights );
		std::swap( posIndices, pData->posIndices );
		std::swap( vertRefPositions, pData->vertRefPositions );
		std::swap( geometry, pData->geometry );
		std::swap( attributes, pData->attributes );
		nTris = pData->nTris;
		fAverageTriArea = pData->fAverageTriArea;
		bIsLightmappable = pData->bIsLightmappable;
	}
	void AssignTo(SBinData *pData)
	{
		pData->positions = positions;
		pData->verts = verts;
		pData->weights = weights;
		pData->posIndices = posIndices;
		pData->vertRefPositions = vertRefPositions;
		pData->geometry = geometry;
		pData->attributes = attributes;
        pData->nTris = nTris;
	    pData->fAverageTriArea = fAverageTriArea;
        pData->bIsLightmappable = bIsLightmappable;
	}
	CObjectInfo() : nTris(0), bIsLightmappable(true) {}	
	void Assign( const SData &data, bool bOptimizeVCache );
	// will modify pData
	void Assign( SData *pData, bool bOptimizeVCache );
	void AssignFast( SData *pData );
	const std::vector<CVec3>& GetPositions() const { return positions; }
	const std::vector<SUVInfo>& GetVertices() const { return verts; }
	const std::vector<SRealVertexWeight>& GetWeights() const { return weights; }
	const std::vector<uint16_t>& GetPositionIndices() const { return posIndices; }
	const std::vector<STriangle>& GetGeometry() const { return geometry; }
	const std::vector<uint32_t> &GetAttribute( int nID );

	void SetAttribute( int nID, const std::vector<uint32_t> &attr );

	void GetVxPositionTriangles( std::vector<STriangle> *pRes ) const;
	void GetPosTriangles( std::vector<STriangle> *pRes ) const; // over positions[]
	int GetTrisCount() { return nTris; }
	float GetAverageTriArea() { return fAverageTriArea; }
	bool IsLightmappable() const { return bIsLightmappable; }
	void SetLightmappable( bool _b ) { bIsLightmappable = _b; }
	
	bool IsEmpty() { return nTris == 0; }
	void CalcBound( SBound *pRes );
	void CalcBound( SSphere *pRes );
	friend class CSquarePacker;
};
void FilterTrinagles( std::vector<STriangle> *pRes, const std::vector<uint16_t> &filter );
void MergePositions( std::vector<uint16_t> *pMatches, std::vector<CVec3> *pPositions );
_3DLIB_EXPORT void SplitWrapping( CObjectInfo::SData *pData );
void SplitWrapping2( CObjectInfo::SData *pData );

}

