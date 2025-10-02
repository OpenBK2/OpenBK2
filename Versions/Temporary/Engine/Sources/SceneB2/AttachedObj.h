#pragma once

#include "VisObjDesc.h"

namespace NGScene
{
	class IGameView;
}
namespace NDb
{
	struct SModel;
	struct SEffect;
	struct SAttachedLightEffect;
}
namespace NAnimation
{
	interface ISkeletonAnimator;
}
class CCSTime;
class CCCVec3;

interface IAttachedObject : virtual public CObjectBase, virtual public IVisObj
{
	virtual void Destroy( const NTimer::STime time ) = 0;
	virtual void Clear( const NTimer::STime time ) = 0;
	// create object at scene
	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer ) = 0;

	// get bone to which attached
	virtual const int GetBoneIndex() const = 0;

	virtual const NDb::SModel* GetModel() const = 0;
	virtual NAnimation::ISkeletonAnimator* GetAnimator() const = 0;
	virtual CFuncBase<SFBTransform>* GetTransform() const = 0;
	virtual void SetTransform( CFuncBase<SFBTransform> *pTransform ) = 0;

	static IAttachedObject* CreateStaticObj( const NDb::SModel *pModel, CFuncBase<SFBTransform> *pTransform, const int nBoneIndex, CFuncBase<SBound> *pBound, bool bUseLOD = false );
	static IAttachedObject* CreateAnimatedObj( const NDb::SModel *pModel, CFuncBase<SFBTransform> *pTransform,
																						 CFuncBase<STime> *pTime, const int nBoneIndex, CFuncBase<SBound> *pBound );
	static IAttachedObject* CreateEffect( const NDb::SEffect *pEffect, CFuncBase<SFBTransform> *pTransform, const NTimer::STime nStartTime,
																				const int nBoneIndex, bool bVertical = false );
	static IAttachedObject* CreateAnimatedLightEffect( const NDb::SAttachedLightEffect *pEffect, CFuncBase<SFBTransform> *pTransform,
																										 const NTimer::STime nStartTime, const int nBoneIndex, const bool bInEditor = false );
	static IAttachedObject* CreateStaticLightEffect( const NDb::SAttachedLightEffect *pEffect, const int nObjectID,
																									 const NTimer::STime nStartTime, const bool bInEditor = false );
};

//
// CAttachedLightEffectTransform - keeps a constant offset from a changing point
//

class CAttachedLightEffectTransform: public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS(CAttachedLightEffectTransform)
protected:
	virtual bool NeedUpdate() { return pBaseTransform.Refresh(); }
	virtual void Recalc();

public:
	ZDATA
		CDGPtr< CFuncBase<SFBTransform> > pBaseTransform;
	SHMatrix mMultiplier;
	CDGPtr< CCCVec3 > pvTranslatePart;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseTransform); f.Add(3,&mMultiplier); f.Add(4,&pvTranslatePart); return 0; }

	CAttachedLightEffectTransform() : pBaseTransform(0), pvTranslatePart(0) {}
	CAttachedLightEffectTransform( CFuncBase<SFBTransform> *_pBaseTransform, const SHMatrix &_mMultiplier ); 
};

class CConstantOffsetTransform : public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS( CConstantOffsetTransform )
	ZDATA
		CDGPtr< CFuncBase<SFBTransform> > pBaseTransform;
		int nTargetID;
		string szBoneName;
		bool bNeedCalcMatrix;
		SHMatrix mMultiplier;
		CDGPtr< CConstantOffsetTransform > pParentTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseTransform); f.Add(3,&nTargetID); f.Add(4,&szBoneName); f.Add(5,&bNeedCalcMatrix); f.Add(6,&mMultiplier); f.Add(7,&pParentTransform); return 0; }

protected:
	virtual bool NeedUpdate() { return pBaseTransform.Refresh(); }
	virtual void Recalc();

public:
	CConstantOffsetTransform() : pBaseTransform( 0 ), nTargetID( -1 ), bNeedCalcMatrix( true ) {}
	CConstantOffsetTransform( const int _nTargetID, const string &_szBoneName, CFuncBase<SFBTransform> *_pBaseTransform );
	CConstantOffsetTransform( const int _nTargetID, const string &_szBoneName, CFuncBase<SFBTransform> *_pBaseTransform, CFuncBase<SFBTransform> *_pParentTransform );
};

// fixed offset from the center of the parent object
class CCenterOffsetTransform : public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS( CCenterOffsetTransform )
	ZDATA
		CDGPtr< CFuncBase<SFBTransform> > pBaseTransform;
		int nTargetID;
		SHMatrix mMultiplier;
		SHMatrix mMultiplierInv;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseTransform); f.Add(3,&nTargetID); f.Add(4,&mMultiplier); f.Add(5,&mMultiplierInv); return 0; }

protected:
	virtual bool NeedUpdate() { return pBaseTransform.Refresh(); }
	virtual void Recalc();

public:
	CCenterOffsetTransform() : pBaseTransform( 0 ), nTargetID( -1 ) {}
	CCenterOffsetTransform( const int _nTargetID, CFuncBase<SFBTransform> *_pBaseTransform, const SHMatrix &_mMultiplier );
};

