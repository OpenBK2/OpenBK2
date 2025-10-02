#pragma once
#include "background.h"
#include "IconOutline.h"

// texture draw only part that will fit to background size
// texture alignment may be 
class CBackgroundSimpleTexture :	public CBackground
{
	OBJECT_BASIC_METHODS(CBackgroundSimpleTexture);
	
	CDBPtr<NDb::SBackgroundSimpleTexture> pStats;
	CDBPtr<NDb::STexture> pTexture;
	CObj<CIconOutliner> pIcon;

public:
	virtual void Visit( interface IUIVisitor * pVisitor );
	virtual int operator&( interface IBinSaver &ss );
	virtual void SetPos( const CVec2 &vPos, const CVec2 &vSize );
	virtual void InitByDesc( const struct NDb::SUIDesc *_pDesc );
	virtual void SetTexture( const struct NDb::STexture *_pDesc );
	virtual void SetOutline( const CDBID &outlineType );
};

