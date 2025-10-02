#pragma once

#include "AttachedObj.h"

namespace NDb
{
	struct SModel;
	struct SEffect;
}

namespace NAnimation
{
	struct ISkeletonAnimator;
}

class CAttachedObject : public IAttachedObject
{
	CObj<CObjectBase> pHolder;
	ZDATA
		CDGPtr< CFuncBase<SFBTransform> > pTransform;
		int nBoneIndex;
		CSyncSrcBind<IVisObj> srcBind;
		CDGPtr< CFuncBase<SBound> > pBounder;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransform); f.Add(3,&nBoneIndex); f.Add(4,&srcBind); f.Add(5,&pBounder); return 0; }

protected:
	void SetHolder( CObjectBase *_pHolder ) { pHolder = _pHolder; }
	CObjectBase* GetHolder() const { return pHolder; }
	CSyncSrcBind<IVisObj>& GetSrcBind() { return srcBind; }

	CAttachedObject() { }
	CAttachedObject( CFuncBase<SFBTransform> *_pTransform, const int _nBoneIndex, CFuncBase<SBound> * _pBounder )
		: pTransform( _pTransform ), nBoneIndex( _nBoneIndex ), pBounder( _pBounder ) { }

	void RefreshTransform() { pTransform.Refresh(); }
public:
	CFuncBase<SFBTransform>* GetTransform() const { return pTransform; }
	virtual void SetTransform( CFuncBase<SFBTransform> *_pTransform ) { pTransform = _pTransform; }

	CFuncBase<SBound>* GetBounder() const { return pBounder; }

	// remove this object from scene
	virtual void Destroy( const NTimer::STime time );	
	virtual void Clear( const NTimer::STime time );
	// create object at scene
	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer ) = 0;
	
	// get bone to which attached
	virtual const int GetBoneIndex() const { return nBoneIndex; }
};

struct CStaticAttachedObj : public CAttachedObject
{
	OBJECT_NOCOPY_METHODS( CStaticAttachedObj )

	ZDATA_( CAttachedObject )
		CDBPtr<NDb::SModel> pModel;
		bool bUseLOD;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CAttachedObject *)this); f.Add(2,&pModel); f.Add(3,&bUseLOD); return 0; }
public:
	CStaticAttachedObj() { }
	CStaticAttachedObj( const NDb::SModel *_pModel, CFuncBase<SFBTransform> *pTransform, const int nBoneIndex, CFuncBase<SBound> *pBounder, bool _bUseLOD )
		: CAttachedObject( pTransform, nBoneIndex, pBounder ), pModel( _pModel ), bUseLOD( _bUseLOD ) { }

	virtual const NDb::SModel* GetModel() const;
	virtual NAnimation::ISkeletonAnimator* GetAnimator() const { return 0; }

	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer );
	virtual void Visit( IAIVisitor *pAIVisitor );
};

struct CAnimatedAttachedObj : public CAttachedObject
{
	OBJECT_NOCOPY_METHODS( CAnimatedAttachedObj )

	ZDATA_( CAttachedObject )
		CDBPtr<NDb::SModel> pModel;
		CObj<NAnimation::ISkeletonAnimator> pAnimator;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CAttachedObject *)this); f.Add(2,&pModel); f.Add(3,&pAnimator); return 0; }
public:
	CAnimatedAttachedObj() { }
	CAnimatedAttachedObj( const NDb::SModel *_pModel, CFuncBase<SFBTransform> *pTransform, NAnimation::ISkeletonAnimator *_pAnimator, const int nBoneIndex, CFuncBase<SBound> *pBounder )
		: CAttachedObject( pTransform, nBoneIndex, pBounder ), pModel( _pModel ), pAnimator( _pAnimator ) { }

	virtual const NDb::SModel* GetModel() const;
	virtual NAnimation::ISkeletonAnimator* GetAnimator() const { return pAnimator; }

	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer );
	virtual void Visit( IAIVisitor *pAIVisitor );
};

struct CAttachedEffect : public CAttachedObject
{
	OBJECT_NOCOPY_METHODS( CAttachedEffect )

	ZDATA_( CAttachedObject )
		CDBPtr<NDb::SEffect> pEffect;
		NTimer::STime nStartTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CAttachedObject *)this); f.Add(2,&pEffect); f.Add(3,&nStartTime); return 0; }
public:
	CAttachedEffect() { }
	CAttachedEffect( const NDb::SEffect *_pEffect, CFuncBase<SFBTransform> *pTransform, const NTimer::STime _nStartTime, const int nBoneIndex )
		: CAttachedObject( pTransform, nBoneIndex, 0 ), pEffect( _pEffect ), nStartTime( _nStartTime ) { }
	~CAttachedEffect();

	const NDb::SModel* GetModel() const { return 0; }
	NAnimation::ISkeletonAnimator* GetAnimator() const { return 0; }

	void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer );
	void Destroy( const NTimer::STime time );
	void Clear( const NTimer::STime time );
};

// A transform that removes the rotational part from the transform
class CAlwaysVerticalTransform: public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS(CAlwaysVerticalTransform)
protected:
	virtual bool NeedUpdate() { return pBaseTransform.Refresh(); }
	virtual void Recalc();

public:
	ZDATA
	CDGPtr< CFuncBase<SFBTransform> > pBaseTransform;
	bool bKeepVertical;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseTransform); f.Add(3,&bKeepVertical); return 0; }

	CAlwaysVerticalTransform() : pBaseTransform(0) {}
	CAlwaysVerticalTransform( CFuncBase<SFBTransform> *_pBaseTransform, const bool _bKeepVertical = false ); 
};

// CAttachedLightEffectPos

class CAttachedLightEffectPos: public CFuncBase<CVec3>
{
	OBJECT_NOCOPY_METHODS(CAttachedLightEffectPos)
private:
	ZDATA
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	SHMatrix mAddTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransform); f.Add(3,&mAddTransform); return 0; }

protected:
	virtual void Recalc();
	virtual bool NeedUpdate() { return pTransform.Refresh(); }

public:
	CAttachedLightEffectPos() {}
	CAttachedLightEffectPos( CFuncBase<SFBTransform> *_pTransform, const SHMatrix &_mAddTransform )
		: pTransform( _pTransform ), mAddTransform(_mAddTransform) {}
};

class CAttachedSoundPos: public CFuncBase<CVec3>
{
	OBJECT_NOCOPY_METHODS(CAttachedSoundPos)
private:
	ZDATA
		CDGPtr<CFuncBase<SFBTransform> > pTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransform); return 0; }

protected:
	virtual bool NeedUpdate() { return pTransform.Refresh(); }
	virtual void Recalc() { Vis2AI( &value, pTransform->GetValue().forward.GetTrans3() ); }

public:
	CAttachedSoundPos() {}
	CAttachedSoundPos( CFuncBase<SFBTransform> *_pTransform )	: pTransform( _pTransform ) {}
};

// CAttachedLightEffectDir

class CAttachedLightEffectDir: public CFuncBase<CVec3>
{
	OBJECT_NOCOPY_METHODS(CAttachedLightEffectDir)
private:
	ZDATA
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransform); return 0; }

protected:
	virtual void Recalc();
	virtual bool NeedUpdate() { return pTransform.Refresh(); }

public:
	CAttachedLightEffectDir() {}
	CAttachedLightEffectDir( CFuncBase<SFBTransform> *_pTransform ): pTransform( _pTransform ) {}
};

struct CAttachedLightEffect : public CAttachedObject
{
	OBJECT_NOCOPY_METHODS( CAttachedLightEffect )

	ZDATA_( CAttachedObject )
	NDb::SAttachedLightEffect lightEffect;
	NTimer::STime nStartTime;
	bool bInEditor;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CAttachedObject *)this); f.Add(2,&lightEffect); f.Add(3,&nStartTime); f.Add(4,&bInEditor); OnSerialize( f ); return 0; }
	int nPointLightID;
	CObj< CObjectBase > pFlareHolder;
	CObj< CObjectBase > pAddEffectHolder;
	CObj< CObjectBase > pDirFlareHolder;
	CObj< CAttachedLightEffectTransform > pConeTransform;
	CObj< CAttachedLightEffectTransform > pParticleTransform;
	CObj< CAttachedLightEffectPos > pFlarePos;
	CObj< CAttachedSoundPos > pSoundPos;
	bool bNeedRecalc;
	void OnSerialize( IBinSaver &saver );

	int nSoundID;

	void AddSound( const NDb::SComplexSoundDesc *pSound );
	void RemoveSound();

	void RegeneratePositions();	
public:
	CAttachedLightEffect() : nPointLightID(OBJECT_ID_GENERATE), bNeedRecalc(false), nSoundID(-1) { }
	CAttachedLightEffect( const NDb::SAttachedLightEffect *_pEffect, CFuncBase<SFBTransform> *pTransform, const NTimer::STime _nStartTime, const int nBoneIndex, const bool _bInEditor = false );

	virtual const NDb::SModel* GetModel() const { return 0; }
	virtual NAnimation::ISkeletonAnimator* GetAnimator() const { return 0; }

	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer );
	virtual void Destroy( const NTimer::STime time );
};

struct CAttachedStaticLightEffect : public CAttachedObject
{
	OBJECT_NOCOPY_METHODS( CAttachedStaticLightEffect )

	ZDATA_( CAttachedObject )
	int nObjectUniqueID;
	NDb::SAttachedLightEffect lightEffect;
	NTimer::STime nStartTime;
	CVec3 vPointLightPos;
	SHMatrix mConePlace;
	int nConeID;
	bool bInEditor;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CAttachedObject *)this); f.Add(2,&nObjectUniqueID); f.Add(3,&lightEffect); f.Add(4,&nStartTime); f.Add(5,&vPointLightPos); f.Add(6,&mConePlace); f.Add(7,&nConeID); f.Add(8,&bInEditor); OnSerialize( f ); return 0; }
	int nPointLightID;
	CObj<CObjectBase> pFlareHolder;
	CObj< CObjectBase > pDirFlareHolder;
	CDGPtr< CCCVec3 > pvFlarePlace;
	CObj< CCSFBTransform > pConeTransform;
	bool bNeedRecalc;
	void OnSerialize( IBinSaver &saver );

	void RegeneratePositions();
public:
	CAttachedStaticLightEffect() : nPointLightID(OBJECT_ID_GENERATE), bNeedRecalc(false) { }
	CAttachedStaticLightEffect( const NDb::SAttachedLightEffect *_pEffect, const int _nObjectID, const NTimer::STime _nStartTime, const bool _bInEditor = false );

	virtual const NDb::SModel* GetModel() const { return 0; }
	virtual NAnimation::ISkeletonAnimator* GetAnimator() const { return 0; }

	virtual void ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer );
	virtual void Destroy( const NTimer::STime time );
};


