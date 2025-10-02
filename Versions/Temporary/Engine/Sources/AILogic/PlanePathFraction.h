#if !defined(_IPathFraction_included_)
#define _IPathFraction_included_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



//	SPrevPathParams

struct SPrevPathParams
{
	CVec3 p0;
	CVec3 p1;
	CVec3 p2;				
	CVec3 vCur;// current point in that plane is

	CVec3 vSpeed;									// speed vector at vCur
	CVec3 vNormale;								// normale (top plane's direction) at vCur
	float fSplineProgress;
	
	float fDistToGo;	// distance that have to be gone on new path
	float fCurTiltSpeed;
	SPrevPathParams( interface IPlane *pPlane );
};

interface IPathFraction;
typedef list<CPtr<IPathFraction> > CPathList;

//	IPathFraction 

interface IPathFraction : public CAIObjectBase
{
	virtual float GetLength() const = 0;
	
	//{ approximate calculations (for aiming)
	virtual CVec3 GetPoint( const float fDist ) const = 0;
	virtual CVec3 GetTangent( const float fDist ) const = 0;
	// direction of plane's top (ideal)
	virtual CVec3 GetNormale( const float fDist ) const { return V3_AXIS_Z;}
	//}

	virtual CVec3 GetPoint() const { NI_ASSERT(false, "" ); return VNULL3; }
	virtual CVec3 GetTangent() const { NI_ASSERT(false, "" ); return VNULL3; }
	virtual CVec3 GetNormale() const { return V3_AXIS_Z;}

	// returns end point. unlike to GetPoint it works on unsubstituted paths
	virtual CVec3 GetEndPoint() const { return GetPoint( GetLength() );}
	virtual CVec3 GetStartPoint() const { return GetPoint(0.0f); }

	virtual CVec3 GetEndTangent() const { return GetTangent(GetLength()); }
	virtual CVec3 GetStartTangent() const { return GetTangent(0.0f); }

	// adds simple paths to path list.
	// simple path must have smooth acceleration 
	virtual void GetSimplePath( CPathList *paths ) = 0;

	// return true if path is finished. 
	// If path is finished, remaining dist is returned in pfDistRemain
	virtual bool Iterate( const float fDist, float *pfDistRemain ) { NI_ASSERT( false, "wrong call" ); return true; }

	virtual void GetPrevPoints( struct SPrevPathParams *pParams ) const { NI_ASSERT( false, "wrong call" ); }
	//virtual void GetPrevPoints( CVec3 *p0, CVec3 *p1, CVec3 *p2, CVec3 *pCur, float *pfSplineProgress  ) const { NI_ASSERT( false, "wrong call" ); }
};
#endif //_IPathFraction_included_

