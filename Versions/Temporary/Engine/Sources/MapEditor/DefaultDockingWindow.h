#if !defined(__DEFAULT_DOCKING_WINDOW__)
#define __DEFAULT_DOCKING_WINDOW__

class CDefaultDockingWindow : public SECControlBar
{
	friend class CMainFrame;
	CWnd* pwndContents;

protected:
	afx_msg void OnSize( UINT nType, int cx, int cy );

public:
	CDefaultDockingWindow() : pwndContents( 0 ) {}
	virtual ~CDefaultDockingWindow() {}

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__DEFAULT_DOCKING_WINDOW__)
