#pragma once

#include <cstdint>

namespace NAI
{

struct SEdge
{
	uint16_t wStart, wFinish;
	//
	SEdge() {}
	SEdge( uint16_t _wS, uint16_t _wF ): wStart(_wS), wFinish(_wF) {}
};

class CEdgesInfo
{
	uint16_t InsertEdge( uint16_t i1, uint16_t i2, const vector<CVec3> &pts );
public:
	vector<SEdge> edges;
	vector<STriangle> mesh;
	bool bClosed;
	//
	CEdgesInfo() { bClosed = true; }
	void BuildTriangleList( vector<STriangle> *pRes ) const;
	void BuildClosedMeshes( vector<vector<STriangle> > *pMeshes ) const;
	void GenerateEdgeList( const vector<STriangle> &tris, const vector<CVec3> &pts );
	bool IsClosed() const; // checks geometry
	bool IsEmpty() const { return mesh.empty(); }
};

struct SJunction
{
	CVec3 pt;
	bool  bGround;

	SJunction() {}
	SJunction( const CVec3 &_pt, bool _bGr = false ): pt(_pt), bGround(_bGr) {}
};

//class CPrecalcSpheres;
//typedef std::unordered_map<int, CPtr<CPrecalcSpheres> > CPrecalcPieces;

struct SPiece
{
	vector<CVec3> points;
	CEdgesInfo edges;
	float fVolume;
	vector<SJunction> juncs;
	//vector<CPtr<CPrecalcSpheres> > precalc;
};

class CGeometryInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CGeometryInfo);
public:
	typedef std::unordered_map<int, SPiece> CPieceMap;

	SBound bound;
	vector<SMassSphere> spheres;
	CVec3 massCenter;
	CPieceMap pieces;
	//
	SPiece* GetPiece( int nPieceID );
	void AddPiece( int nPieceID, const vector<CVec3> &_points, const vector<STriangle> &_tris, 
		float fVolume, vector<SJunction> juncs = vector<SJunction>(), bool _bClosed = true );
		//const vector<CPtr<CPrecalcSpheres> > &spheres = vector<CPtr<CPrecalcSpheres> >() );
	void CalcBound();
	//void PrecalcCollideInfo( bool bTerrain = false );
	//void SetCollideInfo( const CPrecalcPieces &precalc );
	bool HasPiece( int nID ) const { return pieces.find( nID ) != pieces.end(); }
};

//! structure describing object for different tracers
struct SSourceInfo;
struct SConvexHull
{
	const vector<CVec3> &points;
	const CEdgesInfo &tris;
	const SHMatrix &trans;
	const SSourceInfo &src;
	int nUserID;
	//const vector<CPtr<CBSPTree> > trees;
	//const vector<CPtr<CPrecalcSpheres> > &precalc;
	//
	SConvexHull( const vector<CVec3> &_points, const CEdgesInfo &_tris, const SHMatrix &_trans,
		SSourceInfo &_src, int _nUserID )//, const vector<CPtr<CPrecalcSpheres> > &_precalc )
	: points(_points), tris(_tris), trans(_trans), src(_src), nUserID(_nUserID) {}//, precalc(_precalc) {}
	int operator&( CStructureSaver &f ) { ASSERT(0&&"This struct could not be serialized!"); }
};

//! group of entities; entity terrain is represented with several SConvexHull
struct SHullSet
{
	vector<SConvexHull> objects, terrain;
};

}


