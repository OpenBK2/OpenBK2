
#pragma once

#include "InterfaceScreenBase.h"
#include "MultiplayerCommandProcessor.h"
#include "MultiplayerCommandManager.h"

class CInterfaceMPScreenBase : public CInterfaceScreenBase, public CMPUIMessageTranslator
{
private:
protected:
	CInterfaceMPScreenBase( const std::string &szInterfaceType, const std::string &szBindSection );

	bool StepLocal( bool bAppActive );
public:
};


