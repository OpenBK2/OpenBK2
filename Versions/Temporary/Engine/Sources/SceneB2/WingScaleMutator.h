#pragma once

#include "AnimMutators.h"

class CWingScaleMutator : public IWingScaleMutator
{
	OBJECT_BASIC_METHODS( CWingScaleMutator );
	ZDATA
	float fScale;
	bool bShowStatic;
	std::vector<int> scaledWings;
	int nStaticWing;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &fScale );
		f.Add( 3, &bShowStatic );
		f.Add( 4, &scaledWings );
		f.Add( 5, &nStaticWing );
		return 0;
	}

private:
public:
	CWingScaleMutator()
		: fScale( 0.0f )
		, bShowStatic( true )
		, nStaticWing( -1 )
	{
	}

	bool Setup( ISkeletonAnimator *pAnimator,
	            const std::string &szScaledWingPrefix,
	            const std::string &szStaticWingName ) override;

	void SetScale( const float _fScale ) override
	{
		fScale = _fScale;
	}

	void ShowStatic( const bool bShow ) override
	{
		bShowStatic = bShow;
	}

	void MutateSkeletonPose( NAnimation::SSkeletonPose *pPose ) override;
};
