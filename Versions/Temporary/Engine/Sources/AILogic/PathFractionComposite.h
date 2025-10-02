#pragma once

#include "PathFractionComplex.h"
#include "PlanePathMath.h"

// to manage path list as ordinary complex path
class CPathFractionComposite : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPathFractionComposite )
ZDATA_(CPathFractionComplexBase)
	vector<CVec3> points;
	vector<CVec3> normales;
	int nIndex;															// point index
	
	float fSplineProgress;									// move by spline (0..1)
	CVec3 p0, p1, p2, p3;										// points to init spline
	ZSKIP	
	CAnalyticBSpline3 spline;

	CVec3 vPos;					
	CVec3 vSpeed;
	CVec3 vNormale;
	
	float fPathLenght;											// 
	CPtr<IPathFraction> pInitialPath;
	bool bFinished;

	CVec3 vPosLastPoint;					
	CVec3 vSpeedLastPoint;
	CVec3 vNormaleLastPoint;

	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&points); f.Add(3,&normales); f.Add(4,&nIndex); f.Add(5,&fSplineProgress); f.Add(6,&p0); f.Add(7,&p1); f.Add(8,&p2); f.Add(9,&p3); f.Add(11,&spline); f.Add(12,&vPos); f.Add(13,&vSpeed); f.Add(14,&vNormale); f.Add(15,&fPathLenght); f.Add(16,&pInitialPath); f.Add(17,&bFinished); f.Add(18,&vPosLastPoint); f.Add(19,&vSpeedLastPoint); f.Add(20,&vNormaleLastPoint); return 0; }
public:
	CPathFractionComposite() : fSplineProgress( 0 )  {  }
	CPathFractionComposite( interface IPlane *pPlane, interface IPathFraction *pPath );

	virtual CVec3 GetPoint() const 
	{ 
		return vPos; 
	}
	virtual CVec3 GetTangent() const 
	{ 
		return vSpeed; 
	}
	virtual CVec3 GetNormale() const 
	{ 
		return vNormale;
	}
	virtual bool Iterate( const float fDist, float *pfDistRemain );

	virtual float GetLength() const 
	{ 
		return fPathLenght; 
	}
	virtual void GetSimplePath( CPathList *paths ) 
	{ 
		NI_ASSERT( false, "wrong call" ); 
	}
	virtual void GetPrevPoints( struct SPrevPathParams *pParams ) const;
};

