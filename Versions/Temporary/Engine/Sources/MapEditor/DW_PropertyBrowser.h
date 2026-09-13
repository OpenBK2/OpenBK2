#pragma once

// PC_Dialog.h is not needed here any more. It stays because what includes this
// header -- MainFrame.h, and through it much of the module -- has always had the
// property control's declarations from it.
#include "PC_Dialog.h"
#include "PropertyPaneView.h"

#include <string>


class CDWPropertyBrowser : public SECControlBar
{
	// Owned. Which implementation it is comes from NPropertyPane::Create, as the
	// Log Window's contents do; the pane never learns which it got.
	IPropertyPane *pPane;
	std::string szOptionsLabel;
	bool bEnableEdit;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	CDWPropertyBrowser();
	virtual ~CDWPropertyBrowser();

	// Before Create: the contents are made in OnCreate and take it then.
	void SetPCDialogXMLOptionsLabel( const std::string &rszOptionsLabel ) { szOptionsLabel = rszOptionsLabel; }
	void EnableEdit( bool bEnable );
	DECLARE_MESSAGE_MAP()
};
