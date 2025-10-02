#pragma once

#include "Tools_MnemonicsCollector.h"

class CWMMnemonicCodes : public CMnemonicsCollector<UINT>
{
	public:
	CWMMnemonicCodes();
	string Get( UINT nMessage );
};


