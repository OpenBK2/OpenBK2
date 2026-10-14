#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CUIRunModeState : public CDefaultInputState
{
public:
	//Life-cycle
	CUIRunModeState( class CWindowSimpleSharedEditor *_pEditor, const std::string &rszTypeName, const CDBID &rDBID );
	~CUIRunModeState() override;

	//IInputState
	void Enter() override;
	void Leave() override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;

	// members
protected:
	class CWindowSimpleSharedEditor *pEditor;
	CDBID dbid;
	std::string szTypeName;
};
