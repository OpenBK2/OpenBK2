#ifndef __INTERFACE_MP_BASE_H__
#define __INTERFACE_MP_BASE_H__

#pragma ONCE

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

#endif //__INTERFACE_MP_BASE_H__
