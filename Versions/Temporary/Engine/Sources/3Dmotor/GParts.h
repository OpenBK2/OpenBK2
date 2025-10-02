#pragma once
#include "GScene.h"
#include "OcTree.h"
#include "GCombiner.h"
#include "..\3Dlib\Transform.h"
namespace NGScene
{

struct SCombinedKey
{
	ZDATA
	CObj<IMaterial> pMaterial;
	SFullGroupInfo fullGroupInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMaterial); f.Add(3,&fullGroupInfo); return 0; }

	SCombinedKey() {}
	SCombinedKey( IMaterial *_pMaterial, const SFullGroupInfo &_info )
		: pMaterial(_pMaterial), fullGroupInfo(_info)
	{
		SGroupInfo &groupInfo = fullGroupInfo.groupInfo;
		if ( !pMaterial->DoesCastShadow() )
			groupInfo.nObjectGroup &= ~N_MASK_CAST_SHADOW;
		if ( (groupInfo.nObjectGroup & N_MASK_CAST_SHADOW) && pMaterial->IsOpaque() )
			groupInfo.nObjectGroup |= N_MASK_OPAQUE;
		else
			groupInfo.nObjectGroup &= ~N_MASK_OPAQUE;
		if ( pMaterial->DoesIgnoreZ() )
			groupInfo.nObjectGroup |= N_MASK_IGNOREZ;
	}
	//
	//	bool operator==( const SCombinedKey &k ) const { return pMaterial == k.pMaterial && fullGroupInfo == k.fullGroupInfo; }
};
struct SFullStaticTrackers
{
	ZDATA
	CObj<CFuncBase<SPerVertexLightState> > pLightState;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pLightState); return 0; }
};

class IHZBuffer;
class CVolumeNode;
class CCombinedPart;
class ISomePart : public IPart
{
	ZDATA_(IPart)
	SCombinedKey group;
	SPerPartVariables vars;
public:
	CPtr<CCombinedPart> pOwner;
	vector<CObj<CObjectBase> > decals;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IPart*)this); f.Add(2,&group); f.Add(3,&vars); f.Add(4,&pOwner); f.Add(5,&decals); return 0; }
private:
	void SetCacheLightFlags();
	void SetVars( const SPerPartVariables &_vars );
public:
	ISomePart() {}
	ISomePart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	IMaterial* GetMaterial() const { return group.pMaterial; }
	const SGroupInfo& GetGroupInfo() const { return group.fullGroupInfo.groupInfo; }
	const SFullGroupInfo& GetFullGroupInfo() const { return group.fullGroupInfo; }
	virtual bool Is2Sided() const { return group.pMaterial->Is2Sided(); }
	virtual int GetSortValue() const { return ((int)group.pMaterial.GetPtr()) + vars.GetSortValue(); }
	void SetFade( float _fFade );
	float GetFade() const { return vars.fFade; }
	void SetLM( CPtrFuncBase<NGfx::CTexture> *pLM );
	const SPerPartVariables &GetVars() const { return vars; }
	void SetPriority( int _n );
};

class CNonePart : public ISomePart
{
	OBJECT_NOCOPY_METHODS(CNonePart);
public:
	CNonePart() {}
	CNonePart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
		: ISomePart( pData, _pMaterial, _gInfo ) {}
	virtual ETransformType GetTransformType() const { return TT_NONE; }
};

class CSimplePart : public ISomePart
{
	OBJECT_NOCOPY_METHODS(CSimplePart);
	ZDATA_(ISomePart)
	SFBTransform pos;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(ISomePart*)this); f.Add(2,&pos); return 0; }
public:
	CSimplePart() {}
	CSimplePart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo, const SFBTransform &_pos ) : ISomePart( pData, _pMaterial, _gInfo ), pos(_pos) {}
	virtual ETransformType GetTransformType() const { return TT_SIMPLE; }
	virtual const SFBTransform& GetSimplePos() { return pos; }
};

// animated in some bounds part
class CStaticAnimatedPart : public ISomePart
{
	OBJECT_NOCOPY_METHODS(CStaticAnimatedPart);
	ZDATA_(ISomePart)
	CDGPtr<CFuncBase< vector<SHMatrix> > > pAnimation;
	CDGPtr<CFuncBase< vector<NGfx::SCompactTransformer> > > pMMXAnimation;
	SBound bv;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(ISomePart*)this); f.Add(2,&pAnimation); f.Add(3,&pMMXAnimation); f.Add(4,&bv); return 0; }
public:
	CStaticAnimatedPart() {}
	CStaticAnimatedPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase< vector<SHMatrix> > *pAnim, CFuncBase<vector<NGfx::SCompactTransformer> > *pMMXAnim, 
		const SBound &_bv, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	virtual ETransformType GetTransformType() const { return TT_SINGLE_SKIN; }
	virtual const vector<SHMatrix>& GetAnimation() { pAnimation.Refresh(); return pAnimation->GetValue(); }
	virtual const vector<NGfx::SCompactTransformer>& GetMMXAnimation() { pMMXAnimation.Refresh(); return pMMXAnimation->GetValue(); }
	const SBound &GetBound() const { return bv; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) { if ( bVertices ) p->AddHandle( pAnimation ); }
};

class CGenericDynamicPart : public ISomePart
{
protected:
	enum EBindType
	{
		BT_STATIC,
		BT_DYNAMIC,
		BT_NONE
	};
	ZDATA_(ISomePart)
	int nStillCounter;
	EBindType bt;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(ISomePart*)this); f.Add(2,&nStillCounter); f.Add(3,&bt); return 0; }
public:
	CGenericDynamicPart() : bt(BT_NONE) {}
	CGenericDynamicPart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
};

class CDynamicPart : public CGenericDynamicPart
{
	OBJECT_NOCOPY_METHODS( CDynamicPart );
private:
	ZDATA_(CGenericDynamicPart)
		SBound bound;
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	CDGPtr<CPtrFuncBase<CObjectInfo> > pTrackObjInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGenericDynamicPart*)this); f.Add(2,&bound); f.Add(3,&pTransform); f.Add(4,&pTrackObjInfo); return 0; }

public:
	CDynamicPart() {}
	CDynamicPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *pPos, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	virtual ETransformType GetTransformType() const { return TT_SIMPLE; }
	virtual const SFBTransform& GetSimplePos() { return pTransform->GetValue(); }
	CFuncBase<SFBTransform>* GetSimplePosNode() { return pTransform; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) { if ( bVertices) p->AddHandle( pTransform ); }
	bool Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers );
};

class CAnimatedBoundPart : public CGenericDynamicPart
{
	ZDATA_(CGenericDynamicPart)
	CDGPtr<CFuncBase<SBound> > pBound;
	CChangeTrackPtr pChanges;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGenericDynamicPart*)this); f.Add(2,&pBound); f.Add(3,&pChanges); return 0; }

	CAnimatedBoundPart() {};
	CAnimatedBoundPart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo, CFuncBase<SBound> *_pBound, CVersioningBase *_pChange ) : CGenericDynamicPart( pData, _pMaterial, _gInfo), pBound( _pBound ), pChanges( pData ) {}
	bool Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers );
	CFuncBase<SBound> *GetBound() const { return pBound; }
};

class CDynamicPartWithAnimatedBound : public CAnimatedBoundPart
{
	OBJECT_NOCOPY_METHODS( CDynamicPartWithAnimatedBound );
private:
	ZDATA_(CAnimatedBoundPart)
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAnimatedBoundPart*)this); f.Add(2,&pTransform); return 0; }

public:
	CDynamicPartWithAnimatedBound() {}
	CDynamicPartWithAnimatedBound( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *pPos, CFuncBase<SBound> *pBound, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	virtual ETransformType GetTransformType() const { return TT_SIMPLE; }
	virtual const SFBTransform& GetSimplePos() { return pTransform->GetValue(); }
	CFuncBase<SFBTransform>* GetSimplePosNode() { return pTransform; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) { if ( bVertices) p->AddHandle( pTransform ); }
};

class CAnimatedPart : public CAnimatedBoundPart
{
	OBJECT_NOCOPY_METHODS( CAnimatedPart );
	ZDATA_(CAnimatedBoundPart)
	CDGPtr<CFuncBase< vector<SHMatrix> > > pAnimation;
	CDGPtr<CFuncBase< vector<NGfx::SCompactTransformer> > > pMMXAnimation;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAnimatedBoundPart*)this); f.Add(2,&pAnimation); f.Add(3,&pMMXAnimation); return 0; }
public:
	CAnimatedPart() {}
	CAnimatedPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase< vector<SHMatrix> > *pAnim, CFuncBase<vector<NGfx::SCompactTransformer> > *pMMXAnim, 
		CFuncBase<SBound> *_pBound, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	virtual ETransformType GetTransformType() const { return TT_SINGLE_SKIN; }
	virtual const vector<SHMatrix>& GetAnimation() { pAnimation.Refresh(); return pAnimation->GetValue(); }
	virtual const vector<NGfx::SCompactTransformer>& GetMMXAnimation() { pMMXAnimation.Refresh(); return pMMXAnimation->GetValue(); }
	virtual CFuncBase< vector<SHMatrix> >* GetAnimationNode() { return pAnimation; }
	virtual CFuncBase< vector<NGfx::SCompactTransformer> >* GetMMXAnimationNode() { return pMMXAnimation; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) { if ( bVertices ) p->AddHandle( pAnimation ); }
};

// since we have got pBound here we can skip checking geometry changes in Update() and only watch them with CChangeTrackPtr
class CDynamicGeometryPart : public CGenericDynamicPart
{
	OBJECT_NOCOPY_METHODS( CDynamicGeometryPart );
	ZDATA_(CGenericDynamicPart)
	CDGPtr<CFuncBase<SBound> > pBound;
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	CChangeTrackPtr pChanges;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGenericDynamicPart*)this); f.Add(2,&pBound); f.Add(3,&pTransform); f.Add(4,&pChanges); return 0; }
public:
	CDynamicGeometryPart() {}
	CDynamicGeometryPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *_pTransform, CFuncBase<SBound> *pAnim, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo );
	virtual ETransformType GetTransformType() const { return pTransform ? TT_SIMPLE : TT_NONE; }
	virtual const SFBTransform& GetSimplePos() { return pTransform->GetValue(); }
	CFuncBase<SFBTransform>* GetSimplePosNode() { return pTransform; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices );
	bool Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers );
};

class CCombinedPart: public CObjectBase
{
	OBJECT_BASIC_METHODS(CCombinedPart);
public:
	struct SPartInfo
	{
		ZDATA
		SGroupInfo groupInfo;
		int nMaterial;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&groupInfo); f.Add(3,&nMaterial); return 0; }
	};
	struct SMaterialInfo
	{
		ZDATA
		CPtr<IMaterial> pMaterial;
		SPerPartVariables vars;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMaterial); f.Add(3,&vars); return 0; }

		bool operator==( const SMaterialInfo &a ) const { return a.pMaterial == pMaterial && a.vars == vars; }
	};
private:
	SRenderGeometryInfo geometryInfo;
	ZDATA
	CDGPtr<CPerMaterialCombiner> pCombiner;
	int nFloorMask;
	CPartFlags lastNewFlags;
	int nIgnoreMark;
	CPartFlags ignoredParts;
	vector<SPartInfo> parts;
	vector<SMaterialInfo> materials;
	CObj<CFuncBase<SPerVertexLightState> > pPervertexLightState;
	bool bRecalcPartInfo;
	bool bIsDynamic;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCombiner); f.Add(3,&nFloorMask); f.Add(4,&lastNewFlags); f.Add(5,&nIgnoreMark); f.Add(6,&ignoredParts); f.Add(7,&parts); f.Add(8,&materials); f.Add(9,&pPervertexLightState); f.Add(10,&bRecalcPartInfo); f.Add(11,&bIsDynamic); return 0; }
	void InitGeometry();

public:
	CCombinedPart() : nIgnoreMark(0), bRecalcPartInfo(false) {}
	CCombinedPart( SFullStaticTrackers *pTrackers, int _nFloorMask, bool bIsDynamic );
	CPerMaterialCombiner* GetCombiner() const { return pCombiner; }
	IVBCombiner* GetVBCombiner() { return GetGeometryInfo()->pVertices; }
	SRenderGeometryInfo* GetGeometryInfo();
	void SetIgnored( int _nIgnoreMark, const CPartFlags &parts );
	int GetIgnoreMark() const { return nIgnoreMark; }
	int GetMaterialsNumber() const { return materials.size(); }
	int GetFloorMask() const { return nFloorMask; }
	const SMaterialInfo& GetMaterial( int n ) const { return materials[n]; }
	const vector<SPartInfo>& GetPartsInfo() const { return parts; }
	void UpdatePartInfo();
	const CPartFlags& GetIgnoredParts() const { return ignoredParts; }
	void PartHasChanged() { bRecalcPartInfo = true; }
};

//! node size
const int N_MINIMAL_OCTREE_NODE = 8;//16; 
class CVolumeNode : public COcTreeNode<CVolumeNode, N_MINIMAL_OCTREE_NODE>
{
	OBJECT_NOCOPY_METHODS( CVolumeNode );
public:
	struct SPerMaterialHolder
	{
		ZDATA
		vector<CObj<CCombinedPart> > transparent, normal;
		list<CPtr<CCombinedPart> > elements;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&transparent); f.Add(3,&normal); f.Add(4,&elements); return 0; }

		CCombinedPart* AllocCombinerPart( vector<CObj<CCombinedPart> > *pRes, int nFloorMask, SFullStaticTrackers *pTrackers, bool bIsDynamic )
		{
			for ( int k = 0; k < pRes->size(); ++k )
			{
				CCombinedPart *p = (*pRes)[k];
				if ( p->GetCombiner()->GetSize() < PF_MAX_PARTS_PER_COMBINER && p->GetFloorMask() == nFloorMask )
					return (*pRes)[k];
			}
			// have to alloc new one
			CCombinedPart *p = new CCombinedPart( pTrackers, nFloorMask, bIsDynamic );
			pRes->push_back( p );
			elements.push_back( p );
			return p;
		}
		CCombinedPart* GetCombinerPartForAdd( IMaterial *pMaterial, SFullStaticTrackers *pTrackers, int nFloorMask, bool bIsDynamic )
		{
			if ( pMaterial->IsTransparent() )
				return AllocCombinerPart( &transparent, nFloorMask, pTrackers, bIsDynamic );
			return AllocCombinerPart( &normal, nFloorMask, pTrackers, bIsDynamic );
		}
		bool IsEmpty() const
		{
			for ( list<CPtr<CCombinedPart> >::const_iterator i = elements.begin(); i != elements.end(); ++i )
			{
				ASSERT( IsValid( *i ) );
				if ( (*i)->GetCombiner()->GetSize() != 0 )
					return false;
			}
			return true;
		}
	};
	struct SUpdatableObjects
	{
		ZDATA
		int nLastFrame;
		vector< CPtr<CDynamicGeometryPart> > dynamicFrags;
		vector< CPtr<CAnimatedBoundPart> > animatedParts;
		vector< CPtr<CDynamicPart> > movingParts;
		vector< CPtr<CParticles> > particles;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLastFrame); f.Add(3,&dynamicFrags); f.Add(4,&animatedParts); f.Add(5,&movingParts); f.Add(6,&particles); return 0; }
		SUpdatableObjects() : nLastFrame(0) {}
		bool IsEmpty();
		void Update( int nFrame, CVolumeNode *pVolume, SFullStaticTrackers *pTrackers );
	};
	//
	ZDATA_(CParent)
	SPerMaterialHolder staticParts, dynamicParts;
	list<CPtr<CParticles> > particles;
	SUpdatableObjects updatable;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CParent*)this); f.Add(2,&staticParts); f.Add(3,&dynamicParts); f.Add(4,&particles); f.Add(5,&updatable); return 0; }

	CVolumeNode* SelectNode( CFuncBase<SFBTransform> *pTransform, const SBound &bound )
	{
		const SFBTransform &pos = pTransform->GetValue();
		CVec3 ptCenter;
		pos.forward.RotateHVector( &ptCenter, bound.s.ptCenter );
		float fR = sqrt( CalcRadius2( bound, pos.forward ) );
		return GetNode( ptCenter, fR );
	}
	virtual bool IsEmpty();
private:	
	typedef COcTreeNode<CVolumeNode, N_MINIMAL_OCTREE_NODE> CParent;
};

void PlaceToOctree( ISomePart *pPart, CVolumeNode *pRoot, const CVec3 &vPos, float fR, 
	SFullStaticTrackers *pTrackers, bool bIsDynamic );
CVolumeNode *GetUpdatable( CVolumeNode *pRoot, const SBound &hintBV );
}

