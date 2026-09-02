#pragma once

#include "Tools_MnemonicsCollector.h"

class CWMMnemonicCodes : public CMnemonicsCollector<unsigned>
{
	public:
	CWMMnemonicCodes();
	std::string Get( unsigned nMessage );
};


