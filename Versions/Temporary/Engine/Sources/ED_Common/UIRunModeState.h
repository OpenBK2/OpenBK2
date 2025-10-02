#pragma once

#include "../MapEditorLib/DefaultInputState.h"

class CUIRunModeState : public CDefaultInputState
{
public:
	//Life-cycle
	CUIRunModeState( class CWindowSimpleSharedEditor *_pEditor, const string &rszTypeName, const CDBID &rDBID );
	virtual ~CUIRunModeState();
	
	//IInputState
	void Enter();
	void Leave();
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );

	// members
protected:
	class CWindowSimpleSharedEditor *pEditor;
	CDBID dbid;
	string szTypeName;
};



