#pragma once

#include "3Dmotor/GView.h"

#include <cstdint>

namespace NDb
{
	struct SModel;
	struct SAmbientLight;
}
namespace NGScene
{
	class I2DGameView;
	class IGameView;
	class CRTPtr;
}

namespace NAnimation
{
	struct ISkeletonAnimator;
}

enum EScene;
enum ESceneMarkerType;
enum ESceneMode;
enum ESceneObjAnimMode;
enum ESceneShow;
struct ITerraManager;

struct IEditorScene : public CObjectBase
{
	enum { tidTypeID = 0x301CBB41 };

	virtual void RemoveAllScreens() = 0;
	virtual void AddScreen( struct IWindow *pScreen ) = 0;
	virtual void RemoveScreen( struct IWindow *pScreen ) = 0;

	virtual NGScene::I2DGameView *GetG2DView() = 0;
	virtual NGScene::IGameView *GetGView() = 0;
	virtual CVec2 GetScreenRect() = 0;
	virtual class CCSTime *GetGameTimer() = 0;
	virtual uint32_t GetNormal( const CVec2 &vPoint ) const = 0;
	virtual void ClearMarkers( ESceneMarkerType eType, int nID ) = 0;
	virtual void AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter ) = 0;
	virtual void SwitchScene( const EScene eScene ) = 0;

	virtual int AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, 
												 const CVec3 &vScale, ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo,
												 const bool bHasReflection = false ) = 0;
	virtual bool MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale = CVec3(1, 1, 1) ) = 0;
	virtual void RemoveObject( const int nID ) = 0;
	virtual void SetFadedObjects( const list<int> &objects ) = 0;
	virtual void SetFadedObjects( const list<int> &objects, float fFade ) = 0;
	virtual int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot ) = 0;

	virtual int AddPolyline( const int nID, const vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck ) = 0;
	virtual int AddIndexedPolyline( const int nID, const vector<CVec3> &points, const vector<uint16_t> &indices, const CVec4 &vColor, bool bDepthCheck ) = 0;
	virtual void RemovePolyline( const int nID ) = 0;
	virtual bool ToggleShow( ESceneShow eShow ) = 0;
	virtual bool ToggleAIGeometryMode() = 0;

	virtual bool IsShowOn( ESceneShow eShow ) = 0;

	virtual ITerraManager *GetTerraManager() = 0;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual float GetZ( float x, float y ) const = 0;
	virtual float GetTileHeight( int nX, int nY ) const = 0;
	virtual void UpdateZ( CVec3 *pvPos ) = 0;

	virtual bool SetupMode( ESceneMode eMode, bool bEditorMode ) = 0;
	virtual void SetLight( const NDb::SAmbientLight *pLight ) = 0;
	virtual void SetBackgroundColor( const CVec3 &rvBackgroundColor ) = 0;
	virtual CVec4 SetBackgroundColor( const CVec4 &rvBackgroundColor ) = 0;
	virtual void ClearScene( const EScene eScene2Clear ) = 0;
	virtual void Draw( NGScene::CRTPtr *pTarget ) = 0;

	virtual void PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos ) = 0;
	virtual void PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2 ) = 0;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY ) = 0;

	virtual void ShowObject( const int nID, const bool bShow ) = 0;
	virtual bool DoesTerraManagerExist() const { return false; }

	virtual NAnimation::ISkeletonAnimator *GetAnimator( const int nID ) = 0;

	virtual bool ToggleGetSizeFromTarget( bool bGetSizesFromTarget ) = 0;

	// warfog
	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale ) = 0;
	virtual void SetWarFogBlend( const float fBlend ) = 0;
};

IEditorScene* EditorScene();

