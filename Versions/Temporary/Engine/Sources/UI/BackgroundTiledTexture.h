#pragma once
#include "background.h"
#include "..\3DMotor\RectLayout.h"

// tiled background. consists of following elements. 
// LT  T   T  RT
// L   F   F  R
// L   F   F  R
// LB  B   B  RB
// corner element (LT,RT,RB,LB). 
// inner element (F) is tiled to fill remaining space

//CRAP{ DON'T KNOW IS IT REALLY NEEDED. NOT IMPLEMENTED ANYWAY.
//In case that only LT is given, all other are generated from it by rotating.
// the same is for border elements (T,L,R,B), the base element is T.
//CRAP}
namespace NDb
{
	struct SBackgroundTiledTexture;
	struct SSubRect;
}

class CBackgroundTiledTexture : public CBackground
{
	OBJECT_BASIC_METHODS(CBackgroundTiledTexture)
	CPtr<NDb::SBackgroundTiledTexture> pDesc;
	CRectLayout layout;
	//vector<SGFXRect2> rects;						// drawind rects
	
	//void InitTiles( NDb::SSubRect *pSub );
	void InitBorderAndFill();

	// divides SSubRect into drawing rect
	void DivideSubrects( const NDb::SSubRect &in, CRectLayout *pArr );

public:
	CBackgroundTiledTexture();
	virtual void Visit( interface IUIVisitor * pVisitor );
	virtual int operator&( IBinSaver &ss );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	virtual void SetPos( const CVec2 &vPos, const CVec2 &vSize );
};
