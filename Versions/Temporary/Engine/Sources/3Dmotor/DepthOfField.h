#pragma once

class CTransformStack;

namespace NGScene
{

class CSceneFragments;
class IRender;

struct SDepthOfField : public CObjectBase
{
	OBJECT_BASIC_METHODS( SDepthOfField )
	//
public:
	float fFocalDist;
	float fFocusRange;
	//float fFocusRangeBackward;
	//
	SDepthOfField() : fFocalDist(5.0f), fFocusRange(20.0f)/*, fFocusRangeBackward(3.0f)*/ {}
	SDepthOfField( float _fFocalDist, float _fFocusRange/*, float _fFocusRangeBackward*/ ) :
		fFocalDist(_fFocalDist), fFocusRange(_fFocusRange)/*, fFocusRangeBackward(_fFocusRangeBackward)*/ {}
};

void ProcessDepthOfField( const SDepthOfField *pDOF, const CSceneFragments *pScene, const CTransformStack *pTS, IRender *pRender );

} // namespace NGScene

