#pragma once

#include "3Dmotor/aiMap.h"
#include "3Dmotor/aiRender.h"

#include "Input/GameMessage.h"
#include "B2_M1_Terrain/PatchHolder.h"
#include "AIDebugInfo.h"
#include "WeatherVisual.h"

enum ESoundSceneMode : int;

#include <cstdint>

#define SCREEN_VIRTUAL_WIDTH 1024.0f
#define SCREEN_VIRTUAL_HEIGHT 768.0f
#define INTERFACE_3D_ELEMENT_WIDTH 120.0f
#define INTERFACE_3D_ELEMENT_HEIGHT 100.0f

namespace NGScene
{
	class IGameView;
	class CPolyline;
	class CObjectInfo;
	class CRTPtr;
};
namespace NDb
{
	struct SModel;
	struct SAmbientLight;
};
namespace NAIVisInfo
{
	class CDebugObject;
};
namespace NDebugInfo
{
	struct SDebugInfoObject;
	struct SDebugInfoUpdate;
	struct SDebugInfoMarker;
	struct SDebugInfoCircle;
	struct SDebugInfoSegment;
	struct SDebugInfoLine;
	struct SDebugInfoRect;
	struct SDebugInfoDeleteObject;
	struct SDebugInfoDeleteLine;
};
class CTerrainManager;
class CTracksManager;
class CVisObjIconsManager;
class CWindController;
struct SVisObjDescBase;
struct ICamera;
struct SVisObjSelection;
struct SVisObjSelectionHandler;
struct SModelVisObjDesc;
struct IFullScreenFader;

class CSceneSyncSrc: public CSyncSrc<IVisObj>
{
	OBJECT_BASIC_METHODS( CSceneSyncSrc );
};

class CAIMapVisitor : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CAIMapVisitor)
	ZDATA
		COrdinarySyncDst<IVisObj,NAI::IAIMap> dst;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dst); return 0; }
	CAIMapVisitor() {}
	CAIMapVisitor( CSyncSrc<IVisObj> *pSrc, NAI::IAIMap *pAIMap )
		: dst( pSrc, pAIMap ) {}
	void Sync( NAI::IAIMap::ESyncType st = NAI::IAIMap::ST_NORMAL )
	{
		dst.Sync();
		dst.GetVisitor()->Sync( st );
	}
};

class CSceneIconInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CSceneIconInfo )
	//
	CPtr<ICamera> pCamera;
	bool bUpdate;
	NMeshData::SMeshData data;
	CVec3 vCenter;
	CVec2 vHalfSize;
	CVec3 vBBHalfSize;
	CObj<CCSBound> pBound;
	CDBPtr<NDb::SMaterial> pMaterial;
	int nID;
	CSyncSrcBind<CSceneIconInfo> srcBind;
	CDBPtr<NDb::SAIGeometry> pAIGeometry;
protected:
	void Recalc();
	bool NeedUpdate();
	void OrientToViewer();
	//
	CSceneIconInfo() {}
public:
	CSceneIconInfo( ICamera *_pCamera ) : pCamera( _pCamera ), bUpdate( true ) {}
	//
	void CreateIcon( const int _nID, const CVec3 &_vCenter, const CVec2 &_vSize, const CVec2 &_vTexMin, const CVec2 &_vTexMax,
		const NDb::SMaterial *_pMaterial, const NDb::SAIGeometry *_pAIGeometry, CSyncSrc<CSceneIconInfo> *pSyncSrc );
	//
	void MoveIcon( const CVec3 &_vCenter );
	//
	void ForceUpdate() { srcBind.Update(); bUpdate = true; }
	//
	int GetID() const { return nID; }
	//
	CCSBound *GetBound() { return pBound; }
	//
	const NDb::SMaterial *GetMaterial() const { return pMaterial; }
	//
	void Visit( IAIVisitor *pVisitor );
	//
	void AttachSyncSrc( CSyncSrc<CSceneIconInfo> *pSyncSrc )
	{
		srcBind.Link( pSyncSrc, this );
		srcBind.Update();
	}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pCamera );
		saver.Add( 2, &bUpdate );
		saver.Add( 3, &data );
		saver.Add( 4, &vCenter );
		saver.Add( 5, &vHalfSize );
		saver.Add( 6, &vBBHalfSize );
		saver.Add( 7, &pBound );
		saver.Add( 8, &pMaterial );
		saver.Add( 9, &nID );
		saver.Add( 10, &pAIGeometry );
		return 0;
	}
};

typedef NMeshData::SPatchHolder<CSceneIconInfo> CSceneIconHolder;

class CVisObjReflectionInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CVisObjReflectionInfo )
	//
	CDGPtr<CPtrFuncBase<NGScene::CObjectInfo> > pGeometry;
	SHMatrix matr;
	NMeshData::SMeshData data;
	CObj<CTerrainManager> pTerraManager;
	bool bWasUpdate;
protected:
	void Recalc();
	bool NeedUpdate() { return !bWasUpdate && pGeometry.Refresh(); }
public:
	CPtr<NGScene::IMaterial> pMaterial;
	//
	CVisObjReflectionInfo() : bWasUpdate( false ) {}
	CVisObjReflectionInfo( CTerrainManager *_pTerraManager ) : pTerraManager( _pTerraManager ), bWasUpdate( false ) {}
	void SetGeometry( CPtrFuncBase<NGScene::CObjectInfo> *_pGeometry ) { pGeometry = _pGeometry; }
	void SetTransform( const SHMatrix &_matr ) { matr = _matr; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pGeometry );
		saver.Add( 2, &matr );
		saver.Add( 3, &data );
		saver.Add( 4, &pMaterial );
		saver.Add( 5, &bWasUpdate );
		return 0;
	}
};

typedef NMeshData::SPatchHolder<CVisObjReflectionInfo> CVisObjReflectionInfoHolder;

class CIconSceneSyncSrc : public CSyncSrc<CSceneIconInfo>
{
	OBJECT_BASIC_METHODS( CIconSceneSyncSrc );
};

class CIconAIMapVisitor : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CIconAIMapVisitor)
	ZDATA
		COrdinarySyncDst<CSceneIconInfo, NAI::IAIMap> dst;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dst); return 0; }
	CIconAIMapVisitor() {}
	CIconAIMapVisitor( CSyncSrc<CSceneIconInfo> *pSrc, NAI::IAIMap *pAIMap )
		: dst( pSrc, pAIMap ) {}
	void Sync( NAI::IAIMap::ESyncType st = NAI::IAIMap::ST_NORMAL )
	{
		dst.Sync();
		dst.GetVisitor()->Sync( st );
	}
};

struct SSceneData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SSceneData )
public:		
	typedef std::unordered_map<int, CObj<SVisObjDescBase> > CVisObjectsMap;
	typedef std::list< CObj<IWindow> > CScreens; // obsolete
	typedef std::list< CObj<CObjectBase> > CMarkersList;
	typedef std::unordered_map< int, CMarkersList > CMarkersHash;
	typedef std::unordered_map<int, CObj<NGScene::CPolyline> > CPolylinesMap;
	typedef std::unordered_map<int, CSceneIconHolder> CSceneIconsMap;
	typedef std::unordered_map< int, CObj<SVisObjSelectionHandler> > CSelectionMap;
	
	struct SScreenData
	{
		ZDATA
		CObj<IWindow> pScreen;
		CVisObjectsMap interfaceVisObjects; // 3D interface objects		
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pScreen); f.Add(3,&interfaceVisObjects); return 0; }

		CObj<NGScene::IGameView> p3DView; // 3D part of interface
		
		SScreenData() {}
		SScreenData( IWindow *_pScreen );

		void SetSceneConsts( const NDb::SSceneConsts *pSceneConsts );

		bool operator==( const SScreenData &data ) const
		{
			return pScreen == data.pScreen;
		}
		bool operator==( const IWindow *_pScreen ) const
		{
			return pScreen == _pScreen;
		}
	};
	typedef std::list<SScreenData> CScreensData;
	
	enum EAIGeomMode
	{
		EAI_GEOM_NONE,
		EAI_GEOM_OVER,
		EAI_GEOM_SOLID,
		EAI_GEOM_LAST,
	};

	ZDATA
		CDGPtr<CCSTime> pAbsTimer;
		CDGPtr<CCSTime> pGameTimer;
		CVisObjectsMap visObjects;
		std::unordered_map<int, bool> showModes;
		std::unordered_map<int, bool> fadeModes;
		CScreens screens; // obsolete
		int nLastFreeID;
		bool bEnableStatistics;
		CObj<CTerrainManager> pTerraManager;
		CObj<CTracksManager> pTracksManager;
		std::vector<CMarkersHash> markers;
		CSceneIconsMap iconsMap;
		std::vector<CVisObjReflectionInfoHolder> visObjReflections;
		CObj<CWindController> pWindController;
		CObj<CVisObjIconsManager> pVisObjIconsManager;
		ZSKIP //CVisObjectsMap interfaceVisObjects; 
		std::unordered_map<int, int> attachIDToMapObjID;
		CPtr<CWeatherVisual> pWeather;
		ZSKIP
		CDBPtr<NDb::SAmbientLight> pSceneAmbientLight;
		CDBPtr<NDb::SAmbientLight> pInterfaceAmbientLight;
		bool bIsInternal;
		CScreensData screensData;
		ZSKIP //CObj<CObjectBase> pPostEffect;
		ZSKIP
		CObj<IFullScreenFader> pScreenFader;
		ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAbsTimer); f.Add(3,&pGameTimer); f.Add(4,&visObjects); f.Add(5,&showModes); f.Add(6,&fadeModes); f.Add(7,&screens); f.Add(8,&nLastFreeID); f.Add(9,&bEnableStatistics); f.Add(10,&pTerraManager); f.Add(11,&pTracksManager); f.Add(12,&markers); f.Add(13,&iconsMap); f.Add(14,&visObjReflections); f.Add(15,&pWindController); f.Add(16,&pVisObjIconsManager); f.Add(18,&attachIDToMapObjID); f.Add(19,&pWeather); f.Add(21,&pSceneAmbientLight); f.Add(22,&pInterfaceAmbientLight); f.Add(23,&bIsInternal); f.Add(24,&screensData); f.Add(27,&pScreenFader); OnSerialize( f ); return 0; }

	CObj<CObjectBase> pWeatherSceneObject;
	std::list< CObj<CObjectBase> > postEffects;

	CSelectionMap selectionHandlers; // don't serialize
	int nFreeSelectionID; // don't serialize

	void OnSerialize( IBinSaver &f )
	{
		if ( f.IsReading() ) 
		{
			timeStatLastShow = 0;
			nStatNumFrames = 0;
			
			//{ CRAP - for compability with old saves
			if ( screensData.empty() )
			{
				for ( CScreens::iterator it = screens.begin(); it != screens.end(); ++it )
				{
					IWindow *pScreen = *it;
					screensData.push_back( SScreenData( pScreen ) );
				}
			}
			//}
		}
	}

	std::vector< CObj<CObjectBase> > vAIMapMeshes;
	EAIGeomMode eAIGeomMode;

private:
	CObj<NGScene::IGameView> pGScene;	
public:
	CObj<NGScene::IGameView> pInterfaceView;

	CObj<NGScene::I2DGameView> p2DView;	

	CObj< CSyncSrc<IVisObj> > pSyncSrc;
	CObj<CAIMapVisitor> pAIMapVisitor;
	CObj<NAI::IAIMap> pAIMap;

	CObj< CSyncSrc<CSceneIconInfo> > pIconSyncSrc;
	CObj<CIconAIMapVisitor> pIconAIMapVisitor;
	CObj<NAI::IAIMap> pIconAIMap;

	CPolylinesMap polylines;
	std::vector< CObj<NGScene::CPolyline> > terrainGrid;
	//
	//
	CDBPtr<NDb::SSceneConsts> pSceneConsts;
	// statistics measurement
	NTimer::STime timeStatLastShow;
	int nStatNumFrames;
	int nStatNumVerts;
	int nStatNumTris;
	int nStatNumDIPs;
	float fAveOverdraw;
	int nFPSDropCounter;
	
	SSceneData();
	
	void Init();
	NGScene::IGameView* GetGScene() { return bIsInternal ? pInterfaceView : pGScene; }
	void SetGScene( NGScene::IGameView *_pGScene ) { pGScene = _pGScene; };
	bool SetGSceneInternal( bool _bIsInternal )
	{
		bool bTmp = bIsInternal;
		bIsInternal = _bIsInternal;
		return bTmp;
	};
	
	SVisObjSelectionHandler* GetNewSelection();
};

class CScene : public IScene
{
	OBJECT_NOCOPY_METHODS( CScene )

	NInput::CGMORegContainer observers;

	typedef std::unordered_map< int, CPtr<NAIVisInfo::CDebugObject> > CDebugInfoObjects;
	CDebugInfoObjects debugInfoObjects;

	ZDATA
		std::vector< CObj<SSceneData> > data;
		EScene eScene;
		bool bEditorMode;
		NTimer::STime timeBadWeatherLeft;
		ZONSERIALIZE

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&data); f.Add(3,&eScene); f.Add(4,&bEditorMode); f.Add(5,&timeBadWeatherLeft); OnSerialize( f ); return 0; }

private:
	NAI::CFastRenderer fastRender;

	// Does need to get sizes from target texture for creating project matrix
	bool bGetSizesFromTarget;

protected:
	CVec4 vBackgroundColor;

	~CScene();

	void OnSerialize( IBinSaver &saver );
	//
	void CreateGScene();
	int GetNextFreeID() { data[eScene]->nLastFreeID += 1; return data[eScene]->nLastFreeID | 0x80000000; }
	int GetID( int nID ) { return nID == OBJECT_ID_GENERATE ? GetNextFreeID() : nID; }
	//
	bool CheckObjExist( int nID ) const;
	int AddStaticObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, 
											 const CVec3 &vScale, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bHasReflection );
	int AddAnimatedObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale, 
												 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pLowLevelModel, const bool bHasReflection );
	int AddIndexedPolylineInternal( const int nID, const std::vector<CVec3> &points, const std::vector<uint16_t> &indices, const CVec4 &vColor, bool bDepthCheck );

	//	AddObject with client-calculated matrix
	int AddStaticObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace,
											 NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bHasReflection );
	int AddAnimatedObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace,
		ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bHasReflection );
	//
	void AddInterfaceObject( IWindow *pScreen, int nID, const NDb::SModel *pModel, const CVec2 &vScreenPos, const CVec2 &vElementSize );
	void RemoveInterfaceObject( IWindow *pScreen, const int nID );
	//
	void CalcAverageOverdrawMsg( const SGameMessage &msg );
	void ToggleShowMsg( const SGameMessage &msg, ESceneShow eShow );
	void ShowTerrainGrid( ESceneShow eShow );
	void UpdateGrid( int nMinX, int nMinY, int nMaxX, int nMaxY );	// interface for next function
	void UpdateGrid( int nMinX, int nMinY, int nMaxX, int nMaxY, bool bAIGrid );
	//
	void FadeObject( int nID, bool bFade );
	void FadeObject( int nID, float fFade );
	//
	virtual void GetCoveredObjects( std::list<int> *pCoveredObjects, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle );
	virtual void GetObstacleObjects( std::list<int> *pObstacleObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle );
	//
	void AddReflectionFromObject( const NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pModel, const SHMatrix &matr );
	//
	CFuncBase<SFBTransform> *CScene::GetParentTransform( const int nTargetID, const std::string &szBoneName );
	//	
	bool PrepareToAttach( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const ESceneAttachMode eMode, const NTimer::STime timeStart,
												CFuncBase<SFBTransform> **pTransrform, int *pnBoneIndex, const bool bConstantOffset );
	bool PrepareToAttach( const int nTargetID, ESceneSubObjType eType, const ESceneAttachMode eMode, const NTimer::STime timeStart,
												CFuncBase<SFBTransform> **pTransrform, const CVec3 &vOffset );

	bool DeleteAttachesByType( const int nTargetID, ESceneSubObjType eType, const ESceneAttachMode eMode, const NTimer::STime timeStart, const int nBoneIndex );

	void CycleAIGeometryModes();
	void UpdateAIGeometry();

	void InitDebugMaterials( const NDb::SSceneConsts *pSceneConsts );
	void AddDebugInfoObject( const int nID, NAIVisInfo::CDebugObject *pObject );
	void CreateMarker( const NDebugInfo::SDebugInfoMarker *pMarker );
	void CreateCircle( const NDebugInfo::SDebugInfoCircle *pCircle );
	void CreateSegment( const NDebugInfo::SDebugInfoSegment *pSegment );
	void CreateLine( const NDebugInfo::SDebugInfoLine *pLine );
	void CreateRect( const NDebugInfo::SDebugInfoRect *pRect );

	void ProcessDebugInfoUpdates();

	void MakeSelection( SVisObjSelection &selection, SModelVisObjDesc *pVOD, const CVec3 &vPos, 
		float fSelScale, NDb::ESelectionType eSelType, float fDeltaTime, bool bOn );
	void UpdateSelectionHandlers();

	CFuncBase<SBound> *GetObjectBounder( const int nID );

	IAttachedObject *GetAttachedObject( const int nTargetID, const std::string &szBoneName );

	CObjectBase* CreateCircle( CFuncBase<SFBTransform> *pTransform, float fRadius, const CVec3 &vColor, float fWidth );
public:
	CScene();

	void Init();
	virtual void SwitchScene( const EScene eScene );
	virtual const EScene GetCurrentScene() const;
	virtual void SwitchSceneAfterLoad( const EScene eScene );

	bool SetupMode( ESceneMode eMode, bool bEditorMode );
	bool IsEditorMode() { return bEditorMode; }
	bool ToggleShow( ESceneShow eShow );
	bool IsShowOn( ESceneShow eShow );
	bool ToggleAIGeometryMode();
	bool ToggleShowPassability();

	void SetSceneConsts( const NDb::SSceneConsts *_pSceneConsts );
	const NDb::SSceneConsts *GetSceneConsts();
	//	
	void ClearScene( const EScene eScene2Clear );
	// 
	struct ITerraManager* GetTerraManager();
	bool DoesTerraManagerExist() const { return data[eScene]->pTerraManager != 0; }
	// add object to scene. returns new ID
	int AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale, 
								 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pLowLevelModel, const bool bHasReflection );
	//	AddObject with client-calculated matrix
	int AddObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace,
								 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bNeedReflection );

	bool ChangeModel( const int nObjectID, const NDb::SModel *pModel );
	virtual NAnimation::ISkeletonAnimator* GetAnimator( const int nID, bool bRefreshAnimator = true );
	virtual NAnimation::ISkeletonAnimator *GetInterfaceObjAnimator( IWindow *pScreen, const int nID );
	virtual NAnimation::ISkeletonAnimator* GetAnimator( const int nTargetID, const std::string &szBoneName );
	virtual void AttachSubModel( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const NDb::SModel *pSubModel, ESceneAttachMode eMode, const int nNumber, bool bForceAnimated, const bool bConstantOffset );
	virtual void AttachSubModel( const int nTargetID, ESceneSubObjType eType, const NDb::SModel *pSubModel, ESceneAttachMode eMode, const int nNumber, bool bForceAnimated, const CVec3 &vOffset );
	virtual struct IAttachedObject* GetAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber );

	virtual void RemoveAllAttached( const int nTargetID, ESceneSubObjType eType );
	virtual void RemoveAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber );

	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, bool bVertical = false );
	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, CFuncBase<SFBTransform> *pTransform, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, const int nBoneIndex = -1, bool bVertical = false );
	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const SHMatrix &mOffset, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode );
	// Attach light FX: to animated (with bone name) and to static objects (without bone name)
	virtual void AttachLightEffect( const int nTargetID, const NDb::SAttachedLightEffect *pLight, NTimer::STime timeStart, ESceneAttachMode eMode, const bool bInEditor = false, int nHoldID = -1 );

	// add effect to scene. returns new ID
	int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot );
	int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace );
	void AddEffect( const int nID, const std::string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace );

	void StopEffectGeneration( const int nID, NTimer::STime time );

	int AddPointLight( const int nID, const CVec3 &ptColor, const CVec3 &ptOrigin, float fR );
	bool GetVisObjPlacement( const int nID, SFBTransform *pTransform ) const;
	// remove object from scene by ID, returned by AddObject()
	void RemoveObjectPickability( const int nID );
	void RemoveObject( const int nID );
	// move object
	bool MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale = CVec3(1, 1, 1) );
	bool MoveObject( const int nID, const SHMatrix &mPlace );
	// selection operations
	void SelectObject( const int nID, const CVec3 &vPos, const float fSelScale, const NDb::ESelectionType eSelType );
	void UnselectObject( const int nID );
	int AddSelection( int nID, const CVec3 &vPos, float fSelScale, NDb::ESelectionType eSelType, 
		float fFadeInTime, float fFadeOutTime );
	void RemoveSelection( int nID );
	void ClearSelection();
	// tracks operations
	void AddTrack( const int nID, const float fFadingSpeed,
								 const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
								 const CVec2 &vNorm, const float _fWidth, const float fAplha );
	// traces
	void AddShotTrace( const CVec3 &_vStart, const CVec3 &_vEnd, NTimer::STime _timeStart, const NDb::SWeaponRPGStats::SShell *pShell );
	// Weather (if pWeatherDesc is 0, switch off)
	virtual void SwitchWeather( bool bActive, NTimer::STime timeLength );
	// explosions
	void AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial );
	void AddDebris( const CVec2 &vSize, const CVec2 &vCenter, float fAngle, float fWidth, const NDb::SMaterial *pMaterial );
	// polyline
	int AddPolyline( const int nID, const std::vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck );
	int AddIndexedPolyline( const int nID, const std::vector<CVec3> &points, const std::vector<uint16_t> &indices, const CVec4 &vColor, bool bDepthCheck );
	void RemovePolyline( const int nID );

	// shoot areas
	void AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter );

	void AddKeyPointArea( const CVec2 &vCenter, float fRadius, const CVec3 &vColor );
	void ClearKeyPointAreas();

	void SetCircle( int nID, float fRadius, const CVec3 &vColor, float fWidth );

	void AddLineMarker( int nID, const CVec2 &vStart, const CVec2 &vEnd, const CVec3 &vColor );
	void ClearMarkers( ESceneMarkerType eType, int nID );
	// picks
	void PickTerrain( CVec3 *pvPos, const CVec2 &vScreenPos );
	void PickZeroHeight( CVec3 *pvPos, const CVec2 &vScreenPos );
	// pick objects; if objects is attached to CMOObj, then returns CMOObj ID
	void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos, const EPickObjectsClass ePickObjsClass );
	void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2,
		EPickObjects eRadiusCoeff, const EPickObjectsClass ePickObjsClass );

	virtual void PickAllObjects( const CVec3 &vAIPos1, const CVec3 &vAIPos2, std::list<SPickObjInfo> *pPickedObjects, std::list<int> *pPickedAttached );

	// set scene light (re-light all scene)
	void SetLight( const NDb::SAmbientLight *pLight );
	//
	void Draw( NGScene::CRTPtr *pTarget );
	// sound scene
	void SetSoundSceneMode( const enum ESoundSceneMode eMode );

	// UI screen manipulation
	void AddScreen( struct IWindow *pScreen );
	void RemoveScreen( struct IWindow *pScreen );
	void RemoveAllScreens();
	NGScene::I2DGameView *GetG2DView();
	NGScene::IGameView *GetGView();
	NGScene::IGameView *GetInterfaceView();

	CCSTime *GetAbsTimer();
	CCSTime *GetGameTimer();
	void ResetTimer( const NTimer::STime &time );

	IFullScreenFader *GetScreenFader();

	//
	CVec2 GetScreenRect();
	void ShowObject( const int nID, const bool bShow );
	void SetFadedObjects( const std::list<int> &objects );
	void SetFadedObjects( const std::list<int> &objects, float fFade );
	void GetObstacleObjects( std::list<int> *pObstacles, const std::list<int> &objects, const CVec2 &vScreenPos );
	void ClearPostEffectObjects();
	void AddPostEffectObjects( const std::list<int> &objects, const CVec4 &vColor );

	// msg processing
	virtual bool ProcessEvent( const SGameMessage &msg );

	// AI debug info
	/*
	virtual class NAIVisInfo::CDebugSegment* CreateSegment( const CSegment &aiSegment, const int nThickness, 
																									const NAIVisInfo::EColor eColor );
	virtual class NAIVisInfo::CDebugCircle* CreateCircle( const CCircle &aiCircle, 
																								const NAIVisInfo::EColor eColor );
	virtual class NAIVisInfo::CDebugMarker* CreateMarker( const list<CVec2> &tiles, 
																								const NAIVisInfo::EColor eColor );

	virtual void SetMarkerPosition( CObjectBase *pMarker, const list<CVec2> &tiles );
	*/
	// warfog
	void SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	void SetWarFogBlend( const float fBlend );

	void AfterLoad();

	// icons
	void SetIcon( const SSceneObjIconInfo &iconInfo );
	void RemoveIcon( const int nID );
	//
	int AddSceneIcon( const int nID, const CVec3 &vCenter, const CVec2 &vSize, const CVec2 &vTexMin, const CVec2 &vTexMax,
		const NDb::SMaterial *pMaterial );
	void RemoveSceneIcon( const int nID );
	void MoveSceneIcon( const int nID, const CVec3 &vCenter );

	CWindController *GetWindController() { return data[eScene]->pWindController; }
	//
	bool MakeMapShot( const SGameMessage &msg );
	
	virtual float GetZ( float x, float y ) const;
	float GetTileHeight( int nX, int nY ) const;
	virtual void UpdateZ( CVec3 *pvPos );
	virtual uint32_t GetNormal( const CVec2 &vPoint ) const;
	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY );

	virtual NAI::IAIMap* GetAIMap() const { return data[eScene]->pAIMap; }

	virtual void AddAttachedMapping( int nAttachObjID, int nMapObjID );
	virtual void RemoveAttachedMapping( int nAttachObjID );

	virtual CSyncSrc<IVisObj>* GetSyncSrc() const;

	bool SetGSceneInternal( bool bIsInternal );
	//
	virtual void SetBackgroundColor( const CVec3 &rvBackgroundColor );
	virtual CVec4 SetBackgroundColor( const CVec4 &rvBackgroundColor );

	virtual bool ToggleGetSizeFromTarget( bool bGetSizesFromTarget );
};


