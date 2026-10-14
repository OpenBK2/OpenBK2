#pragma once

#include "MapEditorLib/DefaultInputState.h"

class CTextureState : public CDefaultInputState, public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextureState )
public:
	//Life-cycle
	CTextureState() {}

	CTextureState( class CTextureEditor *_pEditor );
	~CTextureState() override;

	//IInputState
	void Enter() override;
	void Leave() override;

	// methods
protected:
	// members
protected:
	class CTextureEditor *pEditor;
};
