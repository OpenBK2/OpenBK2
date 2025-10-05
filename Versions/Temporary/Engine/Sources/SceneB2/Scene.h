#pragma once

#include "SceneB2_export.h"


#include "Stats_B2_M1/DBMapInfo.h"
#include "3Dmotor/GView.h"
#include "Stats_B2_M1/IconsSet.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/AnimModes.h"
#include "SceneTypes.h"

struct SGameMessage;
namespace NDb
{
	struct SModel;
	struct SAmbientLight;
	struct SEffect;
	struct SMaterial;
	struct SSceneConsts;
	struct SWeatherDesc;
	enum ESelectionType;
};
namespace NGScene
{
	class I2DGameView;
	class CRTPtr;
};
namespace NAnimation
{
	struct ISkeletonAnimator;
};
class CWindController;

namespace NAIVisInfo
{
	class CDebugSegment;
	class CDebugCircle;
	class CDebugMarker;

	enum EColor;
};

struct IVisObj;
template<class T> class CSyncSrc;

namespace NAI
{
	struct IAIMap;
}

enum ESceneSubObjType
{
	ESSOT_WEAPON,
	ESSOT_EXHAUST,
	ESSOT_GUN_FIRE,
	ESSOT_LIGHT,				// headlights of objects
	ESSOT_WINDOW,
	ESSOT_PROJECTILE,
	ESSOT_WATER_DROPS,
	ESSOT_PLATFORMS,
	ESSOT_GUNS,
	ESSOT_DEATH_EFFECTS,
	ESSOT_SLOTS,
	ESSOT_PROPELLERS,
	ESSOT_EXTERN, // extern attaches (pointers to movable objects etc.)
	__ESSOT_COUNTER,
};

enum ESceneAttachMode
{
	ESAT_REPLACE_ON_BONE,
	ESAT_REPLACE_ON_TYPE,
	ESAT_NO_REPLACE,
};

class CCSTime;
struct IFullScreenFader;

struct SObjectFilter
{
	virtual bool operator() ( int ) const { return false; };
};

struct IScene : public CObjectBase
{
	enum { tidTypeID = 0x10073C40 };
	//
	enum EPickObjects
	{
		PO_TOUCH = 1,
		PO_CENTER_INSIDE = 0,
		PO_WHOLE_INSIDE = -1,
	};
	//
	enum EPickObjectsClass
	{
		EPOC_OBJECTS,
		EPOC_ICONS
	};
	//
	virtual void Init() = 0;
	virtual void SwitchScene( const EScene eScene ) = 0;
	virtual const EScene GetCurrentScene() const = 0;
	virtual void SwitchSceneAfterLoad( const EScene eScene ) = 0;
	//
	virtual bool SetupMode( ESceneMode eMode, bool bEditorMode ) = 0;
	virtual bool IsEditorMode() = 0;
	//
	virtual void SetSceneConsts( const NDb::SSceneConsts *pSceneConsts ) = 0;
	virtual const NDb::SSceneConsts *GetSceneConsts() = 0;
	//virtual void SetFOV( const float fFOV ) = 0;
	//
	virtual bool ToggleShow( ESceneShow eShow ) = 0;
	virtual bool IsShowOn( ESceneShow eShow ) = 0;
	virtual void UpdateGrid( int nPosX, int nPosY, int nSizeX, int nSizeY ) = 0;
	virtual void UpdateGrid( int nPosX, int nPosY, int nSizeX, int nSizeY, bool bAIGrid ) = 0;
	virtual bool ToggleAIGeometryMode() = 0;
	//
	virtual void ClearScene( const EScene eScene2Clear ) = 0;
	// set scene light (re-light all scene)
	virtual void SetLight( const NDb::SAmbientLight *pLight ) = 0;
	// 
	virtual struct ITerraManager *GetTerraManager() = 0;
	virtual bool DoesTerraManagerExist() const = 0;
	// add object to scene. returns new ID
	virtual int AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, 
												 const CVec3 &vScale, ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pLowLevelModel = 0,
												 const bool bHasReflection = false ) = 0;
	virtual int AddObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace,
												 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bNeedReflection = false ) = 0;

	// CRAP - ugly, should to pass a special object to screen
	virtual void AddInterfaceObject( struct IWindow *pScreen, int nID, const NDb::SModel *pModel, const CVec2 &vScreenPos, const CVec2 &vElementSize ) = 0;
	virtual void RemoveInterfaceObject( struct IWindow *pScreen, const int nID ) = 0;
	//
	virtual bool ChangeModel( const int nObjectID, const NDb::SModel *pModel ) = 0;
	virtual NAnimation::ISkeletonAnimator *GetAnimator( const int nID, bool bRefreshAnimator = true ) = 0;
	// CRAP - ugly, should to pass a special object to screen
	virtual NAnimation::ISkeletonAnimator *GetInterfaceObjAnimator( IWindow *pScreen, const int nID ) = 0;
	// get animator of nTargetID object or it's subobject with bone szBoneName
	virtual NAnimation::ISkeletonAnimator* GetAnimator( const int nTargetID, const std::string &szBoneName ) = 0;

	virtual void AttachSubModel( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const NDb::SModel *pSubModel, ESceneAttachMode eMode, const int nNumber, bool bForceAnimated, const bool bConstantOffset ) = 0;
	virtual void AttachSubModel( const int nTargetID, ESceneSubObjType eType, const NDb::SModel *pSubModel, ESceneAttachMode eMode, const int nNumber, bool bForceAnimated, const CVec3 &vOffset ) = 0;
	virtual struct IAttachedObject* GetAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber ) = 0;
	virtual void RemoveAllAttached( const int nTargetID, ESceneSubObjType eType ) = 0;
	virtual void RemoveAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber ) = 0;

	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, bool bVertical = false ) = 0;
	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, CFuncBase<SFBTransform> *pTransform, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, const int nBoneIndex = -1, bool bVertical = false ) = 0;
	virtual void AttachEffect( const int nTargetID, ESceneSubObjType eType, const std::string &szBoneName, const SHMatrix &mOffset, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode ) = 0;

	// Attach light FX: to animated (with bone name) and to static objects (without bone name)
	virtual void AttachLightEffect( const int nTargetID, const NDb::SAttachedLightEffect *pLight, NTimer::STime timeStart, ESceneAttachMode eMode, const bool bInEditor = false, int nHoldID = -1 ) = 0;
	
	// add effect to scene. returns new ID
	virtual int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot ) = 0;
	virtual int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace ) = 0;
	virtual void AddEffect( const int nID, const std::string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace ) = 0;
	virtual int AddPointLight( const int nID, const CVec3 &ptColor, const CVec3 &ptOrigin, float fR ) = 0;
	virtual bool GetVisObjPlacement( const int nID, SFBTransform *pTransform ) const = 0;

	virtual void StopEffectGeneration( const int nID, NTimer::STime time ) = 0;
	// remove object from scene by ID, returned by AddObject()
	virtual void RemoveObjectPickability( const int nID ) = 0;
	virtual void RemoveObject( const int nID ) = 0;
	// move object
	virtual bool MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale = CVec3(1, 1, 1) ) = 0;
	virtual bool MoveObject( const int nID, const SHMatrix &mPlace ) = 0;
	// selection operations
	virtual void SelectObject( const int nID, const CVec3 &vPos, const float fSelScale, const NDb::ESelectionType eSelType ) = 0;
	virtual void UnselectObject( const int nID ) = 0;
	// use nID = -1 for squad selection, use fFadeOutTime = -1.0f for no fade out
	virtual int AddSelection( int nID, const CVec3 &vPos, float fSelScale, NDb::ESelectionType eSelType, 
		float fFadeInTime, float fFadeOutTime ) = 0;
	virtual void RemoveSelection( int nID ) = 0;
	virtual void ClearSelection() = 0;
	// tracks operations
	virtual void AddTrack( const int nID, const float fFadingSpeed,
													const CVec2 &_v1, const CVec2 &_v2, const CVec2 &_v3, const CVec2 &_v4,
													const CVec2 &vNorm, const float _fWidth, const float fAplha ) = 0;
	// Weather 
	virtual void SwitchWeather( bool bActive, NTimer::STime timeLength ) = 0;
	// traces operations
	virtual void AddShotTrace( const CVec3 &vStart, const CVec3 &vEnd, NTimer::STime timeStart, const NDb::SWeaponRPGStats::SShell *pShell ) = 0;
	// explosions
	virtual void AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial ) = 0;
	virtual void AddDebris( const CVec2 &vSize, const CVec2 &vCenter, float fAngle, float fWidth, const NDb::SMaterial *pMaterial ) = 0;
	// polyline
	virtual int AddPolyline( const int nID, const std::vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck ) = 0;
	virtual int AddIndexedPolyline( const int nID, const std::vector<CVec3> &points, const std::vector<WORD> &indices, const CVec4 &vColor, bool bDepthCheck ) = 0;
	virtual void RemovePolyline( const int nID ) = 0;
	// picks
	virtual void PickTerrain( CVec3 *pvPos, const CVec2 &vScreenPos ) = 0;
	virtual void PickZeroHeight( CVec3 *pvPos, const CVec2 &vScreenPos ) = 0;
	// pick objects; if objects is attached to CMOObj, then returns CMOObj ID
	virtual void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos, const EPickObjectsClass ePickObjsClass = EPOC_OBJECTS ) = 0;
	virtual void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2,
		EPickObjects eRadiusCoeff, const EPickObjectsClass ePickObjsClass = EPOC_OBJECTS ) = 0;

	struct SPickObjInfo
	{
		int nObjID;
		CVec3 vPickPoint;
		CVec3 vNormal;
	};

	virtual void PickAllObjects( const CVec3 &vAIPos1, const CVec3 &vAIPos2, std::list<SPickObjInfo> *pPickedObjects, std::list<int> *pPickedAttached ) = 0;
	//
	virtual void Draw( NGScene::CRTPtr *pTarget ) = 0;
	// Sound Scene
	// virtual void SetSoundSceneMode( const enum ESoundSceneMode eMode ) = 0;

	// UI screen manipulation
	virtual void AddScreen( struct IWindow *pScreen ) = 0;
	virtual void RemoveScreen( struct IWindow *pScreen ) = 0;
	virtual void RemoveAllScreens() = 0;
	virtual NGScene::I2DGameView *GetG2DView() = 0;
	virtual NGScene::IGameView *GetGView() = 0;
	virtual NGScene::IGameView *GetInterfaceView() = 0;

	virtual CCSTime *GetAbsTimer() = 0;
	virtual CCSTime *GetGameTimer() = 0;
	virtual void ResetTimer( const NTimer::STime &time ) = 0;

	virtual IFullScreenFader *GetScreenFader() = 0;
	virtual CVec2 GetScreenRect() = 0;
	//
	virtual void ShowObject( const int nID, const bool bShow ) = 0;
	virtual void SetFadedObjects( const std::list<int> &objects ) = 0;
	virtual void SetFadedObjects( const std::list<int> &objects, float fFade ) = 0;

	virtual void GetCoveredObjects( std::list<int> *pCoveredObjects, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle ) = 0;
	virtual void GetObstacleObjects( std::list<int> *pObstacleObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle ) = 0;

	virtual void ClearPostEffectObjects() = 0;
	virtual void AddPostEffectObjects( const std::list<int> &objects, const CVec4 &vColor ) = 0;

	// msg processing
	virtual bool ProcessEvent( const SGameMessage &msg ) = 0;

	// warfog
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale ) = 0;
	virtual void SetWarFogBlend( const float fBlend ) = 0;

	// after load
	virtual void AfterLoad() = 0;
	virtual void AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter ) = 0;
	virtual void AddLineMarker( int nID, const CVec2 &vStart, const CVec2 &vEnd, const CVec3 &vColor ) = 0;
	virtual void ClearMarkers( ESceneMarkerType eType, int nID ) = 0;

	virtual void AddKeyPointArea( const CVec2 &vCenter, float fRadius, const CVec3 &vColor ) = 0;
	virtual void ClearKeyPointAreas() = 0;

	virtual void SetCircle( int nID, float fRadius, const CVec3 &vColor, float fWidth = 1.0f ) = 0;

	// icons
	virtual void SetIcon( const SSceneObjIconInfo &iconInfo ) = 0;
	virtual void RemoveIcon( const int nID ) = 0;
	//
	virtual int AddSceneIcon( const int nID, const CVec3 &vCenter, const CVec2 &vSize, const CVec2 &vTexMin, const CVec2 &vTexMax,
		const NDb::SMaterial *pMaterial ) = 0;
	virtual void RemoveSceneIcon( const int nID ) = 0;
	virtual void MoveSceneIcon( const int nID, const CVec3 &vCenter ) = 0;

	virtual CWindController *GetWindController() = 0;

	virtual float GetZ( float x, float y ) const = 0;
	virtual void UpdateZ( CVec3 *pvPos ) = 0;
	virtual float GetTileHeight( int nX, int nY ) const = 0;
	virtual DWORD GetNormal( const CVec2 &vPoint ) const = 0;
	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;

	virtual void InitHeights4Editor( int nSizeX, int nSizeY ) = 0;

	virtual NAI::IAIMap* GetAIMap() const = 0;

	virtual void AddAttachedMapping( int nAttachObjID, int nMapObjID ) = 0;
	virtual void RemoveAttachedMapping( int nAttachObjID ) = 0;

	virtual CSyncSrc<IVisObj>* GetSyncSrc() const = 0;
	virtual bool SetGSceneInternal( bool bIsInternal ) = 0;
	virtual void SetBackgroundColor( const CVec3 &rvBackgroundColor ) = 0;
	virtual CVec4 SetBackgroundColor( const CVec4 &rvBackgroundColor ) = 0;

	virtual bool ToggleGetSizeFromTarget( bool bGetSizesFromTarget ) = 0;
};
inline IScene* Scene() { return Singleton<IScene>(); }
SCENEB2_EXPORT IScene* CreateScene();

DEFINE_DG_CONSTANT_NODE( CCSFBTransform, SFBTransform );
DEFINE_DG_CONSTANT_NODE( CCCVec3, CVec3 );
DEFINE_DG_CONSTANT_NODE( CCSBound, SBound );

struct IAIVisitor;
struct IVisObj: virtual public CObjectBase
{
	//	virtual void Visit( IRenderVisitor* ) {}
	virtual void Visit( IAIVisitor* ) {}
	//	virtual void Visit( ISoundVisitor* ) {}
};


