#if !defined(__DOCKING_WINDOW__PROPERTY_BROWSER__)
#define __DOCKING_WINDOW__PROPERTY_BROWSER__

#include "PC_Dialog.h"


class CDWPropertyBrowser : public SECControlBar
{
	CPCDialog wndContents;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( UINT nType, int cx, int cy );

public:
	CDWPropertyBrowser();
	virtual ~CDWPropertyBrowser();

	void SetPCDialogXMLOptionsLabel( const string &rszOptionsLabel ) { wndContents.SetXMLOptionsLabel( rszOptionsLabel ); }
	void EnableEdit( bool bEnable ) { wndContents.EnableEdit( bEnable ); }
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__DOCKING_WINDOW__PROPERTY_BROWSER__)
