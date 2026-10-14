#pragma once
#include "Background.h"

// not tiled, scaled to fit whole window.
class CBackgroundSimpleScallingTexture : public CBackground
{
	OBJECT_BASIC_METHODS( CBackgroundSimpleScallingTexture );

	CDBPtr<NDb::SBackgroundSimpleScallingTexture> pStats;
	CDBPtr<NDb::STexture> pTexture;

	CVec2 vSize; // real size of texture (mainly for n^2-aligned textures)
public:
	void Visit( struct IUIVisitor *pVisitor ) override;
	int operator&( IBinSaver &ss ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void SetTexture( const struct NDb::STexture *_pDesc ) override;
};
