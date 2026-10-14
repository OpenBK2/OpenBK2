#pragma once
#include "DBUISpecificB2.h"
#include "Misc/2Darray.h"
#include "UI/Background.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

#include <zconf.h>

class CBackgroundFrameSequence : public CBackground
{
	OBJECT_BASIC_METHODS( CBackgroundFrameSequence );

	ZDATA_( CBackground )
	CDBPtr<NDb::SBackgroundFrameSequence> pStats;
	CDBPtr<NDb::STexture> pTexture;
	int nFrame;
	int nTime;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CBackground *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &pTexture );
		f.Add( 4, &nFrame );
		f.Add( 5, &nTime );
		return 0;
	}

	NTimer::STime timeStart;

public:
	CBackgroundFrameSequence();

	//{ IWindowPart
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void Visit( struct IUIVisitor *pVisitor ) override;
	void SetTexture( const struct NDb::STexture *pDesc ) override;
	//}
};
