#pragma once

struct IAnimUnit : public CAIObjectBase
{
	virtual void AnimationSet( int nAnimation ) = 0;
	virtual void Moved() = 0;
	virtual void Stopped() = 0;
	virtual void StopCurAnimation() = 0;

	virtual void Segment() = 0;

	virtual void Init( class CAIUnit *pOwner ) = 0;
};
