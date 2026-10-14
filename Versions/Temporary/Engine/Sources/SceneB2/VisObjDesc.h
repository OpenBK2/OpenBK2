#pragma once

#include "3Dmotor/GAnimation.hpp"
#include "Misc/Sync.h"
#include "Scene.h"
#include "VisObjSelection.h"

#include <cstdint>

namespace NDb
{
struct SModel;
struct SEffect;
} // namespace NDb

struct IAIVisitor;
class CCSTime;

struct SVisObjDescBase : public IVisObj
{
	ZDATA
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nID = {};
	bool bHidden = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nID );
		f.Add( 3, &bHidden );
		return 0;
	}

	CObj<CObjectBase> pObj;

	//
	SVisObjDescBase()
		: bHidden( false )
	{
	}

	~SVisObjDescBase() override
	{
		ClearObject();
	}

private:
	SVisObjDescBase( const SVisObjDescBase &a )
	{
		ASSERT( 0 );
	}

	const SVisObjDescBase &operator=( const SVisObjDescBase &a )
	{
		ASSERT( 0 );
		return *this;
	}

public:
	//
	int GetID() const
	{
		return nID;
	}

	//
	virtual void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) = 0;
	virtual void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) = 0;

	virtual void UpdateSrcBind() {}

	virtual void UpdateStuff( class CVisObjIconsManager *pVOIM ) {}

	//
	virtual const SFBTransform &GetPlacement() const = 0;
	virtual CCSFBTransform *GetTransform() const = 0;

	virtual NAnimation::ISkeletonAnimator *GetAnimator( bool bRefreshAnimator )
	{
		return 0;
	}

	virtual CFuncBase<SBound> *GetBounder()
	{
		return 0;
	}

	//
	virtual void ClearObject()
	{
		pObj = 0;
	}

	virtual void
	ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) = 0;

	virtual void SetFade( float _fFade ) {}

	//
	virtual void ChangeModel( const NDb::SModel *pNewModel,
	                          CCSTime *pGameTimer,
	                          NGScene::IGameView *pGScene,
	                          CSyncSrc<IVisObj> *pSyncSrc,
	                          bool bShowBB )
	{
	}
};

struct SModelVisObjDesc : public SVisObjDescBase
{
	typedef std::unordered_map<int, CObj<struct IAttachedObject>> TAttachOfOneType;
	typedef std::unordered_map<int, TAttachOfOneType> CAttaches;
	ZDATA_( SVisObjDescBase )
	CDBPtr<NDb::SModel> pModel;
	SVisObjSelection selection;
	bool bSelected = {};
	float fFade = {};
	CAttaches attachedObjects;
	ZSKIP;
	CDBPtr<NDb::SModel> pLowLevelModel;
	CObj<CObjectBase> pCircle;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SVisObjDescBase *)this );
		f.Add( 2, &pModel );
		f.Add( 3, &selection );
		f.Add( 4, &bSelected );
		f.Add( 5, &fFade );
		f.Add( 6, &attachedObjects );
		f.Add( 8, &pLowLevelModel );
		f.Add( 9, &pCircle );
		return 0;
	}

	static std::vector<int> n2Attach;
	CObj<CObjectBase> pObjLowLevel;

	CSyncSrcBind<IVisObj> srcBind;
	CObj<CObjectBase> pBBPolyLine;

	SModelVisObjDesc()
		: bSelected( false )
		, fFade( -1.0f )
	{
		n2Attach.resize( __ESSOT_COUNTER, 0 );
	}

	void SetFade( float _fFade ) override
	{
		fFade = _fFade;
	}

	void FillBBPoints( std::vector<CVec3> &bbPoints, std::vector<uint16_t> &bbIndices );
	void UpdateBBPolyLine( NGScene::IGameView *pGScene );
	void UpdateSrcBind() override;
	//
	void Visit( IAIVisitor *pVisitor ) override;

	//
	void ClearObject() override
	{
		SVisObjDescBase::ClearObject();
		pObjLowLevel = 0;
		pBBPolyLine = 0;
		srcBind.Unlink();
	}

	void ClearAttached( const ESceneSubObjType eType );
	//
	void UpdateStuff( class CVisObjIconsManager *pVOIM ) override;
};

// static object in game
struct SStaticVisObjDesc : public SModelVisObjDesc
{
	OBJECT_NOCOPY_METHODS( SStaticVisObjDesc )
public:
	ZDATA_( SModelVisObjDesc )
	SFBTransform transform;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SModelVisObjDesc *)this );
		f.Add( 2, &transform );
		return 0;
	}

	//
	void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) override;
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) override;
	void MoveAfterUpdatePlacement( NGScene::IGameView *pGScene );

	const SFBTransform &GetPlacement() const override
	{
		return transform;
	}

	CCSFBTransform *GetTransform() const override
	{
		return 0;
	}

	void ChangeModel( const NDb::SModel *pNewModel,
	                  CCSTime *pGameTimer,
	                  NGScene::IGameView *pGScene,
	                  CSyncSrc<IVisObj> *pSyncSrc,
	                  bool bShowBB ) override;
	void
	ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) override;

	CFuncBase<SBound> *GetBounder() override
	{
		return 0;
	}
};

// static object in editor (can be moved)
struct SDynamicVisObjDesc : public SModelVisObjDesc
{
	OBJECT_NOCOPY_METHODS( SDynamicVisObjDesc )
public:
	ZDATA_( SModelVisObjDesc )
	CObj<CCSFBTransform> pTransform;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SModelVisObjDesc *)this );
		f.Add( 2, &pTransform );
		return 0;
	}

	//
	void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) override;
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) override;

	const SFBTransform &GetPlacement() const override
	{
		return pTransform->GetValue();
	}

	CCSFBTransform *GetTransform() const override
	{
		return pTransform;
	}

	void
	ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) override;
};

// animated object

struct SAnimatedVisObjDescBase : public SModelVisObjDesc
{
	ZDATA_( SModelVisObjDesc )
	CDGPtr<NAnimation::ISkeletonAnimator> pAnimator;
	CObj<CCSFBTransform> pTransform;
	CDGPtr<CFuncBase<SBound>> pBounder;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SModelVisObjDesc *)this );
		f.Add( 2, &pAnimator );
		f.Add( 3, &pTransform );
		f.Add( 4, &pBounder );
		return 0;
	}

	//
	SAnimatedVisObjDescBase();
	//
	void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) override;
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) override;

	const SFBTransform &GetPlacement() const override
	{
		return pTransform->GetValue();
	}

	CCSFBTransform *GetTransform() const override
	{
		return pTransform;
	}

	NAnimation::ISkeletonAnimator *GetAnimator( bool bRefreshAnimator ) override
	{
		if ( bRefreshAnimator )
		{
			pAnimator.Refresh();
		}
		return pAnimator;
	}

	CFuncBase<SBound> *GetBounder() override
	{
		pBounder.Refresh();
		return pBounder;
	}

	void ClearObject() override;
	void
	ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) override;
	//
	void ChangeModel( const NDb::SModel *pNewModel,
	                  CCSTime *pGameTimer,
	                  NGScene::IGameView *pGScene,
	                  CSyncSrc<IVisObj> *pSyncSrc,
	                  bool bShowBB ) override;
	virtual CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene,
	                                         const NDb::SModel *pModel,
	                                         NGScene::IGameView::SMeshInfo *pMeshInfoPassed ) = 0;
};

struct SAnimatedVisObjDesc : public SAnimatedVisObjDescBase
{
	OBJECT_NOCOPY_METHODS( SAnimatedVisObjDesc )
public:
	ZDATA_( SAnimatedVisObjDescBase )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SAnimatedVisObjDescBase *)this );
		return 0;
	}

	//
	CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene,
	                                 const NDb::SModel *pModel,
	                                 NGScene::IGameView::SMeshInfo *pMeshInfoPassed ) override;
};

struct SAnimatedStaticVisObjDesc : public SAnimatedVisObjDescBase
{
	OBJECT_NOCOPY_METHODS( SAnimatedStaticVisObjDesc )
public:
	ZDATA_( SAnimatedVisObjDescBase )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SAnimatedVisObjDescBase *)this );
		return 0;
	}

	//
	CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene,
	                                 const NDb::SModel *pModel,
	                                 NGScene::IGameView::SMeshInfo *pMeshInfoPassed ) override;
};

// effect vis obj
struct SEffectVisObjBase : public SVisObjDescBase
{
	ZDATA_( SVisObjDescBase )
	CDBPtr<NDb::SEffect> pEffect;
	NTimer::STime timeStart = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SVisObjDescBase *)this );
		f.Add( 2, &pEffect );
		f.Add( 3, &timeStart );
		return 0;
	}
};

struct SStaticEffectVisObj : public SEffectVisObjBase
{
	OBJECT_NOCOPY_METHODS( SStaticEffectVisObj )
public:
	ZDATA_( SEffectVisObjBase )
	SFBTransform transform;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SEffectVisObjBase *)this );
		f.Add( 2, &transform );
		return 0;
	}

	//
	void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) override;
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) override;

	//
	const SFBTransform &GetPlacement() const override
	{
		return transform;
	}

	CCSFBTransform *GetTransform() const override
	{
		return 0;
	}

	void
	ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) override;
};

struct SDynamicEffectVisObj : public SEffectVisObjBase
{
	OBJECT_NOCOPY_METHODS( SDynamicEffectVisObj )
public:
	ZDATA_( SEffectVisObjBase )
	CObj<CCSFBTransform> pTransform;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (SEffectVisObjBase *)this );
		f.Add( 2, &pTransform );
		return 0;
	}

	//
	void
	UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) override;
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) override;

	//
	const SFBTransform &GetPlacement() const override
	{
		return pTransform->GetValue();
	}

	CCSFBTransform *GetTransform() const override
	{
		return pTransform;
	}

	void
	ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) override;
};
