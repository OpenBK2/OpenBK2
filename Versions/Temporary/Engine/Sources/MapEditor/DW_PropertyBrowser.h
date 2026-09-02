#pragma once

#include "PC_Dialog.h"


class CDWPropertyBrowser : public SECControlBar
{
	CPCDialog wndContents;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	CDWPropertyBrowser();
	virtual ~CDWPropertyBrowser();

	void SetPCDialogXMLOptionsLabel( const std::string &rszOptionsLabel ) { wndContents.SetXMLOptionsLabel( rszOptionsLabel ); }
	void EnableEdit( bool bEnable ) { wndContents.EnableEdit( bEnable ); }
	DECLARE_MESSAGE_MAP()
};


