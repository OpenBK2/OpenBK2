#pragma once
#include "background.h"



// not tiled, scaled to fit whole window.
class CBackgroundSimpleScallingTexture : public CBackground
{
	OBJECT_BASIC_METHODS(CBackgroundSimpleScallingTexture);

	CDBPtr<NDb::SBackgroundSimpleScallingTexture> pStats;
	CDBPtr<NDb::STexture> pTexture;

	CVec2 vSize; // real size of texture (mainly for n^2-aligned textures)
public:
	virtual void Visit( interface IUIVisitor * pVisitor );
	virtual int operator&( IBinSaver &ss );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	virtual void SetTexture( const struct NDb::STexture *_pDesc );
};

