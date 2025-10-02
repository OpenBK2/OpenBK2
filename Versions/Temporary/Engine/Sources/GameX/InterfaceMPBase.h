
#pragma once

#include "InterfaceScreenBase.h"
#include "MultiplayerCommandProcessor.h"
#include "MultiplayerCommandManager.h"

class CInterfaceMPScreenBase : public CInterfaceScreenBase, public CMPUIMessageTranslator
{
private:
protected:
	CInterfaceMPScreenBase( const string &szInterfaceType, const string &szBindSection );

	bool StepLocal( bool bAppActive );
public:
};


