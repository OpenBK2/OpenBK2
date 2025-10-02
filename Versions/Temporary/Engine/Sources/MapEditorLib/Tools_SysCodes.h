#if !defined(__COMMON_TOOLS__SYS_CODES__)
#define __COMMON_TOOLS__SYS_CODES__
#pragma once

#include "Tools_MnemonicsCollector.h"

class CWMMnemonicCodes : public CMnemonicsCollector<UINT>
{
	public:
	CWMMnemonicCodes();
	string Get( UINT nMessage );
};

#endif // #if !defined(__COMMON_TOOLS__SYS_CODES__)
