#pragma once
#include "Background.h"
#include "IconOutline.h"

// texture draw only part that will fit to background size
// texture alignment may be
class CBackgroundSimpleTexture : public CBackground
{
	OBJECT_BASIC_METHODS( CBackgroundSimpleTexture );

	CDBPtr<NDb::SBackgroundSimpleTexture> pStats;
	CDBPtr<NDb::STexture> pTexture;
	CObj<CIconOutliner> pIcon;

public:
	void Visit( struct IUIVisitor *pVisitor ) override;
	int operator&( struct IBinSaver &ss ) override;
	void SetPos( const CVec2 &vPos, const CVec2 &vSize ) override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
	void SetTexture( const struct NDb::STexture *_pDesc ) override;
	void SetOutline( const CDBID &outlineType ) override;
};
