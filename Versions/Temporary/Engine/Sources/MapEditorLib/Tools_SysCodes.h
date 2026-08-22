#pragma once

#include "Tools_MnemonicsCollector.h"

class CWMMnemonicCodes : public CMnemonicsCollector<unsigned>
{
	public:
	CWMMnemonicCodes();
	string Get( unsigned nMessage );
};


