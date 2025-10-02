#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CTextureState : public CDefaultInputState, public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CTextureState)
public:
	//Life-cycle
	CTextureState() {}
	CTextureState( class CTextureEditor *_pEditor );
	virtual ~CTextureState();

	//IInputState
	void Enter();
	void Leave();

	// methods
protected:

	// members
protected:
	class CTextureEditor *pEditor;
};




