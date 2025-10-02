#ifndef __ANIM_UNIT_H__
#define __ANIM_UNIT_H__

#pragma ONCE

interface IAnimUnit : public CAIObjectBase
{
	virtual void AnimationSet( int nAnimation ) = 0;
	virtual void Moved() = 0;
	virtual void Stopped() = 0;
	virtual void StopCurAnimation() = 0;

	virtual void Segment() = 0;

	virtual void Init( class CAIUnit *pOwner ) = 0;
};

#endif // __ANIM_UNIT_H__

