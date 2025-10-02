#if !defined(__MODEL_EDITOR_SETTINGS__)
#define __MODEL_EDITOR_SETTINGS__
#pragma once


class CModelEditorSettings
{
public:
	//
	static const CVec3 vShift;				// vis tiles
	static const float fDefaultDiff;	// vis tiles
	//
	struct SCameraPlacement
	{
		CVec3 vAnchor;
		float fDistance;
		float fPitch;
		float fYaw;
		float fFOV;
		bool bValid;
		//
		SCameraPlacement();
		// serializing...
		int operator&( IXmlSaver &xs );
		//
		bool IsValid() const { return bValid; }
		void SetValid( bool _bValid ) { bValid = _bValid; }
	};

	bool bShowToolbar;
	bool bShowTool;
	bool bDrawTerrain;
	bool bDrawAnimations;
	bool bDrawAIGeometry;
	bool bDoubleSided;
	bool bAnimationsCircle;
	bool bShowGrid;
	bool bShowSolidAIGeometry;
	//
	SCameraPlacement defaultCamera;
	SCameraPlacement currentCamera;
	//
  CDBID lightDBID;
  CDBID mapInfoDBID;
	CDBID defaultGeometryDBID;
	CDBID defaultMaterialDBID;
	//
	float nGridSize;									// in vis tiles ( 8 ... 64 )
	CVec3 vSceneColor;								// CVec3( 0 ... 255 )
	CVec4 vTerrainColor;							// CVec4( 0 ... 255 )
	int nMaxAnimationsCount;					// 0 ... 64
	float fAnimationsCircleDistance;	// in vis tiles ( 2 ... 32 ) 
	float fAnimationsBetweenDistance; // in vis tiles ( 2 ... 16 )
	int nGameTimerSpeed;							// -10 ... +10
	//
	CModelEditorSettings();
	// serializing...
	int operator&( IXmlSaver &xs );
};

#endif // !defined(__MODEL_EDITOR_SETTINGS__)
