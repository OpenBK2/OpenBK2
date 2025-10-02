#if !defined(__MODEL_DW_WINDOW__)
#define __MODEL_DW_WINDOW__
#pragma once

#include "..\MapEditorLib\ResizeDialog.h"
#include "ModelState.h"
#include "ResourceDefines.h"

class CModelWindow : public CResizeDialog, public ICommandHandler
{
	bool bCreateControls;

	CComboBox	wndLightComboBox;
	CComboBox	wndTerrainSizeComboBox;
	CComboBox	wndAnimCountComboBox;
	CComboBox	wndAnimSpeedComboBox;
	CComboBox	wndAnimRadiusComboBox;
	CComboBox	wndAnimDistanceComboBox;

	bool GetEditParameters( CModelState::SEditParameters *pEditParameters );
	bool SetEditParameters( const CModelState::SEditParameters &rEditParameters );
	void UpdateControls( const CModelState::SEditParameters &rEditParameters );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	
	afx_msg void OnClickedLightButton();
	afx_msg void OnClickedAnimSpeedDown();
	afx_msg void OnClickedAnimSpeedUp();
	afx_msg void OnClickedTerrain();
	afx_msg void OnClickedTerrainDoubleSided();
	afx_msg void OnClickedTerrainGrid();
	afx_msg void OnClickedAnim();
	afx_msg void OnClickedAnimType();
	afx_msg void OnClickedAIGeometry();
	afx_msg void OnClickedAIGeometryType();
	afx_msg void OnSelChangeLightCombo();
	afx_msg void OnSelChangeTerrainSizeCombo();
	afx_msg void OnSelChangeAnimCountCombo();
	afx_msg void OnSelChangeAnimSpeedCombo();
	afx_msg void OnSelChangeAnimCircleRadiusCombo();
	afx_msg void OnSelChangeAnimLineDistanceCombo();
	afx_msg void OnChangeSceneColor();
	afx_msg void OnChangeFOV();
	afx_msg void OnChangeTerrainColor();
	afx_msg void OnChangeTerrainColorOpacity();
	afx_msg void OnClickedSceneColorButton();
	afx_msg void OnClickedTerrainColorButton();
	//
	afx_msg void OnTimer( UINT nIDEvent );

	LONG nSceneColorTimer;
  inline UINT GetSceneColorID() { return 1; }
  inline UINT GetSceneColorTimerInterval() { return 500; }	// Частота в миллисекундах
  void SetSceneColorTimer();
  void KillSceneColorTimer();
  void OnSceneColorTimer();
	//
	LONG nTerrainColorTimer;
  inline UINT GetTerrainColorID() { return 2; }
  inline UINT GetTerrainColorTimerInterval() { return 500; }	// Частота в миллисекундах
  void SetTerrainColorTimer();
  void KillTerrainColorTimer();
  void OnTerrainColorTimer();
	//
	LONG nTerrainColorOpacityTimer;
  inline UINT GetTerrainColorOpacityID() { return 3; }
  inline UINT GetTerrainColorOpacityTimerInterval() { return 500; }	// Частота в миллисекундах
  void SetTerrainColorOpacityTimer();
  void KillTerrainColorOpacityTimer();
  void OnTerrainColorOpacityTimer();
	//
	LONG nFOVTimer;
  inline UINT GetFOVID() { return 4; }
  inline UINT GetFOVTimerInterval() { return 500; } // Частота в миллисекундах
  void SetFOVTimer();
  void KillFOVTimer();
  void OnFOVTimer();

	//CResizeDialog
	bool IsDrawGripper() { return false; }

public:
	enum { IDD = IDD_TAB_MODEL_TOOL };

	CModelWindow( CWnd* pParent = 0 );
	~CModelWindow();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__MODEL_DW_WINDOW__)
