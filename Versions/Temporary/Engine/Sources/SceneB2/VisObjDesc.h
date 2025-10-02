#pragma once

#include "3DMotor/GAnimation.hpp"
#include "Misc/Sync.h"
#include "Scene.h"
#include "VisObjSelection.h"

namespace NDb
{
	struct SModel;
	struct SEffect;
}

struct IAIVisitor;
class CCSTime;

struct SVisObjDescBase : public IVisObj
{
	ZDATA
		int nID;
		bool bHidden;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&bHidden); return 0; }
	CObj<CObjectBase> pObj;
	//
	SVisObjDescBase(): bHidden( false ) {}
	virtual ~SVisObjDescBase() { ClearObject(); }

private:
	SVisObjDescBase( const SVisObjDescBase& a ){ ASSERT(0); }
	const SVisObjDescBase& operator = ( const SVisObjDescBase& a ){ ASSERT(0); return *this; }

public:
	//
	int GetID() const { return nID; }
	//
	virtual void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale ) = 0;
	virtual void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace ) = 0;
	virtual void UpdateSrcBind() {}
	virtual void UpdateStuff( class CVisObjIconsManager *pVOIM ) {}
	//
	virtual const SFBTransform &GetPlacement() const = 0;
	virtual CCSFBTransform* GetTransform() const = 0;
	virtual NAnimation::ISkeletonAnimator *GetAnimator( bool bRefreshAnimator ) { return 0; }
	virtual CFuncBase<SBound> *GetBounder() { return 0; }
	//
	virtual void ClearObject() { pObj = 0; }
	virtual void ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB ) = 0;
	virtual void SetFade( float _fFade ) {	}
	//
	virtual void ChangeModel( const NDb::SModel *pNewModel, CCSTime *pGameTimer, 
														NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, bool bShowBB ) { }
};

struct SModelVisObjDesc : public SVisObjDescBase
{
	typedef hash_map< int, CObj<struct IAttachedObject> > TAttachOfOneType;
	typedef hash_map<int, TAttachOfOneType> CAttaches;
	ZDATA_( SVisObjDescBase )
		CDBPtr<NDb::SModel> pModel;
		SVisObjSelection selection;
		bool bSelected;
		float fFade;
		CAttaches attachedObjects;
		ZSKIP;
		CDBPtr<NDb::SModel> pLowLevelModel;
		CObj<CObjectBase> pCircle;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SVisObjDescBase *)this); f.Add(2,&pModel); f.Add(3,&selection); f.Add(4,&bSelected); f.Add(5,&fFade); f.Add(6,&attachedObjects); f.Add(8,&pLowLevelModel); f.Add(9,&pCircle); return 0; }

	static vector<int> n2Attach;
	CObj<CObjectBase> pObjLowLevel;
	
	CSyncSrcBind<IVisObj> srcBind;
	CObj<CObjectBase> pBBPolyLine;

	SModelVisObjDesc()
		: bSelected( false ), fFade( -1.0f )
	{
		n2Attach.resize( __ESSOT_COUNTER, 0 );
	}

	void SetFade( float _fFade ) { fFade = _fFade; }
	void FillBBPoints( vector<CVec3> &bbPoints, vector<WORD> &bbIndices );
	void UpdateBBPolyLine( NGScene::IGameView *pGScene );
	void UpdateSrcBind();
	//
	virtual void Visit( IAIVisitor *pVisitor );
	//
	virtual void ClearObject() { SVisObjDescBase::ClearObject(); pObjLowLevel = 0; pBBPolyLine = 0; srcBind.Unlink(); }
	void ClearAttached( const ESceneSubObjType eType );
	//
	void UpdateStuff( class CVisObjIconsManager *pVOIM );
};

// static object in game
struct SStaticVisObjDesc : public SModelVisObjDesc
{
	OBJECT_NOCOPY_METHODS( SStaticVisObjDesc )
public:
	ZDATA_( SModelVisObjDesc )
		SFBTransform transform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SModelVisObjDesc *)this); f.Add(2,&transform); return 0; }
	//
	void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale );
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace );
	void MoveAfterUpdatePlacement( NGScene::IGameView *pGScene );
	const SFBTransform &GetPlacement() const { return transform; }	
	CCSFBTransform* GetTransform() const { return 0; }
	void ChangeModel( const NDb::SModel *pNewModel, CCSTime *pGameTimer, 
		NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, bool bShowBB );
	void ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB );
	
	CFuncBase<SBound> *GetBounder() { return 0; }
};

// static object in editor (can be moved)
struct SDynamicVisObjDesc : public SModelVisObjDesc
{
	OBJECT_NOCOPY_METHODS( SDynamicVisObjDesc )
public:
	ZDATA_(SModelVisObjDesc)
		CObj<CCSFBTransform> pTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SModelVisObjDesc*)this); f.Add(2,&pTransform); return 0; }
	//
	void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale );
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace );
	const SFBTransform &GetPlacement() const { return pTransform->GetValue(); }
	CCSFBTransform* GetTransform() const { return pTransform; }
	void ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB );
};

// animated object

struct SAnimatedVisObjDescBase : public SModelVisObjDesc
{
	ZDATA_( SModelVisObjDesc )
		CDGPtr<NAnimation::ISkeletonAnimator> pAnimator;
		CObj<CCSFBTransform> pTransform;
		CDGPtr<CFuncBase<SBound> > pBounder;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SModelVisObjDesc *)this); f.Add(2,&pAnimator); f.Add(3,&pTransform); f.Add(4,&pBounder); return 0; }
	//
	SAnimatedVisObjDescBase();
	//
	void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale );
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace );
	const SFBTransform &GetPlacement() const { return pTransform->GetValue(); }
	CCSFBTransform* GetTransform() const { return pTransform; }
	NAnimation::ISkeletonAnimator *GetAnimator(  bool bRefreshAnimator ) { if (bRefreshAnimator) pAnimator.Refresh(); return pAnimator; }
	CFuncBase<SBound> *GetBounder() { pBounder.Refresh(); return pBounder; }
	void ClearObject();
	void ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB );
	//
	void ChangeModel( const NDb::SModel *pNewModel, CCSTime *pGameTimer, 
										NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, bool bShowBB );
	virtual CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene, const NDb::SModel *pModel, NGScene::IGameView::SMeshInfo *pMeshInfoPassed ) = 0;
};

struct SAnimatedVisObjDesc : public SAnimatedVisObjDescBase 
{
	OBJECT_NOCOPY_METHODS( SAnimatedVisObjDesc )
public:
	ZDATA_(SAnimatedVisObjDescBase)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAnimatedVisObjDescBase*)this); return 0; }
	//
	CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene, const NDb::SModel *pModel, NGScene::IGameView::SMeshInfo *pMeshInfoPassed );
};
struct SAnimatedStaticVisObjDesc : public SAnimatedVisObjDescBase 
{
	OBJECT_NOCOPY_METHODS( SAnimatedStaticVisObjDesc )
public:
	ZDATA_(SAnimatedVisObjDescBase)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAnimatedVisObjDescBase*)this); return 0; }
	//
	CObjectBase *CreateAnimatedMesh( NGScene::IGameView *pGScene, const NDb::SModel *pModel, NGScene::IGameView::SMeshInfo *pMeshInfoPassed );
};

// effect vis obj
struct SEffectVisObjBase :	public SVisObjDescBase
{
	ZDATA_( SVisObjDescBase )
		CDBPtr<NDb::SEffect> pEffect;
		NTimer::STime timeStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SVisObjDescBase *)this); f.Add(2,&pEffect); f.Add(3,&timeStart); return 0; }
};

struct SStaticEffectVisObj : public SEffectVisObjBase
{
	OBJECT_NOCOPY_METHODS( SStaticEffectVisObj )
public:
	ZDATA_( SEffectVisObjBase )
		SFBTransform transform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SEffectVisObjBase *)this); f.Add(2,&transform); return 0; }
	//
	void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale );
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace );
	//
	const SFBTransform &GetPlacement() const { return transform; }
	CCSFBTransform* GetTransform() const { return 0; }
	void ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB );
};

struct SDynamicEffectVisObj : public SEffectVisObjBase
{
	OBJECT_NOCOPY_METHODS( SDynamicEffectVisObj )
public:
	ZDATA_( SEffectVisObjBase )
		CObj<CCSFBTransform> pTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SEffectVisObjBase *)this); f.Add(2,&pTransform); return 0; }
	//
	void UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale );
	void UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace );
	//
	const SFBTransform &GetPlacement() const { return pTransform->GetValue(); }
	CCSFBTransform* GetTransform() const { return pTransform; }
	void ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB );
};


