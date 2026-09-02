#include "stdafx.h"

#include <thread>
#include "RenderNode.h"
#include "GSceneUtils.h"
#include "GScene.h"
#include "GView.h"
#include "GltfFormat.h"
#include "GTexture.h"
#include "GBind.h"
#include "System/BasicShare.h"
#include "DBScene.h"
#include "GAnimFormat.h"
#include "GParticles.h"
#include "3DLib/Transform.h"
#include "GAnimLight.h"
#include "System/Commands.h"
#include "GPointLightGlow.h"
#include "GAnimUtils.h"
#include "GParticlesRain.h"
#include "System/VFSOperations.h"
#include "LoadingCounter.h"
#include "GMatShare.h"
#include "GPolyline.h"
#include "GSprite.h"
#include "AnimatedChannel.h"
#include "GfxUtils.h"
#include "GRenderClouds.h"
#include "ObjectFader.h"
#include "Fader.h"
#include "HeightFog.h"
#include "WindDeformer.h"
#include "DepthOfField.h"
#include "SkyDome.h"
#include "DecalFader.h"
#include "2DSceneSW.h"
#include "SWTexture.h"
#include "GRenderCore.h"
#include "Gfx.h"
#include "GfxRender.h"
#include "GRenderLight.h"

#include "port/time.h"

#include <algorithm>
#include <cstdint>

//#define DEBUG_LIGHTING
//#define FADE_TEST
//namespace NAnimation
//{
	//extern CBasicShare<SIntResKey, CGrannySkeletonLoader> shareGrannySkeletons;
//}

namespace NGScene
{
extern bool bFreeze;
static bool bShowParticles = true;
bool bWaterReflection = false;
extern bool bLowRAM;
#ifdef FADE_TEST
static std::vector<CPtr<CObjectBase> > fadeTestObjects;
#endif

// share objects
typedef CBasicShare<STextureKey, CFileTexture, STextureKeyHash> CTextureBasicShare;
_3DMOTOR_EXPORT CTextureBasicShare shareTextures(103);
static CBasicShare<SIntResKey, CParticlesLoader> shareParticles(109);

static CBasicShare<SPartAndSkeletonKey, CGrannyMeshLoader, SPartAndSkeletonKeyHash> shareGrannyMeshes(118);
static CBasicShare<CDBPtr<NDb::SCubeTexture>, CFileCubeTexture, SDBPtrHash > shareCubeTextures(114);
static CBasicShare<CDBPtr<NDb::SAnimLight>, CLightLoader, SDBPtrHash > shareAnimLights(115);
#ifdef _DEBUG
static EHSRMode defaultHSRMode = HSR_NONE;
static ESceneRenderMode defaultRenderMode = SRM_BEST;//SRM_FASTEST;
#else
static EHSRMode defaultHSRMode = HSR_NONE;
static ESceneRenderMode defaultRenderMode = SRM_BEST;
#endif

class CParticleTexture : public CHoldedPtrFuncBase<NGfx::CTexture>
{
	OBJECT_BASIC_METHODS(CParticleTexture);
	typedef CHoldedPtrFuncBase<NGfx::CTexture> TParent;

private:
	ZDATA
	ZSKIP
	CDGPtr<CSWTexture> pSWTexture;
	bool bFakeTexture;
	ZEND int operator&( IBinSaver &f ) { f.Add(3,&pSWTexture); return 0; }
	CDGPtr<CFuncBase<CVec3> > pColor;

protected:
	virtual bool NeedUpdate();
	virtual void Recalc();

public:
	void SetKey( const CDBPtr<NDb::STexture> &key ) 
	{ 
		const NDb::STexture *pTex = key;
		if ( !pTex )
			return;

		pSWTexture = NGScene::GetSWTex( pTex );
		bFakeTexture = true;
	}
	void SetColor( CFuncBase<CVec3> *_pColor ) { if ( pColor != _pColor ) pColor = _pColor; }
};

static CBasicShare<CDBPtr<NDb::STexture>, CParticleTexture, SDBPtrHash > shareParticleTextures(231);

bool CParticleTexture::NeedUpdate()
{
	bool bRes = TParent::NeedUpdate();
	bRes |= !IsValid( pValue );
	if ( pColor )
		bRes |= pColor.Refresh();

	if ( pSWTexture )
	{
		pSWTexture.Refresh();
		bRes |= bFakeTexture && pSWTexture->IsReady();
	}

	return bRes;
}

void CParticleTexture::Recalc()
{
	pSWTexture.Refresh();
	if ( !pSWTexture->IsReady() && !IsValid(pValue) )
	{
		if ( NGfx::Is16BitTextures() )	
		{
			pValue = NGfx::MakeTexture( 1, 1, -1, NGfx::SPixel4444::ID, NGfx::TRANSPARENT_TEXTURE, NGfx::CLAMP );
			NGfx::CTextureLock<NGfx::SPixel4444> sLock( pValue, 0, NGfx::INPLACE );
			sLock[0][0].wColor = 0;
		}
		else
		{
			pValue = NGfx::MakeTexture( 1, 1, -1, NGfx::SPixel8888::ID, NGfx::TRANSPARENT_TEXTURE, NGfx::CLAMP );
			NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, 0, NGfx::INPLACE );
			sLock[0][0] = 0;

		}
		return;
	}

	bFakeTexture = false;
		
	CSWTextureData * pTextureData = pSWTexture->GetValue();

	if ( NGfx::Is16BitTextures() )	
		pValue = NGfx::MakeTexture( pTextureData->GetSizeX(), pTextureData->GetSizeY(), -1, NGfx::SPixel4444::ID, NGfx::TRANSPARENT_TEXTURE, NGfx::CLAMP );
	else
		pValue = NGfx::MakeTexture( pTextureData->GetSizeX(), pTextureData->GetSizeY(), -1, NGfx::SPixel8888::ID, NGfx::TRANSPARENT_TEXTURE, NGfx::CLAMP );

	CVec3 vColor( 1.0f, 1.0f, 1.0f );
	if ( pColor )
	{
		pColor.Refresh();
		vColor = pColor->GetValue() * 4.0f;
	}

	CDynamicCast<NGfx::I2DBuffer> pValueBuffer( pValue );
	if ( pValueBuffer->GetNumMipLevels() > pTextureData->mips.size() )
	{
		ASSERT( false );
		return;
	}

	int nLevels = pValueBuffer->GetNumMipLevels();

	if ( NGfx::Is16BitTextures() )
	{
		for ( int nLevel = 0; nLevel < nLevels; ++nLevel )
		{
			NGfx::CTextureLock<NGfx::SPixel4444> sLock( pValue, nLevel, NGfx::INPLACE );

			for ( int nY = 0; nY < sLock.GetSizeY(); ++nY )
			{
				for ( int nX = 0; nX < sLock.GetSizeX(); ++nX )
				{
					NGfx::SPixel4444 &sDst = sLock[nY][nX];
					const NGfx::SPixel8888 &sSrc = pTextureData->mips[nLevel][nY][nX];

					sDst.r = (uint8_t)( sSrc.r * vColor.x ) >> 4;
					sDst.g = (uint8_t)( sSrc.g * vColor.y ) >> 4;
					sDst.b = (uint8_t)( sSrc.b * vColor.z ) >> 4;
					sDst.a = sSrc.a >> 4;
				}
			}
		}
	}
	else
	{
		for ( int nLevel = 0; nLevel < nLevels; ++nLevel )
		{
			NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, nLevel, NGfx::INPLACE );

			for ( int nY = 0; nY < sLock.GetSizeY(); ++nY )
			{
				for ( int nX = 0; nX < sLock.GetSizeX(); ++nX )
				{
					NGfx::SPixel8888 &sDst = sLock[nY][nX];
					const NGfx::SPixel8888 &sSrc = pTextureData->mips[nLevel][nY][nX];
					
					sDst.r = sSrc.r*vColor.x;
					sDst.g = sSrc.g*vColor.y;
					sDst.b = sSrc.b*vColor.z;
					sDst.a = sSrc.a;
				}
			}
		}
	}
}

class CRenderNode;
class CGameView: public IGameView
{
	OBJECT_BASIC_METHODS(CGameView);
	ZDATA
	CObj<IGScene> pScene;
	std::vector<CPtr<CRenderNode> > nodes; // if object is just created and no refs are stored it should not become leak
	ZSKIP
	//list<CPtr<CGrassTracker> > grassTrackers;
	CObj<CMaterialShare> pMaterials;
	CColorMaterialShare colorMaterials;
	CTransparentMaterialShare transparentMaterials;
	int nCutFloor;
	ZSKIP
	ZSKIP
	EHSRMode hsrMode;
	ZSKIP
	CVec3 vDefaultClearColor;
	ETransparentMode trMode;
	CObj<CFuncBase<SFBTransform> > pIdentityTransform;
	CDBPtr<NDb::SAmbientLight> pPrevLight;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pHaze;
	ZSKIP
	ESceneRenderMode renderMode;
	bool bForceFastest;
	ZSKIP
	std::vector<CPtr<CRenderNode> > precacheObjects;
	CObj<CFuncBase<STime> > pPrevLightTime;
	std::vector<CPtr<CObjectFader> > faders;
	CObj<CObjectBase> pRain;
	CObj<SDepthOfField> pDepthOfField;
	CObj<ISkyDome> pSkyDome;
	CArray2D<NGfx::SPixel8888> fogColors;
	bool bIsTwilight;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pScene); f.Add(3,&nodes); f.Add(5,&pMaterials); f.Add(6,&colorMaterials); f.Add(7,&transparentMaterials); f.Add(8,&nCutFloor); f.Add(11,&hsrMode); f.Add(13,&vDefaultClearColor); f.Add(14,&trMode); f.Add(15,&pIdentityTransform); f.Add(16,&pPrevLight); f.Add(17,&pHaze); f.Add(19,&renderMode); f.Add(20,&bForceFastest); f.Add(22,&precacheObjects); f.Add(23,&pPrevLightTime); f.Add(24,&faders); f.Add(25,&pRain); f.Add(26,&pDepthOfField); f.Add(27,&pSkyDome); f.Add(28,&fogColors); f.Add(29,&bIsTwilight); return 0; }

	bool bWaitLoading;
	CObj<ILoadingCounter> pWaitLoading, pCalcTerrain;

	/*void AddSkinModelPart( CRenderNode *pRes, int nPart, const NDb::SGeometry *pGeometry, 
		const NDb::SMaterial *pMaterial, CFuncBase<SSkeletonMatrices> *pAnimation, 
		CFuncBase<vector<NGfx::SCompactTransformer> > *pMMXAnimation, const SFullRoomInfo &_r );*/
	CRenderNode* NewRenderNode();

	CObjectBase* CreateParticles( bool bIsDynamic, int nPFlags, bool bCastShadows, bool bTreeCrown, CPtrFuncBase<CParticleEffect> *pEffect,
		CFuncBase<SFBTransform> *pPlacement, const SBound &bound, const SRoomInfo &_g, IFader *pFader );
	CObjectBase* TrueCreateParticles( bool _bIsDynamic, const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, CFuncBase<SFBTransform> *pPlacement, const SRoomInfo &_g,
		IFader *pFader, CFuncBase<NAnimation::SGrannySkeletonPose> *pScAnim, NAnimation::SGrannySkeletonHandle *pSkeletonH, IParticleFilter *pFilter );
	void Draw( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, const SRTClearParams &rtClear, ERenderPath rp, int nLightOptions );
	void MakeTargetRect( CTRect<float> *pRes, const SDrawInfo &drawInfo );
	IMaterial* CreateMaterialShared( const NDb::SMaterial *p );
	IMaterial* CreateSubstituteMaterial();
	ERenderPath GetRenderPath() const;
	void TouchPrecached();
	void LoadEverythingInt();

	void LoadFogColors( const NDb::SAmbientLight *pAmbientLight );

public:
	CGameView();
	
	IGScene* GetGScene() const { return pScene; }
	virtual IMaterial *CreateMaterial( const NDb::SMaterial *pMaterial ) { return CreateMaterialShared( pMaterial ); }
	virtual IMaterial *CreateMaterial( const CVec4 &vColor, bool bDoesCastShadow );
	virtual void CreateMaterialInfo( const NDb::SMaterial *pMaterial, SMaterialCreateInfo *pRes );
	virtual void CreateMeshInfo( const NDb::SModel *pModel, SMeshInfo *pRes, bool bWholeAnimated, int nPlayer = 0, bool bIsLightmapped = false );
	virtual CObjectBase* CreateMesh( const SMeshInfo &meshInfo, const CCreateMeshTransform &meshTransform,
		const CCreateMeshBound &meshBound, const CMeshAnimStuff &animStuff, const SFullRoomInfo &_g, IFader *pFader = 0 );
	virtual CObjectBase* CreateDynamicMesh( const SMeshInfo &meshInfo, CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullRoomInfo &_g, IFader *pFader = 0 );
	virtual CObjectBase* CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, CFuncBase<SFBTransform> *pPlacement, const SRoomInfo &_g,
		IFader *pFader, CFuncBase<NAnimation::SGrannySkeletonPose> *pScAnim, NAnimation::SGrannySkeletonHandle *pSkeletonH, IParticleFilter *pFilter );
	virtual CObjectBase* CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, const SFBTransform &place, const SRoomInfo &_g,
		IFader *pFader, IParticleFilter *pFilter );
	virtual CObjectBase* CreateRain( const NDb::SParticleInstance *pInstance, CFuncBase<STime> *pTime, IParticleFilter *pFilter, const SRoomInfo &_g );
	virtual CPolyline* CreatePolyline( const std::vector<CVec3> &points, const std::vector<unsigned short> &indices, const CVec4 &color, bool bDepthTest );
	virtual CObjectBase* Precache( const NDb::SModel *pModel );
	virtual void Precache( const NDb::SEffect *pEffect );
	virtual void LoadEverything() { WaitForLoad( true ); LoadEverythingInt(); }
	virtual void Draw( const SDrawInfo &drawInfo );
	virtual CVec2 GetScreenRect();
	virtual int  GetCutFloor();
	virtual void SetCutFloor( int nFloor );
	virtual CObjectBase* AddPointLight( const CVec3 &ptColor, const CVec3 &ptOrigin, float fR );
	virtual CObjectBase* AddPointLight( CPtrFuncBase<CAnimLight> *pLight );
	virtual CObjectBase* AddFlare( CFuncBase<CVec3> *pOrigin, CFuncBase<STime> *pTime, int nFloor, float fFlareRadius, const NDb::STexture *pFlareTexture, float fOnTime, float fOffTime );
	virtual CObjectBase* AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, CFuncBase<CVec2> *pSize, const CVec2 &vMaxSize, const NDb::STexture *pTexture, int nFloor );
	virtual CObjectBase* AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, const CVec2 &sSize, const NDb::STexture *pTexture, int nFloor );
	virtual CObjectBase* AddPostFilter( const std::vector<CObjectBase*> &target, IPostProcess *pEffect );
	virtual CObjectBase* AddSpotLight( const CVec3 &ptColor, const CVec3 &ptOrigin, const CVec3 &ptDir, float fFOV, float fRadius, const NDb::STexture *pMask, bool bLightmapOnly );
	virtual CDecalTarget* CreateDecalTarget( const std::vector<CObjectBase*> &targets, const SDecalMappingInfo &_info );
	virtual CObjectBase* AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial );
	virtual CDecalFader* AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial, STime tFadeInStart, STime tFadeInEnd, STime tFadeOutStart, STime tFadeOutEnd, CFuncBase<STime> *pTime );
	virtual void SetWarFogBlend( float fBlend );
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	void SetAmbient( const NDb::SAmbientLight *pLight, bool bSelectGF2, CFuncBase<STime> *pTime );
	virtual void SetAmbient( const NDb::SAmbientLight *pLight, CFuncBase<STime> *pTime ) { SetAmbient( pLight, false, pTime ); }
	virtual void SetAmbientEffect( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime );
	virtual void SetDepthOfField( SDepthOfField *pDOFParams ) { pDepthOfField = pDOFParams; }
	virtual SDepthOfField *GetDepthOfField() { return pDepthOfField; }
	virtual ESceneRenderMode GetRenderMode() const;
	virtual void SetRenderMode( ESceneRenderMode mode );
	virtual bool TraceScene( const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps, SFullRoomInfo *pRoomInfo, CObjectBase **SFullRoomInfo, bool bOpaqueOnly );
	virtual void MakeHQShot( const SDrawInfo &drawInfo, CArray2D<NGfx::SPixel8888> *pRes );
	virtual void SetLoadingCounter( ILoadingCounter *_pWaitLoading, ILoadingCounter *_pCalcTerrain, ILoadingCounter *pCounter ) { pWaitLoading = _pWaitLoading; pCalcTerrain = _pCalcTerrain; pScene->SetLoadingCounter( pCounter ); }
	virtual void WaitForLoad( bool bWait ) { bWaitLoading = bWait; pScene->WaitForLoad( bWait ); }
	void SetHSRMode( EHSRMode m ) { hsrMode = m; }
	EHSRMode GetHSRMode() const { return hsrMode; }
	void SetTransparentMode( ETransparentMode m ) { trMode = m; }
	ETransparentMode GetTransparentMode() const { return trMode; }
	virtual void SetTwilight(bool _bIsTwilight ) { bIsTwilight = _bIsTwilight;}
	virtual void SetFreezeMode( bool mode ){ NGScene::bFreeze = mode; };
};

static SGroupInfo GetGroupInfo( const SRoomInfo &r, bool bCastShadows = true, bool bParticles = false )
{
	return SGroupInfo( r.nLightFlags, GetFloorBit( r.nFloor, bCastShadows, bParticles, r.nLODFlags ) );
}

static SFullGroupInfo GetGroupInfo( const SFullRoomInfo &r, int nPart, bool bCastShadows = true, bool bParticles = false )
{
	ASSERT( N_USERID_MASK == ( (1<<24) - 1 ) );
	nPart &= -(r.nUserID != 0); // nPart = 0 if r.nUserID == 0
	ASSERT( ( r.nUserID & ~N_USERID_MASK ) == 0 );
	int nUserID = ( r.nUserID & N_USERID_MASK ) | (nPart << 24 );
	return SFullGroupInfo( GetGroupInfo( r.room, bCastShadows, bParticles ), r.pUser, nUserID );
}
static SFullRoomInfo GetRoomInfo( const SFullGroupInfo &g )
{
	SFullRoomInfo res;
	res.room.nFloor = 0; // incorrect
	res.room.nLightFlags = g.groupInfo.nLightFlags;
	res.pUser = g.pUser;
	res.nUserID = g.nUserID;
	return res;
}

// CGameView

CGameView::CGameView()
{
	pScene = CreateScene();
	nCutFloor = N_MAX_FLOOR;
	pMaterials = new CMaterialShare;
	SetHSRMode( defaultHSRMode );
	renderMode = defaultRenderMode;
	vDefaultClearColor = CVec3( 0.25f, 0.25f, 0.25f );
	trMode = TRM_NORMAL;
	SFBTransform identity;
	Identity( &identity.forward );
	Identity( &identity.backward );
	pIdentityTransform = new CCFBTransform( identity );
	pPrevLight = 0;
	bForceFastest = false;
	bWaitLoading = true;
	bIsTwilight = false;
}

IMaterial* CGameView::CreateMaterialShared( const NDb::SMaterial *p )
{
	IMaterial *pRes = pMaterials->CreateMaterial( p );
	return pRes;
}

void CGameView::CreateMaterialInfo( const NDb::SMaterial *pMaterial, SMaterialCreateInfo *pRes )
{
	pMaterials->FillCreateMaterialInfo( pMaterial, pRes );
}

IMaterial* CGameView::CreateSubstituteMaterial()
{
	return colorMaterials.CreateMaterial( CVec3(0.8f, 0.f, 0.f) );
}

CRenderNode* CGameView::NewRenderNode()
{
	CRenderNode *pRes = new CRenderNode;
	nodes.push_back( pRes );
	return pRes;
}

IMaterial *CGameView::CreateMaterial( const CVec4 &vColor, bool bDoesCastShadow ) 
{ 
	if ( vColor.a == 1 )
		return colorMaterials.CreateMaterial( CVec3( vColor.x, vColor.y, vColor.z ) );
	return transparentMaterials.CreateMaterial( vColor, bDoesCastShadow ); 
}

void CGameView::CreateMeshInfo( const NDb::SModel *pModel, SMeshInfo *pRes, bool bWholeAnimated, int nPlayer, bool bIsLightMapped )
{
	pRes->parts.resize(0);
	if ( !pModel )
		return;
	const NDb::SGeometry *pGeometry = pModel->pGeometry;
	if (!pGeometry) {
		return;
	}
	if ( pGeometry->szModelFileRef.empty()
		? !CResourceFileOpener::DoesExist( "Geometries", GetIntResKey(pGeometry) )
		: !NGltf::DoesModelFileExist( pGeometry, pGeometry->szModelFileRef ) )
	{
		//ASSERT(0 & "Geometry);
		return;
	}

	const int nSkelInFile = (pModel->pSkeleton ? 0 : -1);
	// The external document is authoritative for its mesh-node count; the old
	// generated count may still contain values from an earlier GR2 export.
	const int nMeshCount = !pGeometry->szModelFileRef.empty()
		? NGltf::GetMeshCount( pGeometry, pGeometry->szModelFileRef, pGeometry->szRootMesh )
		: pGeometry->nNumMeshes;

	bool bRigidGltfModel = false;
	if ( !pGeometry->szModelFileRef.empty() && pModel->pSkeleton &&
		!pModel->pSkeleton->szModelFileRef.empty() && !pModel->pSkeleton->szRootJoint.empty() )
	{
		const NGltf::TGltfFilePtr skeletonFile = NGltf::LoadFile( pModel->pSkeleton,
			pModel->pSkeleton->szModelFileRef );
		bRigidGltfModel = skeletonFile && skeletonFile->asset.skins.empty();
	}

	const std::vector<int> &materialQuantities = pGeometry->materialQuantities;
	if ( materialQuantities.empty() )
	{
		// B2-ish way: 1 to 1 correspondence between meshes and materials.
		// (And for any mesh that is out of correspondence last material in the list is set.
		//
		for ( int nMesh = 0; nMesh < nMeshCount; ++nMesh )
		{
			const int nModelMaterial = (std::min<int>)( nMesh, pModel->materials.size() - 1 );
			if ( pModel->materials.empty() || ( pModel->materials[nModelMaterial] == 0 ) )
			{
				continue;
			}
			SPartInfo &p = pRes->parts.emplace_back();
			SPartAndSkeletonKey key( pGeometry, nMesh, -1, pModel->pSkeleton, nSkelInFile, bIsLightMapped ? 1 : 0 );
			p.pGeometry = shareGrannyMeshes.Get( key );
			p.pMaterial = CreateMaterial( pModel->materials[nModelMaterial] );
			// Every node can be an animated parent in a skinless GLTF hierarchy,
			// so legacy per-mesh flags must not route rigid parts as static geometry.
			p.bAnimated = bWholeAnimated || bRigidGltfModel;
		}
	}
	else
	{
		// Multiple materials per mesh way
		//
		// In normal situation pGeometry->nNumMeshes must be equal to materialQuantities.size(),
		// but we will survive if it's not.

		int nModelMaterial = 0;
		for ( int nMesh = 0; nMesh < nMeshCount; ++nMesh )
		{
			if ( nMesh < materialQuantities.size() && materialQuantities[nMesh] )
			{
				for ( int nMaterial = 0; nMaterial < materialQuantities[nMesh]; ++nMaterial )
				{
					SPartInfo &p = pRes->parts.emplace_back();
					SPartAndSkeletonKey key( pGeometry, nMesh, nMaterial, pModel->pSkeleton, nSkelInFile, bIsLightMapped ? 1 : 0 );
					p.pGeometry = shareGrannyMeshes.Get( key );
					if ( nModelMaterial < pModel->materials.size() )
					{
						if ( pModel->materials[nModelMaterial] )
							p.pMaterial = CreateMaterial( pModel->materials[nModelMaterial] );

						/*if ( ( nMesh < pModel->pGeometry->meshNames.size() ) &&
							( strncmp( pModel->pGeometry->meshNames[nMesh].c_str(), SZ_REPLACE_MATERIAL_PREFIX.c_str(), SZ_REPLACE_MATERIAL_PREFIX.size() ) == 0 ) )
						p.pMaterial = CreateMaterial( GetReplacedMaterial( nPlayer ) );
						else if ( pModel->materials[nModelMaterial] )
							p.pMaterial = CreateMaterial( pModel->materials[nModelMaterial] );*/

						++nModelMaterial;
					}
					if ( !p.pMaterial )
					{
						p.pMaterial = CreateSubstituteMaterial();
					}
					p.nOrigMeshIndex = nMesh;

					if ( nMesh < pGeometry->meshAnimated.size() )
						p.bAnimated = pGeometry->meshAnimated[nMesh] || bRigidGltfModel;
					else
						p.bAnimated = bWholeAnimated || bRigidGltfModel;

					if ( nMesh < pGeometry->meshWindAffected.size() )
						p.bWindAffected = pGeometry->meshWindAffected[nMesh];

					if( p.bWindAffected ) p.pMaterial = p.pMaterial->GetWindAffected();

					ASSERT(p.pMaterial);
				}
			}
			else
			{
				SPartInfo &p = pRes->parts.emplace_back();
				SPartAndSkeletonKey key( pGeometry, nMesh, -1, pModel->pSkeleton, nSkelInFile, bIsLightMapped ? 1 : 0 );
				p.pGeometry = shareGrannyMeshes.Get( key );
				p.pMaterial = CreateSubstituteMaterial();

				p.nOrigMeshIndex = nMesh;

				if ( nMesh < pGeometry->meshAnimated.size() )
					p.bAnimated = pGeometry->meshAnimated[nMesh] || bRigidGltfModel;
				else
					p.bAnimated = bWholeAnimated || bRigidGltfModel;

				if ( nMesh < pGeometry->meshWindAffected.size() )
					p.bWindAffected = pGeometry->meshWindAffected[nMesh];

				if( p.bWindAffected ) p.pMaterial = p.pMaterial->GetWindAffected();
				//if( !p.bReceiveShadows ) p.pMaterial = p.pMaterial->GetNoReceiveShadows();

				ASSERT(p.pMaterial);
			}
		}
	}
	//else
	//{
	//	ASSERT( 0 && "pGeometry->nNumMeshes doesn't match pGeometry->materialQuantities.size()" );
	//}
}

CObjectBase* CGameView::CreateDynamicMesh( const SMeshInfo &meshInfo, CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullRoomInfo &_g, IFader *pFader )
{
	CRenderNode *pRes = NewRenderNode();
	for ( int k = 0; k < meshInfo.parts.size(); ++k )
	{
		const SPartInfo &p =meshInfo.parts[k];
		if ( !p.pMaterial || !p.pGeometry )
			continue;

		CPtr<CPtrFuncBase<CObjectInfo> > pGeom;
		if ( pPrevLight && pPrevLight->pHeightFog )
			pGeom = CreateHeightFogHolder( p.pGeometry, pPrevLight->pHeightFog, pPlacement );
		else
			pGeom = p.pGeometry;

		CObjectBase *pPart = pScene->CreateDynamicGeometry( pGeom, pPlacement, p.pMaterial, pBound, hintBV, GetGroupInfo(_g, k) );
		pRes->AddPart( pPart );
	}
	if ( pFader )
	{
		CObjectFader *pObjFader = new CObjectFader( pRes, pFader, 0 );
		faders.push_back( pObjFader );
		return pObjFader;
	}
	return pRes;
}

CDecalTarget* CGameView::CreateDecalTarget( const std::vector<CObjectBase*> &targets, const SDecalMappingInfo &_info )
{
	return pScene->CreateDecalTarget( targets, _info );
}

CObjectBase* CGameView::AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial )
{
	return pScene->AddDecal( pTarget, pMaterials->CreateMaterial( pMaterial ) );
}

CDecalFader* CGameView::AddDecal( NGScene::CDecalTarget *pTarget, const NDb::SMaterial *pMaterial, STime tFadeInStart,
								 STime tFadeInEnd, STime tFadeOutStart, STime tFadeOutEnd, CFuncBase<STime> *pTime )
{
	return new CDecalFader( AddDecal( pTarget, pMaterial ), tFadeInStart,  tFadeInEnd,  tFadeOutStart, tFadeOutEnd, pTime );
}

void CGameView::SetWarFogBlend( float fBlend )
{
	pScene->SetWarFogBlend( fBlend );
}

void CGameView::SetWarFog( const CArray2D<unsigned char> &fog, float fScale )
{
	pScene->SetWarFog( fog, fScale );
}

CCreateMeshBound::CCreateMeshBound( CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, const NDb::SModel *pModel )
{
	pBounder = 0;
	bound.BoxExInit( VNULL3, VNULL3 );
	hintBV = MakeLargeHintBound();
	Create( pAnimation, pModel );
}

void CCreateMeshBound::Create( CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, const NDb::SModel *pModel )
{
	if ( pAnimation && pModel && pModel->pGeometry )
	{
		SBound bv;
		bv.BoxExInit( pModel->pGeometry->vCenter, pModel->pGeometry->vSize * 0.5f );
		pBounder = new CAnimatedBound( bv, pAnimation );
		hintBV = MakeLargeHintBound();
	}
}

CObjectBase* CGameView::CreateMesh( const SMeshInfo &meshInfo, const CCreateMeshTransform &meshTransform,
	const CCreateMeshBound &_meshBound, const CMeshAnimStuff &animStuff, const SFullRoomInfo &_g, IFader *pFader )
{
	if ( meshInfo.parts.empty() )
		return 0;

	CPtr<CObjectBase> pPart;
	CPtr<CBind> pBind;
	CPtr< CFuncBase<std::vector<NGfx::SCompactTransformer> > > pMMXAnimation;

	CCreateMeshBound meshBound = _meshBound;
	if ( !meshBound.GetBounder() && ( meshBound.GetBound().s.fRadius == 0.0f ) )
		meshBound.Create( animStuff.GetAnimation(), animStuff.GetModel() );
	// we suppose, that if animation present, then object is potentially animated (CPtr to avoid memory leaks else)
	if ( animStuff.GetAnimation() && animStuff.GetModel() && animStuff.GetModel()->pSkeleton )
	{
		pBind = new CBind( animStuff.GetAnimation(), animStuff.GetModel()->pSkeleton, 0 );
		pMMXAnimation = MakeMMXAnimation( pBind );
	}

	CRenderNode *pRes = NewRenderNode();
	for ( int k = 0; k < meshInfo.parts.size(); ++k )
	{
		const SPartInfo &p = meshInfo.parts[k];
		if ( !p.pMaterial || !p.pGeometry )
			continue;

		CPtr<CPtrFuncBase<CObjectInfo> > pGeom;
		if ( pPrevLight && pPrevLight->pHeightFog )
		{
			if ( meshTransform.GetTransformer() )
				pGeom = CreateHeightFogHolder( p.pGeometry, pPrevLight->pHeightFog, meshTransform.GetTransformer() );
			else
				pGeom = CreateHeightFogHolder( p.pGeometry, pPrevLight->pHeightFog, meshTransform.GetPlace() );
		}
		else
			pGeom = p.pGeometry;

		if(p.bWindAffected)
		{
			if ( meshTransform.GetTransformer() )
				pGeom = CreateDeformerHolder( pGeom, meshTransform.GetTransformer() );
			else
				pGeom = CreateDeformerHolder( pGeom, meshTransform.GetPlace() );
		}

		if ( !( p.bAnimated ) || !pMMXAnimation )
		{
			if ( meshTransform.GetTransformer() )
			{
				if ( meshBound.GetBounder() )
					pPart = pScene->CreateGeometry( pGeom, p.pMaterial, meshTransform.GetTransformer(), meshBound.GetBounder(), _meshBound.GetHintBV(), GetGroupInfo(_g, k) );				
				else
					pPart = pScene->CreateGeometry( pGeom, p.pMaterial, meshTransform.GetTransformer(), _meshBound.GetHintBV(), GetGroupInfo(_g, k) );				
			}
			else
			{
				pPart = pScene->CreateGeometry( pGeom, p.pMaterial, meshTransform.GetPlace(), GetGroupInfo(_g, k) );
			}
		}
		else
		{
			if ( meshBound.GetBounder() )
				pPart = pScene->CreateGeometry( pGeom, p.pMaterial, pBind, pMMXAnimation, meshBound.GetBounder(), _meshBound.GetHintBV(), GetGroupInfo(_g, k) );
			else
				pPart = pScene->CreateGeometry( pGeom, p.pMaterial, pBind, pMMXAnimation, meshBound.GetBound(), GetGroupInfo(_g, k) );
		}

		pRes->AddPart( pPart );
	}

	if ( pRes && ( pFader || animStuff.GetTransparencyAnimations() ) )
	{
		CObjectFader *pObjFader = new CObjectFader( pRes, pFader ? pFader : NGScene::CreateSimpleFader(), animStuff.GetTransparencyAnimations() );
		faders.push_back( pObjFader );
		return pObjFader;
	}

	return pRes;
}

CObjectBase* CGameView::CreateParticles( bool bIsDynamic, int nPFlags, bool bCastShadows, bool bCrown, CPtrFuncBase<CParticleEffect> *pEffect, 
	CFuncBase<SFBTransform> *pPlacement, const SBound &bound, const SRoomInfo &_r, IFader *pFader )
{
	SBound hintBV = MakeLargeHintBound();
	if ( bIsDynamic )
		return pScene->CreateParticles( pEffect, pPlacement, hintBV, hintBV, GetGroupInfo( _r, bCastShadows, bCrown ), nPFlags );
	else
		return pScene->CreateParticles( pEffect, pPlacement, bound, hintBV, GetGroupInfo( _r, bCastShadows, bCrown ), nPFlags );
}

template<class T>
static void InitParticleTextures( T *pAnimator, const NDb::SParticleInstance *pInstance, CFuncBase<CVec3> *pColor = 0 )
{
	pAnimator->textureIDs.resize( pInstance->textures.size() );
	int nLast = -1;
	for ( int i = 0; i < pAnimator->textureIDs.size(); ++i )
	{
		if ( !pInstance->textures[i] )
			continue;
		nLast = i;

		// We needn't lit add type textures
		if ( pInstance->textures[i]->eConversionType == NDb::CONVERT_TRANSPARENT )
		{
			CPtr<CParticleTexture> pParticleTexture = shareParticleTextures.Get( pInstance->textures[i] );
			pParticleTexture->SetColor( pColor );
			pAnimator->textureIDs[i] = pParticleTexture;
		}
		else
			pAnimator->textureIDs[i] = shareTextures.Get( STextureKey( pInstance->textures[i], STextureKey::TK_TRANSPARENT ) );
	}
	pAnimator->textureIDs.resize( nLast + 1 );
}

CObjectBase* CGameView::TrueCreateParticles( bool _bIsDynamic, const NDb::SEffect *pEffect, STime stBeginTime, 
	CFuncBase<STime> *pTime, CFuncBase<SFBTransform> *pPlacement, const SRoomInfo &_g, IFader *pFader,
	CFuncBase<NAnimation::SGrannySkeletonPose> *pScAnim, NAnimation::SGrannySkeletonHandle *pSkeletonH, IParticleFilter *pFilter )
{
	CPtr< CFuncBase<SFBTransform> > pTransformHolder( pPlacement );
	CPtr<CFuncBase<NAnimation::SGrannySkeletonPose> > pAnimatorHolder( pScAnim );
	if ( pEffect->instances.empty() && pEffect->lights.empty() && pEffect->models.empty() )
		return 0;

	CRenderNode *pRes = NewRenderNode();

	// particle parts
	for ( int i = 0; i < pEffect->instances.size(); ++i )
	{
		const NDb::SParticleInstance *pInstance = pEffect->instances[i];
		if ( !pInstance )
			continue;
		const NDb::SParticle *pParticle = pInstance->pParticle;
		if ( !pParticle )
			continue;

		CMSRNode *pMSR = new CMSRNode;
		if ( pScAnim && pInstance->nGlueToBone )
		{
			ASSERT( pSkeletonH && "if you provide an animator, provide a skeleton handle too" );
			if ( CDynamicCast<NAnimation::IGetBone> pGetBone = pScAnim )
			{
				char buf[128];
				sprintf( buf, "Effect%d", pInstance->nGlueToBone );
				int nBone = pGetBone->GetBoneIndex( buf );
				NAnimation::CAddBoneFilter *pABF = new NAnimation::CAddBoneFilter( pScAnim, *pSkeletonH, nBone );
				pMSR->pAncestor = pABF;
			}
			else
			{
				ASSERT( 0 && "animator does not support IGetBone" );
				continue;
			}
		}
		else if ( !IsValid( pPlacement ) )
			continue;
		else
			pMSR->pAncestor = pPlacement;

		{
			SFBTransform trans;
			CVec3 scale = CVec3( pInstance->fScale, pInstance->fScale, pInstance->fScale );
			MakeMatrix( &trans.forward, pInstance->vPosition, pInstance->qRotation, scale );
			trans.backward.HomogeneousInverse( trans.forward );
			CCFBTransform *pTrans = new CCFBTransform( trans );
			pMSR->pPos = pTrans;
		}

		SBound particleBound = pParticle->bound;
		if ( pParticle->vWrapSize.x != 0 ) // IsWrapping()
			particleBound.BoxInit( CVec3(0,0,0), CVec3(1000, 1000, 50 ) );

		CParticleAnimator *pAnimator = new CParticleAnimator( pInstance, stBeginTime, pFilter,
			pTime, shareParticles.Get( GetIntResKey( pParticle ) ), pMSR,
			pInstance->bLeaveParticlesWhereStarted, pParticle->bPerParticleFog );
		InitParticleTextures( pAnimator, pInstance, pScene->GetParticlesLightColor() );

		int nPFlags = 0;
		if ( pInstance->eLight == NDb::SParticleInstance::L_LIT )
			nPFlags |= PF_LIT;
		else
			nPFlags |= PF_SELF_ILLUM;
		if ( pInstance->eStatic == NDb::SParticleInstance::P_STATIC && !_bIsDynamic )
			nPFlags |= PF_STATIC;
		else
			nPFlags |= PF_DYNAMIC;
		pRes->AddPart( CreateParticles( _bIsDynamic, nPFlags, pInstance->bDoesCastShadow, pInstance->bIsCrown, pAnimator, pMSR, particleBound, _g, pFader ) );
	}

	// light parts
	for ( int i = 0; i < pEffect->lights.size(); ++i )
	{
		const NDb::SLightInstance *pInstance = pEffect->lights[i];
		if ( !pInstance )
			continue;

		const NDb::SAnimLight *pLight = pInstance->pLight;
		if ( !pLight )
			continue;

		CMSRNode *pMSR = new CMSRNode;
		if ( pScAnim && pInstance->nGlueToBone )
		{
			ASSERT( pSkeletonH && "if you provide an animator, provide a skeleton handle too" );
			if ( CDynamicCast<NAnimation::IGetBone> pGetBone = pScAnim )
			{
				char buf[128];
				sprintf( buf, "Effect%d", pInstance->nGlueToBone );
				int nBone = pGetBone->GetBoneIndex( buf );
				NAnimation::CAddBoneFilter *pABF = new NAnimation::CAddBoneFilter( pScAnim, *pSkeletonH, nBone );
				pMSR->pAncestor = pABF;
			}
			else
			{
				ASSERT( 0 && "animator does not support IGetBone" );
				continue;
			}
		}
		else
			pMSR->pAncestor = pPlacement;

		{
			SFBTransform trans;
			CVec3 scale = CVec3( pInstance->fScale, pInstance->fScale, pInstance->fScale );
			MakeMatrix( &trans.forward, pInstance->vPosition, pInstance->qRotation, scale );
			trans.backward.HomogeneousInverse( trans.forward );
			CCFBTransform *pTrans = new CCFBTransform( trans );
			pMSR->pPos = pTrans;
		}

		CLightAnimator *pAnimator = new NGScene::CLightAnimator( pInstance, stBeginTime, pInstance->fScale );
		pAnimator->pInfo = shareAnimLights.Get( pLight );
		pAnimator->pTime = pTime;
		pAnimator->pPlacement = pMSR;

		pRes->AddPart( pScene->AddPointLight( pAnimator ) );
	}

	// 3D geometry parts
	for ( int i = 0; i < pEffect->models.size(); ++i )
	{
		const NDb::SModelInstance *pInstance = pEffect->models[i];
		ASSERT( pInstance->pModel );
		if ( !pInstance->pModel )
		{
			continue;
		}

		CMSRNode *pMSR = new CMSRNode;
		if ( pScAnim && pInstance->nGlueToBone )
		{
			ASSERT( pSkeletonH && "if you provide an animator, provide a skeleton handle too" );
			if ( CDynamicCast<NAnimation::IGetBone> pGetBone = pScAnim )
			{
				char buf[128];
				sprintf( buf, "Effect%d", pInstance->nGlueToBone );
				int nBone = pGetBone->GetBoneIndex( buf );
				NAnimation::CAddBoneFilter *pABF = new NAnimation::CAddBoneFilter( pScAnim, *pSkeletonH, nBone );
				pMSR->pAncestor = pABF;
			}
			else
			{
				ASSERT( 0 && "animator does not support IGetBone" );
				continue;
			}
		}
		else
			pMSR->pAncestor = pPlacement;

		{
			SFBTransform trans;
			const CVec3 scale( pInstance->fScale, pInstance->fScale, pInstance->fScale );
			MakeMatrix( &trans.forward, pInstance->vPosition, pInstance->qRotation, scale );
			trans.backward.HomogeneousInverse( trans.forward );
			CCFBTransform *pTrans = new CCFBTransform( trans );
			pMSR->pPos = pTrans;
		}

		{
			const NDb::SModel *pModel = pInstance->pModel;
			const NDb::SAnimBase *pSkelAnim = pInstance->pSkelAnim;

			SFullRoomInfo fullRoomInfo(_g, 0, 0);

			SMeshInfo meshInfo;

			if ( pModel->pSkeleton == 0 || pSkelAnim == 0 )
			{
				CreateMeshInfo( pModel, &meshInfo, false );
				pRes->AddPart( CreateMesh( meshInfo, pMSR, 0, 0, fullRoomInfo, pFader ) );
			}
			else
			{
				CreateMeshInfo( pModel, &meshInfo, true );

				// FIXME: не поддерживается SModelInstance::fCycleLength
				//

				const NAnimation::SGrannySkeletonHandle skelId( pModel->pSkeleton, 0 );
				const NAnimation::SAnimHandle animId( pSkelAnim, 0 );

				NAnimation::ISkeletonAnimator *pSkelAnimator = CreateSkeletonAnimator( skelId, pTime );
				const STime tStartTime = stBeginTime + STime(pInstance->fOffset * 1000.f);

				NAnimation::ISkeletonAnimator::SAnimID internalId = pSkelAnimator->AddAnimation( tStartTime, animId, false, pSkelAnim->GetSpeedFactor(), 1.0f );
				if ( internalId != (-1) )
				{
					pSkelAnimator->SetLoopCount( internalId, pInstance->nCycleCount );
				}
				else
				{
					csSystem << CC_RED << "(3Dmotor:TrueCreateParticles) Couldn't add animation \"" << (animId.pAnimFile ? animId.pAnimFile->GetDBID().ToString() : "NULL") << "\"" << endl;
				}
				pSkelAnimator->SetGlobalTransform( pMSR );

				// FIXME: давать свой SBound, когда возможно, это ускорит рассчёт того, что в кадр попадает
				std::vector<CPtr<CFuncBase<float> > > transparencyAnimators;
				CreateAnimatedTransparencyChannels( &transparencyAnimators, meshInfo, pModel, pSkelAnimator );
 				pRes->AddPart( CreateMesh( meshInfo, 0, 0, NGScene::CMeshAnimStuff( pModel, pSkelAnimator, &transparencyAnimators ), fullRoomInfo, pFader ) );
			}
		}
	}

	if ( pRes && pFader )
	{
		CObjectFader *pObjFader = new CObjectFader( pRes, pFader, 0 );
		faders.push_back( pObjFader );
		return pObjFader;
	}

	return pRes;
}

CObjectBase* CGameView::CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, 
	CFuncBase<STime> *pTime, CFuncBase<SFBTransform> *pPlacement, const SRoomInfo &_r, IFader *pFader,
	CFuncBase<NAnimation::SGrannySkeletonPose> *pScAnim, NAnimation::SGrannySkeletonHandle *pSkeletonH, IParticleFilter *pFilter )
{
	return TrueCreateParticles( true, pEffect, stBeginTime, pTime, pPlacement, _r, pFader, pScAnim, pSkeletonH, pFilter );
}

CObjectBase* CGameView::CreateParticles( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime, 
	const SFBTransform &place, const SRoomInfo &_g, IFader *pFader, IParticleFilter *pFilter )
{
	return TrueCreateParticles( false, pEffect, stBeginTime, pTime, new CCFBTransform( place ), _g, pFader, 0, 0, pFilter );
}

CObjectBase* CGameView::CreateRain( const NDb::SParticleInstance *pInstance, CFuncBase<STime> *pTime, IParticleFilter *pFilter, const SRoomInfo &_g )
{
	const NDb::SParticle *pParticle = pInstance->pParticle;
	if ( !pParticle )
		return 0;
	CRainAnimator *pAnimator = new NGScene::CRainAnimator( pTime, pScene->GetCamera(), pFilter );
	InitParticleTextures( pAnimator, pInstance );
	int nPFlags = 0;
	if ( pInstance->eLight == NDb::SParticleInstance::L_LIT )
		nPFlags |= PF_LIT;
	else
		nPFlags |= PF_SELF_ILLUM;
	nPFlags |= PF_DYNAMIC;
	SBound particleBound;
	particleBound.BoxInit( CVec3(0,0,0), CVec3(1000, 1000, 50 ) );
	return CreateParticles( true, nPFlags, pInstance->bDoesCastShadow, pInstance->bIsCrown, pAnimator, pIdentityTransform, particleBound, _g, 0 );
}

CPolyline* CGameView::CreatePolyline( const std::vector<CVec3> &points, const std::vector<unsigned short> &indices,
	const CVec4 &color, bool bDepthTest )
{
	return pScene->CreatePolyline( new CMemGeometry( points ), indices, color, bDepthTest );
}

template<class T>
inline CResourcePrecache<T>* MakePrecache( T *p ) { return new NGScene::CResourcePrecache<T>( p ); }

CObjectBase* CGameView::Precache( const NDb::SModel *pModel )
{
	SMeshInfo meshInfo;
	CreateMeshInfo( pModel, &meshInfo, false, 0 );

	CRenderNode *pRes = new CRenderNode;
	for ( int nPart = 0; nPart < meshInfo.parts.size(); ++nPart )
	{
		if ( !meshInfo.parts[nPart].pGeometry )
			continue;

		pRes->AddPart( MakePrecache( meshInfo.parts[nPart].pGeometry.GetPtr() ) );
	}

	for ( int nMaterial = 0; nMaterial < pModel->materials.size(); ++nMaterial )
	{
		CPtr<IMaterial> pMaterial = CreateMaterial( pModel->materials[nMaterial] );
		pMaterial->Precache();
	}

	precacheObjects.push_back( pRes );
	return pRes;
}

void CGameView::Precache( const NDb::SEffect *pEffect )
{
	CRenderNode *pRes = new CRenderNode;

		// Particle parts.
	for ( int i = 0; i < pEffect->instances.size(); ++i )
	{
		const NDb::SParticleInstance *pInstance = pEffect->instances[i];
		const NDb::SParticle *pParticle = pInstance->pParticle;
		if ( !pParticle )
			continue;

		pRes->AddPart( MakePrecache( shareParticles.Get( GetIntResKey( pParticle ) ) ) );
	}

		// Light parts.
	for ( int i = 0; i < pEffect->lights.size(); ++i )
	{
		const NDb::SLightInstance *pInstance = pEffect->lights[i];
		const NDb::SAnimLight *pLight = pInstance->pLight;
		if ( !pLight )
			continue;
		pRes->AddPart( MakePrecache( shareAnimLights.Get( pLight ) ) );
	}
	precacheObjects.push_back( pRes );

		// 3D geometry parts.
	for ( int i = 0; i < pEffect->models.size(); ++i )
	{
		const NDb::SModelInstance *pInstance = pEffect->models[i];
		ASSERT( pInstance->pModel );
		if ( pInstance->pModel )
		{
			Precache( pInstance->pModel );
		}
	}
}

void CGameView::TouchPrecached()
{
	for ( std::vector<CPtr<CRenderNode> >::iterator i = precacheObjects.begin(); i != precacheObjects.end(); )
	{
		CRenderNode *p = *i;
		bool bOk = true;
		if ( IsValid( p ) )
		{
			for ( int k = 0; k < p->parts.size(); ++k )
			{
				CDynamicCast<IPrecache> pPrecache( p->parts[k] );
				if ( pPrecache )
					bOk &= pPrecache->TryUpdate();
			}
		}
		if ( bOk )
			i = precacheObjects.erase( i );
		else
			++i;
	}
}

class CCountCalcTerrain : public ILoadingCounter
{
	OBJECT_NOCOPY_METHODS(CCountCalcTerrain);
	int nTerrains, nTotal;
	CPtr<ILoadingCounter> pForward;
public:
	CCountCalcTerrain() : pForward(0), nTerrains( 0 ), nTotal( 0 ) { }
	virtual void LeftToLoad( int nCount )
	{
		nTerrains = nTotal - nCount;
		if ( pForward )
			pForward->LeftToLoad( nCount );
	}
	virtual void SetTotalCount( int nTotalCount )
	{
		nTotal = nTotalCount;
		if ( pForward )
			pForward->SetTotalCount( nTotalCount );
	}
	virtual void Step()
	{
		++nTerrains;
		if ( pForward )
			pForward->Step();
	}
	void Forward( ILoadingCounter *_pForward ) 
	{ 
		nTotal = nTerrains; 
		nTerrains = 0; 
		pForward = _pForward; 
		pForward->SetTotalCount( nTotal ); 
	}
};
void CGameView::LoadEverythingInt()
{
	if ( bWaitLoading )
	{
		CObj<CCountCalcTerrain> pTerrainCounter = new CCountCalcTerrain();
		pScene->PrecacheMaterials( pTerrainCounter );
		TouchPrecached();
		if ( pWaitLoading )
			pWaitLoading->SetTotalCount( CountFileRequestsInFly() );
		while ( HasFileRequestsInFly() )
		{
			if ( pWaitLoading )
				pWaitLoading->LeftToLoad( CountFileRequestsInFly() );
			std::this_thread::yield();
		}
		MarkNewDGFrame();
		if ( pCalcTerrain )
			pTerrainCounter->Forward( pCalcTerrain );
		pScene->PrecacheMaterials( pTerrainCounter );
		TouchPrecached();
		ASSERT( precacheObjects.empty() );
		bWaitLoading = false;
	}

	pScene->LoadEverything();
}

void CGameView::LoadFogColors( const NDb::SAmbientLight *pAmbientLight )
{
	if ( pAmbientLight && 
			 pAmbientLight->pDistanceFog && 
		   pAmbientLight->pDistanceFog->pColorTexture &&
			 pAmbientLight->pDistanceFog->pColorTexture->eFormat == NDb::STexture::TF_8888
			 )
	{
		CDGPtr<CPtrFuncBase<CSWTextureData> > pTextureDataFunc = NGScene::GetSWTex( pAmbientLight->pDistanceFog->pColorTexture );
		pTextureDataFunc.Refresh();
		CSWTextureData * pTextureData = pTextureDataFunc->GetValue();
		fogColors = pTextureData->mips[0];
	}
	else
		fogColors.Clear();

}

void CGameView::Draw( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, const SRTClearParams &rtClear, 
	ERenderPath rp, int nLightOptions )
{
	LoadEverythingInt();
	TouchPrecached();

	// Compact surviving faders in place so removal stays linear for the vector.
	std::vector<CPtr<CObjectFader> >::iterator writeFader = faders.begin();
	for ( std::vector<CPtr<CObjectFader> >::iterator readFader = faders.begin(); readFader != faders.end(); ++readFader )
	{
		if ( IsValid( *readFader ) && (*readFader)->Update( (void *)0 ) )
		{
			if ( writeFader != readFader )
				*writeFader = *readFader;
			++writeFader;
		}
	}
	faders.erase( writeFader, faders.end() );

	//for ( list<CPtr<CGrassTracker> >::iterator i = grassTrackers.begin(); i != grassTrackers.end(); ++i )
	//	(*i)->Update();
	nodes.clear();
	SGroupSelect mask( GetFloorMask( nCutFloor ), GetParticlesRequireFlag( bShowParticles ) );
	NGfx::CCubeTexture *pSky = 0;
	CDGPtr<CPtrFuncBase<NGfx::CCubeTexture> > pSkyNode( pMaterials->GetSky() );
	if ( pSkyNode )
	{
		pSkyNode.Refresh();
		pSky = pSkyNode->GetValue();
	}

	NGfx::SFogParams fogParams;
	if ( pPrevLight && pPrevLight->pDistanceFog )
	{
		const NDb::SDistanceFog *p = pPrevLight->pDistanceFog;
		fogParams.fMaxDist = p->fMaxDist;
		fogParams.fMinDist = p->fMinDist;
		fogParams.fMaxZDis = p->fMaxZDis;
		fogParams.fMinZDis = p->fMinZDis;

		fogParams.vColor = p->vColor;
		
		// Fog color should be setted from texture
		if ( fogColors.GetSizeX() > 0 )
		{
			// Calculating camera direction
			SHMatrix matTransform = pTS->GetProjection().backward * pTS->Get().forward;
			CVec3 vCameraDirection = matTransform.GetZAxis3();
			Normalize( &vCameraDirection ); // It is not necessary but ...

			float fYaw = atan2( -vCameraDirection.y, vCameraDirection.x );
			float fPitch = -atan2( vCameraDirection.z, sqrt(sqr(vCameraDirection.x) + sqr(vCameraDirection.y)) );

			if ( fYaw < 0 )
				fYaw += FP_2PI;

			// Calculating color by influence of 4 points
			float fX = (fYaw / FP_2PI)*(fogColors.GetSizeX()-1);
			float fY = ((fPitch + FP_PI2)/ FP_PI*(fogColors.GetSizeY() - 1));

			int nX = fX;
			int nY = fY;
			int nX1 = (std::min)( nX+1, fogColors.GetSizeX()-1 );
			int nY1 = (std::min)( nY+1, fogColors.GetSizeY()-1 );

			fX -= nX;
			fY -= fY;

			CVec3 vColor( fogColors[nY][nX].r/255.0f, fogColors[nY][nX].g/255.0f, fogColors[nY][nX].b/255.0f );
			CVec3 vColorX( fogColors[nY][nX1].r/255.0f, fogColors[nY][nX1].g/255.0f, fogColors[nY][nX1].b/255.0f );
			CVec3 vColorY( fogColors[nY1][nX].r/255.0f, fogColors[nY1][nX].g/255.0f, fogColors[nY1][nX].b/255.0f );
			CVec3 vColorXY( fogColors[nY1][nX1].r/255.0f, fogColors[nY1][nX1].g/255.0f, fogColors[nY1][nX1].b/255.0f );

			vColor = vColor*(1.0f-fX)*(1.0f-fY) + vColorX*fX*(1.0f-fY) + vColorY*fY*(1.0f-fX)+vColorXY*fX*fY;
			fogParams.vColor = vColor;
		}				
	}
	pRC->SetFogParams( fogParams );
	//pScene->SetTwilight(rand()&1);
	pScene->SetTwilight( bIsTwilight );
	pScene->Draw( pTS, pClipTS, pRC, mask, rp, rtClear, hsrMode, trMode, pSky, pDepthOfField, nLightOptions );
}

void CGameView::MakeTargetRect( CTRect<float> *pRes, const SDrawInfo &drawInfo )
{
	CTRect<float> &rFullScreen = *pRes;
	CVec2 vSize = pScene->GetScreenRect();
	rFullScreen.x1 = vSize.x * drawInfo.vOrigin.x; 
	rFullScreen.x2 = rFullScreen.x1 + vSize.x * drawInfo.vSize.x;
	rFullScreen.y1 = vSize.y * drawInfo.vOrigin.y; 
	rFullScreen.y2 = rFullScreen.y1 + vSize.y * drawInfo.vSize.y;
}

static ERenderPath GetRenderPath( ESceneRenderMode rm, bool bForceFastest )
{
	if ( NGfx::IsTnLDevice() )
		return RP_TNL;
	switch ( rm )
	{
		case SRM_SHOWOVERDRAW: return RP_SHOWOVERDRAW;
		case SRM_BEST: return RP_GF3_FAST;
		case SRM_SHOWLIGHTMAP: return RP_SHOWLIGHTMAP;
		case SRM_SHOWPOLYCOUNT: return RP_SHOWPOLYCOUNT;
	}
	ASSERT(0);
	return RP_GF3_FAST;
}

ERenderPath CGameView::GetRenderPath() const
{
	return NGScene::GetRenderPath( renderMode, bForceFastest );
}

static void DrawHaze( NGfx::CTexture *pTex, NGfx::CRenderContext &rc )
{
	rc.SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );

	CDynamicCast<NGfx::I2DBuffer> p2D( pTex );
	const int nXSize = p2D->GetSizeX();
	const int nYSize = p2D->GetSizeY();
	const CTRect<float> hazeRect( 0, 0, nXSize, nYSize );

	CVec2 vScrSize = NGfx::GetScreenRect();
	const CTRect<float> rDst( 0, 0, vScrSize.x, vScrSize.y );

	NGfx::C2DQuadsRenderer quadsRender;
	quadsRender.SetTarget( rc, vScrSize, NGfx::QRM_DEPTH_NONE );
	quadsRender.AddRect( rDst, pTex, hazeRect );
}

inline CVec3 GetCameraPosition( CTransformStack *pTS )
{
	CVec4 vCamPos4;
	pTS->Get().backward.RotateHVector( &vCamPos4, CVec4(0,0,0,1) );
	return CVec3( vCamPos4.x / vCamPos4.w, vCamPos4.y / vCamPos4.w, vCamPos4.z / vCamPos4.w );
}

void CGameView::Draw( const SDrawInfo &drawInfo )
{
  if ( pSkyDome && drawInfo.pTS )
    pSkyDome->SetCameraPos( GetCameraPosition( drawInfo.pTS ) );

#ifdef FADE_TEST
	{
		float fFade =sin( GetCurrentTimeMilliseconds() / 1000.0f );
		for ( int k = 0; k < fadeTestObjects.size(); ++k )
		{
			CObjectBase *p = fadeTestObjects[k];
			if ( IsValid(p) )
				SetFade( p, fFade );
		}
	}
#endif
#ifdef DEBUG_LIGHTING
	static CObj<CObjectBase> pHoldPL;
	static int nPrevWarFog;
	int nTickCount = GetCurrentTimeMilliseconds();
	if ( ( nTickCount % 37 ) == 0 )
	{
		float fX = ( rand() & 255 ) / 255.0f;
		float fY = ( rand() & 255 ) / 255.0f;
		pHoldPL = AddPointLight( CVec3(1,1,1), CVec3(10 + fX * 50,10 + fY * 50,30), 40 );
	}
	if ( nTickCount >= nPrevWarFog + 1000 )
	{
		nPrevWarFog = nTickCount;
		CArray2D<unsigned char> fog;
		fog.SetSizes( 257, 257 );
		float fTime = nTickCount;
		for ( int y = 0; y < fog.GetSizeY(); ++y )
		{
			for ( int x = 0; x < fog.GetSizeX(); ++x )
			{
				float fPeriod = ( ( x * 134 + y * 9434 ) & 255 ) * 20 + 500;
				float fShift = ( ( x * 34 + y * 134 + x * y * 13 ) & 1023 ) / 100.0f;
				fog[y][x] = sin( fTime / fPeriod + fShift ) * 60 + 180;
			}
		}
		SetWarFog( fog, 1 / 2.75f );
	}
	SetWarFogBlend( ( (nTickCount - nPrevWarFog) % 1000 ) / 1000.0f );
#endif

	ERenderPath renderPath = GetRenderPath();
	NGfx::CRenderContext rc;
	if ( drawInfo.pTarget )
		rc.SetTextureRT( drawInfo.pTarget );
	
	SRTClearParams rtClear = drawInfo.rtClear;
	if ( drawInfo.bUseDefaultClearColor )
		rtClear.vColor = CVec4( vDefaultClearColor, 1 );
	
	int nLightOptions = drawInfo.bShadows ? 0 : LO_NOSHADOWS;

	CTransformStack tsClip;
	MakeClipTS( &tsClip, *drawInfo.pTS, drawInfo.vOrigin, drawInfo.vSize );
	Draw( drawInfo.pTS, &tsClip, &rc, rtClear, renderPath, nLightOptions );

	if ( pHaze )
	{
		pHaze.Refresh();
		DrawHaze( pHaze->GetValue(), rc );
	}
}

void CGameView::MakeHQShot( const SDrawInfo &_drawInfo, CArray2D<NGfx::SPixel8888> *pRes )
{
	ERenderPath renderPath = GetRenderPath();
	

	CArray2D<NGfx::SPixel8888> &shot = *pRes;
	CTRect<float> rSize;
	NGfx::GetRegisterSize( &rSize );
	int nXSize = Float2Int( rSize.x2 );
	int nYSize = Float2Int( rSize.y2 );
	const int MAG = NGlobal::GetVar( "gfx_hqshot_mag", 4 ).GetFloat();
	shot.SetSizes( nXSize * MAG, nYSize * MAG );
	for ( int dy = 0; dy < MAG; ++dy )
	{
		for ( int dx = 0; dx < MAG; ++dx )
		{
			SDrawInfo drawInfo(_drawInfo);
			drawInfo.vSize.x /= MAG;
			drawInfo.vSize.y /= MAG;
			drawInfo.vOrigin.x += dx * drawInfo.vSize.x;
			drawInfo.vOrigin.y += dy * drawInfo.vSize.y;

			if ( pSkyDome && drawInfo.pTS )
				pSkyDome->SetCameraPos( GetCameraPosition( drawInfo.pTS ) );

			//for ( int k = 0; k < 100; ++k )
			{
				NGfx::CRenderContext rc;
				rc.SetVirtualRT();
				rc.ClearTarget();
				rc.ClearBuffers(  0 );
				/*if ( drawInfo.bUseDefaultClearColor )
					NGScene::Clear( &rc, vDefaultClearColor );
				else
					NGScene::Clear( &rc, drawInfo.vClearColor );*/
				CTransformStack tsClip;
				
				MakeClipTS( &tsClip, *drawInfo.pTS, drawInfo.vOrigin, drawInfo.vSize );
				SRTClearParams rtClear = drawInfo.rtClear;
				if ( drawInfo.bUseDefaultClearColor )
					rtClear.vColor = CVec4( vDefaultClearColor, 1 );

				Draw( &tsClip, drawInfo.pTS, &rc, rtClear, GetRenderPath(),  drawInfo.bShadows ? 0 : LO_NOSHADOWS );			
			
			
			}
			CArray2D<NGfx::SPixel8888> rt;

			NGfx::CTexture *pTex=NGfx::GetRegisterTexture( 0 );
			NGfx::GetRenderTargetData( &rt, pTex );	
			
			for ( int y = 0; y < nYSize; ++y )
			{
				for ( int x = 0; x < nXSize; ++x )
				{
					NGfx::SPixel8888 sPix;
					sPix.dwColor =  rt[y][x].dwColor | 0xff000000;
					shot[ dy * nYSize + y ][ dx * nXSize + x ] = sPix;
				}
			}
		}
	}
}

CVec2 CGameView::GetScreenRect()
{
	return pScene->GetScreenRect();
}

void CGameView::SetAmbientEffect( const NDb::SEffect *pEffect, STime stBeginTime, CFuncBase<STime> *pTime )
{
	if ( !pEffect || pEffect->lights.empty() )
	{
		pScene->SetAmbientAnimation( 0 );
		return;
	}
	//
	const NDb::SLightInstance *pInstance = pEffect->lights.front();
	const NDb::SAnimLight *pLight = pInstance->pLight;
	if ( !pLight )
		return;
	CLightAnimator *pAnimator = new NGScene::CLightAnimator( pInstance, stBeginTime, pInstance->fScale );
	pAnimator->pInfo = shareAnimLights.Get( pLight );
	pAnimator->pTime = pTime;
	SFBTransform trans;
	CVec3 scale = CVec3( pInstance->fScale, pInstance->fScale, pInstance->fScale );
	MakeMatrix( &trans.forward, pInstance->vPosition, pInstance->qRotation, scale );
	trans.backward.HomogeneousInverse( trans.forward );
	pAnimator->pPlacement = new CCFBTransform( trans );

	pScene->SetAmbientAnimation( pAnimator );
}

CObjectBase* CGameView::AddPointLight( const CVec3 &ptColor, const CVec3 &ptOrigin, float fR )
{
	if ( fR <= 0 )
		return 0;
	return pScene->AddPointLight( ptColor, ptOrigin, fR );
}

CObjectBase* CGameView::AddPointLight( CPtrFuncBase<CAnimLight> *pLight )
{
	return pScene->AddPointLight( pLight );
}

CObjectBase* CGameView::AddFlare( CFuncBase<CVec3> *pOrigin, CFuncBase<STime> *pTime, int nFloor, float fFlareRadius, const NDb::STexture *pFlareTexture, float fOnTime, float fOffTime )
{
	//fFlareRadius = 1;
	if ( fFlareRadius <= 0 || pTime == 0 || pFlareTexture == 0 )
		return 0;
	// glow in the dark
	SBound bound;
	bound.SphereInit( CVec3(0,0,0), fFlareRadius );
	return CreateParticles( true,
		PF_DYNAMIC | PF_SELF_ILLUM, false, false, 
		new CPointGlowAnimator( 
			pScene,
			pTime, 
			pOrigin,
			shareTextures.Get( STextureKey( pFlareTexture, STextureKey::TK_TRANSPARENT ) ),
			fFlareRadius,
			fOnTime,
			fOffTime ),
		new CMNode( pIdentityTransform, pOrigin ), bound, 
		SRoomInfo( 0, nFloor, 0 ), 0 );
}

CObjectBase* CGameView::AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, const CVec2 &sSize, const NDb::STexture *pTexture, int nFloor )
{
	if ( ( fabs( sSize ) < FP_EPSILON ) || !pTexture )
		return 0;

	return AddDirFlare( pPos, pDir, new CCVec2( sSize ), sSize, pTexture, nFloor );
}

CObjectBase* CGameView::AddDirFlare( CFuncBase<CVec3> *pPos, CFuncBase<CVec3> *pDir, CFuncBase<CVec2> *pSize, const CVec2 &vMaxSize, const NDb::STexture *pTexture, int nFloor )
{
	////
	SBound sBound;
	sBound.SphereInit( CVec3( 0, 0, 0 ), fabs( vMaxSize ) );
	////
	return CreateParticles( true,
		PF_DYNAMIC | PF_SELF_ILLUM, false, false, 
		new CSpriteAnimator( 
			pScene,
			pPos,
			pDir,
			shareTextures.Get( STextureKey( pTexture, STextureKey::TK_TRANSPARENT ) ),
			pSize ),
		new CMNode( pIdentityTransform, pPos ), sBound, 
		SRoomInfo( 0, nFloor, 0 ), 0 );
}

static void GetParts( std::vector<CObjectBase*> *pRes, const std::vector<CObjectBase*> &target )
{
	for ( int k = 0; k < target.size(); ++k )
	{
		CDynamicCast<CRenderNode> pNode( target[k] );
		if ( !IsValid( pNode ) )
		{
			pRes->push_back( target[k] );
			continue;
		}

		for ( int nTemp = 0; nTemp < pNode->parts.size(); nTemp++ )
			pRes->push_back( pNode->parts[nTemp] );
	}
}

CObjectBase* CGameView::AddPostFilter( const std::vector<CObjectBase*> &target, IPostProcess *pEffect )
{
	CPtr<IPostProcess> pHold(pEffect);
	std::vector<CObjectBase*> parts;
	GetParts( &parts, target );
	CRenderNode *pRes = new CRenderNode;
	for ( int k = 0; k < parts.size(); ++k )
	{
		CObjectBase *p = pScene->CreatePostProcessor( parts[k], pEffect );
		if ( p )
			pRes->AddPart( p );
	}
	return pRes;
}

CObjectBase* CGameView::AddSpotLight( const CVec3 &ptColor, const CVec3 &ptOrigin, const CVec3 &ptDir, float fFOV, 
	float fRadius, const NDb::STexture *pMask, bool bLightmapOnly )
{
	if ( pMask )
		return pScene->AddSpotLight( new CCVec3(ptColor), ptOrigin, ptDir, fFOV, fRadius, shareTextures.Get( (STextureKey)pMask ), bLightmapOnly );
	return pScene->AddSpotLight( new CCVec3(ptColor), ptOrigin, ptDir, fFOV, fRadius, 0, bLightmapOnly );
}

static CVec3 GetLightDir( float fPitch, float fYaw )
{
	CVec3 vLightDir;
	float fHor = sin( ToRadian( fPitch ) );
	vLightDir.z = -cos( ToRadian( fPitch ) );
	vLightDir.x = fHor * cos( ToRadian( fYaw ) );
	vLightDir.y = fHor * sin( ToRadian( fYaw ) );
	return vLightDir;
}
void CGameView::SetAmbient( const NDb::SAmbientLight *pLight, bool bSelectGF2, CFuncBase<STime> *pTime )
{
	//CPtr<NDb::SAmbientLight> pHold(pLight);
	pPrevLight = pLight;
	LoadFogColors( pPrevLight );

	pPrevLightTime = pTime;
	if ( pLight == 0 )
	{
		pScene->SetDirectionalLight( 
			new CCVec3( CVec3(0.25,0.25,0.25 ) ), new CCVec3( CVec3(1,1,1) ), 
			CVec3(0.707f, 0, -0.707f), CVec3(0.707f, 0, -0.707f), 20, 10, 1,
			new CCVec3( CVec3(0.25,0.25,0.25) ), new CCVec3( CVec3(0,0,0) ), new CCVec3( CVec3(0,0,0) ), CVec3(1.0f, 1.0f, 1.0f),
			0, 0, CVec3( 1, 1, 1) );
		//SetAmbient( CVec3(0.5f, 0.5f, 0.5f), CVec3(0.5f, 0.5f, 0.5f) );
		pMaterials->SetSky( 0 );
		return;
	}
	vDefaultClearColor =  pLight->vBackColor;

	// set haze
	if ( pLight->pHaze )
		pHaze = shareTextures.Get( (STextureKey)pLight->pHaze );
	else
		pHaze = 0;

	// set depth of field
	if ( pLight->pDepthOfField )
		SetDepthOfField( new SDepthOfField( pLight->pDepthOfField->fFocalDist, pLight->pDepthOfField->fFocusRange ) );


	// set shadows
	if ( !IsUsingShadows( GetRenderPath() ) && pLight->pGForce2Light && !bSelectGF2 )//NGfx::GetHardwareLevel() < NGfx::HL_GFORCE3 
	{
		SetAmbient( pLight->pGForce2Light, true, pTime );
		pPrevLight = pLight;
		LoadFogColors( pPrevLight );
		return;
	}
	//
	CVec3 vLightColor( pLight->vLightColor );//1, 0, 0 );//
	CVec3 vAmbientColor( pLight->vAmbientColor ); //0, 0, 0 );//
	CVec3 vShadeColor( pLight->vShadeColor );//0, 1, 0 );//
	CVec3 vIncidentShadeColor( pLight->vIncidentShadowColor );//1, 0, 0 );//
	vLightColor -= vAmbientColor;
	vShadeColor -= vAmbientColor;
	vIncidentShadeColor -= vAmbientColor;
	// directional light
	CVec3 vLightDir = GetLightDir( pLight->fPitch, pLight->fYaw );
	CVec3 vShadowsLightDir;
	if ( pLight->fShadowPitch == 100 )
		vShadowsLightDir = vLightDir;
	else
		vShadowsLightDir = GetLightDir( pLight->fShadowPitch, pLight->fShadowYaw );

	CPtrFuncBase<NGfx::CTexture> *pCloudsTexture = 0;
	if ( pLight->pCloudTex )
		pCloudsTexture = shareTextures.Get( GetKey( pLight->pCloudTex ) );
	CFuncBase<SHMatrix> *pCloudsPos = 0;
	if ( pTime && pCloudsTexture )
		pCloudsPos = new CCloudMover( pTime, pLight->vCloudSize, pLight->fCloudDir, pLight->fCloudSpeed );

	float fMaxShadowHeight = pLight->fMaxShadowHeight == 0 ? 20 : pLight->fMaxShadowHeight;
	pScene->SetDirectionalLight( 
		new CCVec3( vLightColor ), new CCVec3( pLight->vGlossColor ), 
		vLightDir, vShadowsLightDir, fMaxShadowHeight, pLight->fShadowsMaxDetailLength, pLight->fBlurStrength,
		new CCVec3( vAmbientColor ), new CCVec3( vShadeColor ), new CCVec3( vIncidentShadeColor ), pLight->vParticlesColor,
		pCloudsTexture, pCloudsPos, pLight->vDymanicLightsModifications );
	// ambient light
	if ( pLight->pSky )
		pMaterials->SetSky( shareCubeTextures.Get( pLight->pSky ) );
	else
		pMaterials->SetSky( 0 );
	////
	if ( pLight->pSunFlares )
	{
		CPtr<CSunFlares> pFlares = new CSunFlares;
		pFlares->flares.resize( pLight->pSunFlares->flares.size() );
		for ( int nTemp = 0; nTemp < pLight->pSunFlares->flares.size(); ++nTemp )
		{
			const NDb::SSunFlare &sDBFlare = pLight->pSunFlares->flares[nTemp];
			CSunFlares::SFlare &sFlare = pFlares->flares[nTemp];
			////
			sFlare.bFade = sDBFlare.bFade;
			sFlare.fScale = sDBFlare.fScale;
			sFlare.fDistance = sDBFlare.fDistance;
			sFlare.pFlare = shareTextures.Get( STextureKey( sDBFlare.pTexture, 0 ) );
		}
		////
		if ( pLight->pSunFlares->pOverBright )
			pFlares->pOverbright = shareTextures.Get( STextureKey( pLight->pSunFlares->pOverBright, 0 ) );
		////
		CVec3 sunFlareDir = GetLightDir( pLight->fSunFlarePitch, pLight->fSunFlareYaw );
		Normalize( &sunFlareDir );
		pScene->SetSunFlares( sunFlareDir, pFlares, pTime );
	}
	else
		pScene->SetSunFlares( CVec3( 0, 0, 0 ), 0, 0 );
#ifdef DEBUG_LIGHTING
	static std::vector<CObj<CObjectBase> > lights;
	float fBright = 1;//.5f;
	float fRadius = 50;
	lights.push_back( AddPointLight( CVec3(fBright,fBright,0), CVec3(20,20,30), fRadius ) );
	lights.push_back( AddPointLight( CVec3(0,fBright,0), CVec3(50,20,30), fRadius ) );
	lights.push_back( AddPointLight( CVec3(0,fBright,fBright), CVec3(20,50,30), fRadius ) );
	lights.push_back( AddPointLight( CVec3(fBright,0,fBright), CVec3(50,50,30), fRadius ) );
	//lights.push_back( AddPointLight( CVec3(50,50,20), 100, CVec3(fBright,fBright,0) ) );
#endif

	pRain = 0;
	if ( pLight->pRain && pTime )
		pRain = CreateRain( pLight->pRain, pTime, 0, SRoomInfo() );

	pSkyDome = 0;
	if ( pLight->pSkyDome )
		pSkyDome = CreateSkyDome( this, pLight->pSkyDome );
}

void CGameView::SetRenderMode( ESceneRenderMode mode )
{
	renderMode = mode;
	if ( pPrevLight )
		SetAmbient( pPrevLight, pPrevLightTime );
}

ESceneRenderMode CGameView::GetRenderMode() const
{
	return renderMode;
}

bool CGameView::TraceScene( const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps, SFullRoomInfo *pRoomInfo, CObjectBase **ppPart, bool bOpaqueOnly )
{
	int nReq = bOpaqueOnly ? N_MASK_CAST_SHADOW : 0;
	SGroupSelect mask( GetFloorMask( N_MAX_FLOOR ), nReq | GetParticlesRequireFlag( true ) );
	SFullGroupInfo gg;
	bool bRes = pScene->TraceScene( mask, r, pfT, pNormal, ps, &gg, ppPart );
	if ( pRoomInfo )
		*pRoomInfo = GetRoomInfo( gg );
	return bRes;
}

int CGameView::GetCutFloor()
{
	return nCutFloor;
}

void CGameView::SetCutFloor( int _nFloor )
{
	nCutFloor = Clamp( _nFloor, N_MIN_FLOOR, N_MAX_FLOOR );
}

// standalone functions

template<class T>
static void PerPartFunc( CObjectBase *_p, T f )
{
	if ( !IsValid(_p) )
		return;
	if ( CDynamicCast<CRenderNode> p = _p )
	{
		for ( int k = 0; k < p->parts.size(); ++k )
			f( p->parts[k] );
	}
	else
		f( _p );
}

struct SSetFade
{ 
	float f;
	SSetFade( float _f ) : f(_f) {}
	void operator()( CObjectBase *p ) const { SetPartFade( p, f ); }
};
void SetFade( CObjectBase *_p, float _f )
{
	float f = Clamp( _f, 0.0f, 1.0f );
	PerPartFunc( _p, SSetFade( f ) );
}

struct SSetPriority
{
	int n;
	SSetPriority( int _n ) : n(_n) {}
	void operator()( CObjectBase *p ) const { SetPartPriority( p, n ); }
};
void SetPriority( CObjectBase *_p, int _nPriority )
{
	PerPartFunc( _p, SSetPriority( _nPriority ) );
}

struct SStopParticlesGeneration
{
	STime tTime;
	SStopParticlesGeneration( STime _t ) : tTime(_t) {}
	void operator()( CObjectBase *_p ) const 
	{ 
		CPtrFuncBase<CParticleEffect> *p = GetParticleAnimator( _p );
		if ( !p )
			return;
		if ( CDynamicCast<CParticleAnimator> pAnimator = p )
			pAnimator->StopParticlesGeneration( tTime );
	}
};
void StopParticlesGeneration( CObjectBase *_p, STime tStop )
{
	PerPartFunc( _p, SStopParticlesGeneration( tStop ) );
}

struct SStopDynamicLighting
{
	void operator()( CObjectBase *_p ) const 
	{ 
		if ( CDynamicCast<CDynamicPointLight> pLight = _p )
		{
			pLight->SwitchLight( false );
		}
	}
};	
void StopDynamicLighting( CObjectBase *_p )
{
	PerPartFunc( _p, SStopDynamicLighting() );
}

template<class T>
static void SetShadowsMode( IGameView *p, T take )
{
	ESceneRenderMode r = p->GetRenderMode();
	for(;;)
	{
		r = (ESceneRenderMode) (r + 1);
		if ( r == SRM_LAST ) 
			r = SRM_SHOWOVERDRAW;
		if ( take( r ) )
			break;
	}
	p->SetRenderMode( r );
}
inline bool Fake( ESceneRenderMode m )
{
	return true;
}
void SetNextLightmapViewMode( IGameView *p )
{
	SetShadowsMode( p, Fake );
}
void SetNextTranspRenderMode( IGameView *p )
{
	ETransparentMode tr = p->GetTransparentMode();
	tr = (ETransparentMode)( tr + 1 );
	if ( tr == TRM_LAST )
		tr = TRM_NONE;
	p->SetTransparentMode( tr );
}
void SetNextHSRMode( IGameView *p )
{
	EHSRMode r = (EHSRMode) (p->GetHSRMode() + 1);
	if ( r == HSR_LAST ) 
		r = HSR_NONE;
	p->SetHSRMode( r );
}

void ReloadTexture( const NDb::STexture *p )
{
	const CTextureBasicShare::CDataHash &data = shareTextures.GetAll();
	if ( p )
	{
		for ( CTextureBasicShare::CDataHash::const_iterator i = data.begin(); i != data.end(); ++i )
		{
			if ( IsValid( i->second ) && i->first == p )
				i->second->ReleaseTexture();
		}
	}
	else
	{
		for ( CTextureBasicShare::CDataHash::const_iterator i = data.begin(); i != data.end(); ++i )
		{
			if ( IsValid( i->second ) )
				i->second->ReleaseTexture();
		}
	}
}

CLightmapsHolder *CalcLightmaps( IGameView *_p, CObjectBase *pUser, int nUserID, const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder )
{
	if ( IsValid(_p) )
	{
		CDynamicCast<CGameView> p = _p;
		return CalcLightmaps( p->GetGScene(), pUser, nUserID, highResLM, quality, pTmpHolder );
	}
	return 0;
}

void ApplyLightmaps( IGameView *_p, CObjectBase *pUser, CLightmapsHolder *pLightmaps, CLightmapsLoader *pLD )
{
	if ( IsValid(_p) )
	{
		CDynamicCast<CGameView> p = _p;
		ApplyLightmaps( p->GetGScene(), pUser, pLightmaps, pLD );
	}
}

class CTransparencyToOpacityFunc
{
public:
	float operator()( float fValue )
	{
		return (1.f - Clamp(fValue, 0.f, 1.f));
	}
};

void CreateAnimatedTransparencyChannels( std::vector<CPtr<CFuncBase<float> > > *pResult,
		const IGameView::SMeshInfo &meshInfo, const NDb::SModel *pModel,
		NAnimation::ISkeletonAnimator *pAnimator )
{
	pResult->assign(meshInfo.parts.size(), 0);
	std::string szChannelName;
	for ( int i = 0; i < meshInfo.parts.size(); ++i )
	{
		const NGScene::IGameView::SPartInfo &partInfo = meshInfo.parts[i];
		// уступка B2(M1), у которых meshNames всегда пустой
		if ( partInfo.nOrigMeshIndex < pModel->pGeometry->meshNames.size() )
		{
			szChannelName = pModel->pGeometry->meshNames[ partInfo.nOrigMeshIndex ];
			szChannelName += ".Transparency";
			(*pResult)[i] = new NAnimation::CAnimatedChannel<CTransparencyToOpacityFunc>(pAnimator, szChannelName);
		}
	}
}

CDGPtr <CGrannyMeshLoader> GetMeshLoader( const SPartAndSkeletonKey & key )
{
	return shareGrannyMeshes.Get( key );
}

void CollectAllParts( std::vector<CObjectBase*> *pRes, IGameView *_pView )
{
	CDynamicCast<CGameView> pView( _pView );
	IGScene *pScene = pView->GetGScene();
	pScene->CollectAllParts( pRes );
}

// main create routine
IGameView* CreateNewView()
{
	return new CGameView;
}

// Commands/Vars

static void VarSetHSR( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	defaultHSRMode = HSR_NONE;
	if ( sValue.GetFloat() == 1 )
		defaultHSRMode = HSR_FAST;
	if ( sValue.GetFloat() > 1 )
		defaultHSRMode = HSR_DYNAMIC;
}

// IT WILL BE DELETED SOON - for fast change of parameters
float s_fWaterAmplitude = 0.03f;
float s_fWaterWaveLength = 5.13f;
float s_fWaterWaveFrequence = 4000.0f;

START_REGISTER(GView)
	REGISTER_VAR( "gfx_hsr", VarSetHSR, 0, STORAGE_USER )
	REGISTER_VAR( "gfx_hqshot_mag", 0, 4, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_particles", NGlobal::VarBoolHandler, &bShowParticles, true, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_water_reflection", NGlobal::VarBoolHandler, &bWaterReflection, false, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_water_amplitude", NGlobal::VarFloatHandler, &s_fWaterAmplitude, 0.01f, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_water_length", NGlobal::VarFloatHandler, &s_fWaterWaveLength, 5.13f, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_water_frequence", NGlobal::VarFloatHandler, &s_fWaterWaveFrequence, 4000.0f, STORAGE_USER )

FINISH_REGISTER

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x01741140, CGameView )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x34256C40, CParticleTexture)
REGISTER_SAVELOAD_TEMPL_CLASS( _3DMOTOR, 0x028b2122, CResourcePrecache<NAnimation::CGrannyAnimationLoader>, CResourcePrecache )
BASIC_REGISTER_CLASS( _3DMOTOR, IGameView )
using namespace NAnimation;
REGISTER_SAVELOAD_TEMPL_CLASS( _3DMOTOR, 0x50161480, CAnimatedChannel<CTransparencyToOpacityFunc>, CAnimatedChannel)

