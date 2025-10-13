#pragma once

#include "MapEditorLib/DefaultInputState.h"
#include "3Dmotor/DBScene.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

//Mapinfo height state edit parameters
#define MODEL_EP_LIGHT_COUNT						0x00000001
#define MODEL_EP_LIGHT_INDEX						0x00000002
#define MODEL_EP_COLOR									0x00000004
#define MODEL_EP_FOV										0x00000008
#define MODEL_EP_TERRAIN								0x00000010
#define MODEL_EP_TERRAIN_SIZE_COUNT			0x00000020
#define MODEL_EP_TERRAIN_SIZE_INDEX			0x00000040
#define MODEL_EP_TERRAIN_COLOR					0x00000080
#define MODEL_EP_TERRAIN_COLOR_OPACITY	0x00000100
#define MODEL_EP_TERRAIN_DOUBLESIDED		0x00000200
#define MODEL_EP_TERRAIN_GRID						0x00000400
#define MODEL_EP_ANIM										0x00000800
#define MODEL_EP_ANIM_COUNT_COUNT				0x00001000
#define MODEL_EP_ANIM_COUNT_INDEX				0x00002000
#define MODEL_EP_ANIM_SPEED_COUNT				0x00004000
#define MODEL_EP_ANIM_SPEED_INDEX				0x00008000
#define MODEL_EP_ANIM_TYPE							0x00010000
#define MODEL_EP_ANIM_RADIUS_COUNT			0x00020000
#define MODEL_EP_ANIM_RADIUS_INDEX			0x00040000
#define MODEL_EP_ANIM_DISTANCE_COUNT		0x00080000
#define MODEL_EP_ANIM_DISTANCE_INDEX		0x00100000
#define MODEL_EP_AI_GEOMETRY						0x00200000
#define MODEL_EP_AI_GEOMETRY_TYPE				0x00400000
#define MODEL_EP_ALL										0xFFFFFFFF


class CModelEditor;
class CModelState : public CDefaultInputState, public ICommandHandler
{
	friend class CModelEditor;
	friend class CModelWindow;

	// Структура данных, с помощью которой конфигурационный диалог общается со стейтом
	struct SEditParameters
	{
		enum EAnimType
		{
			AT_CIRCLE					= 0,
			AT_LINE						= 1,
		};
		enum EAIGeometryType
		{
			AIGT_TRANSPARENT	= 0,
			AIGT_SOLID				= 1,
		};
		//
		UINT nFlags;
		//
		vector<string> lightList;
		int nLightIndex;
		CVec3 vColor;
		int nFOV;
		bool bTerrain;
		vector<string> terrainSizeList;
		int nTerrainSizeIndex;
		CVec3 vTerrainColor;
		int nTerrainColorOpacity;
		bool bTerrainDoubleSided;
		bool bTerrainGrid;
		bool bAnim;
		vector<string> animCountList;
		int nAnimCountIndex;
		vector<string> animSpeedList;
		int nAnimSpeedIndex;
		EAnimType eAnimType;
		vector<string> animRadiusList;
		int nAnimRadiusIndex;
		vector<string> animDistanceList;
		int nAnimDistanceIndex;
		bool bAIGeometry;
		EAIGeometryType eAIGeometryType;
	};

	enum SModelEditorType
	{
		ET_MODEL,
		ET_RPGSTATS,
	};

	//Данные специфичные для данного редактрора
	int nModelSceneID;
	list<int> animModelSceneIDList;
	CObj<CObjectBase> pPlane;
	CObj<NDb::SModel> pMutableModel;

	SModelEditorType eModelEditorType;
	//
	// Данные общего назначения 
	CModelEditor *pModelEditor;
	SEditParameters editParameters;
	//
	void ClearScene( bool bClearAll );
	void UpdateTerrain();
	void UpdateModels( bool bUpdateAll );
	void UpdateLight();
	void UpdateTime( bool bReset );
	void UpdateSceneColor( bool bReset );
	void UpdateAIGeometry( bool bReset );
	//void SetTerrain();
	void SetLight();
	void CenterCamera();
	void SaveCamera( bool bDefaultCamera );
	void SaveCameraForRPGStats();
	void ResetCamera( bool bDefaultCamera );
	//
	void GetEditParameters( UINT nFlags ); // editParameters -> editorSettings
	void SetEditParameters( UINT nFlags ); // editorSettings -> editParameters
	//
	CObjectBase* BuildPlane( const CVec3 &vStart, const CVec2 &vSize, const CVec4 &color, int nXStripCount, int nYStripCount, bool bDoubleSided );
	CObjectBase* BuildPlane( const CVec3 &vStart, const CVec2 &vSize, const CVec4 &color, int nXStripCount, int nYStripCount, float fDiff, bool bDoubleSided );
public:
	CModelState( CModelEditor *_pCModelEditor );
	~CModelState();

	//IInputState
	void Enter();
	void Leave();
	void OnContextMenu( const CTPoint<int> &rMousePoint );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );

	//ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};




