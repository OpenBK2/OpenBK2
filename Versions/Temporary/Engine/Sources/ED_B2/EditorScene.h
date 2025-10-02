#pragma once

#include "ED_B2_M1/EditorScene.h"

class CEditorScene : public IEditorScene
{
	OBJECT_NOCOPY_METHODS( CEditorScene )
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	virtual void RemoveAllScreens();
	virtual void AddScreen( struct IWindow *pScreen );
	virtual void RemoveScreen( struct IWindow *pScreen );

	virtual NGScene::I2DGameView *GetG2DView();
	virtual NGScene::IGameView *GetGView();
	virtual CVec2 GetScreenRect();
	virtual class CCSTime *GetGameTimer();
	virtual DWORD GetNormal( const CVec2 &vPoint ) const;
	virtual void ClearMarkers( ESceneMarkerType eType, int nID );
	virtual void AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter );
	virtual void SwitchScene( const EScene eScene );

	virtual int AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, 
												 const CVec3 &vScale, ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo,
												 const bool bHasReflection = false );
	virtual bool MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale = CVec3(1, 1, 1) );
	virtual void RemoveObject( const int nID );
	virtual void SetFadedObjects( const list<int> &objects );
	virtual void SetFadedObjects( const list<int> &objects, float fFade );
	virtual int AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot );

	virtual int AddPolyline( const int nID, const vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck );
	virtual int AddIndexedPolyline( const int nID, const vector<CVec3> &points, const vector<WORD> &indices, const CVec4 &vColor, bool bDepthCheck );
	virtual void RemovePolyline( const int nID );
	virtual bool ToggleShow( ESceneShow eShow );
	virtual bool ToggleAIGeometryMode();
	virtual bool IsShowOn( ESceneShow eShow );

	virtual ITerraManager *GetTerraManager();
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	virtual float GetZ( float x, float y ) const;
	virtual float GetTileHeight( int nX, int nY ) const;
	virtual void UpdateZ( CVec3 *pvPos );

	virtual bool SetupMode( ESceneMode eMode, bool bEditorMode );
	virtual void SetLight( const NDb::SAmbientLight *pLight );
	virtual void SetBackgroundColor( const CVec3 &rvBackgroundColor );
	virtual CVec4 SetBackgroundColor( const CVec4 &rvBackgroundColor );
	virtual void ClearScene( const EScene eScene2Clear );
	virtual void Draw( NGScene::CRTPtr *pTarget );

	virtual void PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos );
	virtual void PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2 );
	virtual void InitHeights4Editor( int nSizeX, int nSizeY );

	virtual void ShowObject( const int nID, const bool bShow );
	virtual bool DoesTerraManagerExist() const;

	virtual NAnimation::ISkeletonAnimator *GetAnimator( const int nID );

	virtual bool ToggleGetSizeFromTarget( bool bGetSizesFromTarget );

	virtual void SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	virtual void SetWarFogBlend( const float fBlend );
};


