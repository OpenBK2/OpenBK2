#pragma once

#include "PlanePathFraction.h"


//	CPathFractionLine

class CPathFractionLine : public IPathFraction
{
	OBJECT_BASIC_METHODS( CPathFractionLine )
	ZDATA
	CVec3 x0, x1;
	CVec3 v0;
	float fLength;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&x0); f.Add(3,&x1); f.Add(4,&v0); f.Add(5,&fLength); return 0; }
public:
	CPathFractionLine() : fLength( 0 ) {  }
	// FOR 2D PATH
	void Init( const CVec2 &_x0, const CVec2 &_x1, const float _fZ );
	void Init( const CVec3 &_x0, const CVec3 &_x1 );
	
	virtual float GetLength() const 
	{ 
		return fLength ; 
	}
	virtual CVec3 GetPoint( const float fDist ) const 
	{ 
		return x0 + v0 * fDist; 
	}
	virtual CVec3 GetTangent( const float fDist ) const 
	{ 
		return v0; 
	}
	virtual CVec3 GetNormale( const float fDist ) const 
	{ 
		const CVec3 vXY = CVec3( -v0.y, v0.x, 0 );
		CVec3 vNorm( v0^vXY ); 
		//if ( vNorm.z == 0.0f )
			//return V3_AXIS_Z;
		Normalize( &vNorm );
		return vNorm;
	}

	virtual CVec3 GetStartPoint() const 
	{ 
		return x0;
	}
	virtual CVec3 GetStartTangent() const 
	{ 
		return v0;
	}
	virtual CVec3 GetEndPoint() const 
	{ 
		return x1;
	}
	virtual CVec3 GetEndTangent() const 
	{ 
		return v0;
	}
	virtual void GetSimplePath( CPathList *pPaths ) { NI_ASSERT( false, "wrong call" ); }
};


