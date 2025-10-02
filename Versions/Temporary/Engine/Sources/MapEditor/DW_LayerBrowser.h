#if !defined(__DOCKING_WINDOW__LAYER_BROWSER__)
#define __DOCKING_WINDOW__LAYER_BROWSER__


class CDWLayerBrowser : public SECControlBar
{
	//CWnd wndContents;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( UINT nType, int cx, int cy );

public:
	CDWLayerBrowser();
	virtual ~CDWLayerBrowser();

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__DOCKING_WINDOW__LAYER_BROWSER__)
