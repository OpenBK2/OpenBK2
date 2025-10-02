#ifndef __UI_RUN_MODE_INTERFACE__
#define __UI_RUN_MODE_INTERFACE__

#pragma once

#include "../ED_Common/RunModeInterfaceBase.h"
interface IWindow;

class CUIRunModeInterface : public CRunModeInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CUIRunModeInterface );

public:	
	// lifecycle
	CUIRunModeInterface() {}
	CUIRunModeInterface( const string &rszTypeName, const CDBID &rDBID, const CDBID &rTemplateWindowDBID, const CDBID &rTemplateScreenDBID );
	virtual ~CUIRunModeInterface();
	
	// IInterfaceBase
	bool ProcessEvent( const struct SGameMessage &msg );
	void Step( bool bAppActive );

	// methods
protected:
	void LoadWindow( const string &rszTypeName, const CDBID &rDBID, const CDBID &rTemplateWindowDBID, const CDBID &rTemplateScreenDBID );

	// members
protected:
	CObj<IWindow> pScreen;
};

INTERFACE_COMMAND_DECLARE( CUIRunModeIC, CUIRunModeInterface )

#endif // __UI_RUN_MODE_INTERFACE__

