
#pragma once

#include "InterfaceScreenBase.h"
#include "MultiplayerCommandManager.h"
#include "MultiplayerCommandProcessor.h"

class CInterfaceMPScreenBase : public CInterfaceScreenBase, public CMPUIMessageTranslator
{
private:
protected:
	CInterfaceMPScreenBase( const std::string &szInterfaceType, const std::string &szBindSection );

	bool StepLocal( bool bAppActive );

public:
};
