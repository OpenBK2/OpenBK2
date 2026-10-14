#pragma once

#include "InGroupPath.h"

class CInFormationPath : public CInGroupPathBasis
{
	OBJECT_BASIC_METHODS( CInFormationPath );

	ZDATA_( CInGroupPathBasis )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInGroupPathBasis *)this );
		return 0;
	}

public:
};
