#pragma once
namespace NAI
{

struct SEdge
{
	WORD wStart, wFinish;
	//
	SEdge() {}
	SEdge( WORD _wS, WORD _wF ): wStart(_wS), wFinish(_wF) {}
};

class CEdgesInfo
{
	WORD InsertEdge( WORD i1, WORD i2, const std::vector<CVec3> &pts );
public:
	std::vector<SEdge> edges;
	std::vector<STriangle> mesh;
	bool bClosed;
	//
	CEdgesInfo() { bClosed = true; }
	void BuildTriangleList( std::vector<STriangle> *pRes ) const;
	void BuildClosedMeshes( std::vector<std::vector<STriangle> > *pMeshes ) const;
	void GenerateEdgeList( const std::vector<STriangle> &tris, const std::vector<CVec3> &pts );
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
//typedef hash_map<int, CPtr<CPrecalcSpheres> > CPrecalcPieces;

struct SPiece
{
	std::vector<CVec3> points;
	CEdgesInfo edges;
	float fVolume;
	std::vector<SJunction> juncs;
	//vector<CPtr<CPrecalcSpheres> > precalc;
};

class CGeometryInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CGeometryInfo);
public:
	typedef std::unordered_map<int, SPiece> CPieceMap;

	SBound bound;
	std::vector<SMassSphere> spheres;
	CVec3 massCenter;
	CPieceMap pieces;
	//
	SPiece* GetPiece( int nPieceID );
	void AddPiece( int nPieceID, const std::vector<CVec3> &_points, const std::vector<STriangle> &_tris,
		float fVolume, std::vector<SJunction> juncs = std::vector<SJunction>(), bool _bClosed = true );
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
	const std::vector<CVec3> &points;
	const CEdgesInfo &tris;
	const SHMatrix &trans;
	const SSourceInfo &src;
	int nUserID;
	//const vector<CPtr<CBSPTree> > trees;
	//const vector<CPtr<CPrecalcSpheres> > &precalc;
	//
	SConvexHull( const std::vector<CVec3> &_points, const CEdgesInfo &_tris, const SHMatrix &_trans,
		SSourceInfo &_src, int _nUserID )//, const vector<CPtr<CPrecalcSpheres> > &_precalc )
	: points(_points), tris(_tris), trans(_trans), src(_src), nUserID(_nUserID) {}//, precalc(_precalc) {}
	int operator&( CStructureSaver &f ) { ASSERT(0&&"This struct could not be serialized!"); }
};

//! group of entities; entity terrain is represented with several SConvexHull
struct SHullSet
{
	std::vector<SConvexHull> objects, terrain;
};

}


