#pragma once

#include "ED_B2_M1/EditorScene.h"

#include <cstdint>

class CEditorScene : public IEditorScene
{
	OBJECT_NOCOPY_METHODS( CEditorScene )

	ZDATA
	ZEND int operator&( IBinSaver &f ) override
	{
		return 0;
	}

public:
	void RemoveAllScreens() override;
	void AddScreen( struct IWindow *pScreen ) override;
	void RemoveScreen( struct IWindow *pScreen ) override;

	NGScene::I2DGameView *GetG2DView() override;
	NGScene::IGameView *GetGView() override;
	CVec2 GetScreenRect() override;
	class CCSTime *GetGameTimer() override;
	uint32_t GetNormal( const CVec2 &vPoint ) const override;
	void ClearMarkers( ESceneMarkerType eType, int nID ) override;
	void AddShootArea( int nID,
	                   float fStartAngle,
	                   float fEndAngle,
	                   float fMinRadius,
	                   float fMaxRadius,
	                   const CVec3 &vColor,
	                   const CVec2 &vCenter ) override;
	void SwitchScene( const EScene eScene ) override;

	int AddObject( const int nID,
	               const NDb::SModel *pModel,
	               const CVec3 &vPos,
	               const CQuat &qRot,
	               const CVec3 &vScale,
	               ESceneObjAnimMode eAnimMode,
	               NGScene::IGameView::SMeshInfo *pMeshInfo,
	               const bool bHasReflection = false ) override;
	bool
	MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale = CVec3( 1, 1, 1 ) ) override;
	void RemoveObject( const int nID ) override;
	void SetFadedObjects( const std::list<int> &objects ) override;
	void SetFadedObjects( const std::list<int> &objects, float fFade ) override;
	int AddEffect( const int nID,
	               const NDb::SEffect *pEffect,
	               NTimer::STime timeStart,
	               const CVec3 &vPos,
	               const CQuat &qRot ) override;

	int AddPolyline( const int nID, const std::vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck ) override;
	int AddIndexedPolyline( const int nID,
	                        const std::vector<CVec3> &points,
	                        const std::vector<uint16_t> &indices,
	                        const CVec4 &vColor,
	                        bool bDepthCheck ) override;
	void RemovePolyline( const int nID ) override;
	bool ToggleShow( ESceneShow eShow ) override;
	bool ToggleAIGeometryMode() override;
	bool IsShowOn( ESceneShow eShow ) override;

	ITerraManager *GetTerraManager() override;
	bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;
	float GetZ( float x, float y ) const override;
	float GetTileHeight( int nX, int nY ) const override;
	void UpdateZ( CVec3 *pvPos ) override;

	bool SetupMode( ESceneMode eMode, bool bEditorMode ) override;
	void SetLight( const NDb::SAmbientLight *pLight ) override;
	void SetBackgroundColor( const CVec3 &rvBackgroundColor ) override;
	CVec4 SetBackgroundColor( const CVec4 &rvBackgroundColor ) override;
	void ClearScene( const EScene eScene2Clear ) override;
	void Draw( NGScene::CRTPtr *pTarget ) override;

	void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos ) override;
	void PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2 ) override;
	void InitHeights4Editor( int nSizeX, int nSizeY ) override;

	void ShowObject( const int nID, const bool bShow ) override;
	bool DoesTerraManagerExist() const override;

	NAnimation::ISkeletonAnimator *GetAnimator( const int nID ) override;

	bool ToggleGetSizeFromTarget( bool bGetSizesFromTarget ) override;

	void SetWarFog( const CArray2D<unsigned char> &fog, float fScale ) override;
	void SetWarFogBlend( const float fBlend ) override;
};
