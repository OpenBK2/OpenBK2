#pragma once

#include "PlanePathFraction.h"

typedef std::pair<const IPathFraction *, float> CCurFraction;

//	CPathFractionComplexBase

class CPathFractionComplexBase : public IPathFraction
{
protected:
	typedef std::vector<CPtr<IPathFraction>> CSubstitutes;
	CSubstitutes substitute;

	CCurFraction GetCur( const float fDist ) const;
	void AfterSubstitute();

	CPathFractionComplexBase( const CPathList &pathList );

	CPathFractionComplexBase() {}

public:
	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, &substitute );
		return 0;
	}

	float GetLength() const override;
	CVec3 GetPoint( const float fDist ) const override;
	CVec3 GetTangent( const float fDist ) const override;
	CVec3 GetNormale( const float fDist ) const override;
};

// CPathFractionComplex

class CPathFractionComplex : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPathFractionComplex )

public:
	CPathFractionComplex() {}

	CPathFractionComplex( const CPathList &pathList )
		: CPathFractionComplexBase( pathList )
	{
	}

	void GetSimplePath( CPathList *paths ) override
	{
		NI_ASSERT( false, "wrong call" );
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, static_cast<CPathFractionComplexBase *>( this ) );
		return 0;
	}
};
