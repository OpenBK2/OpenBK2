#if !defined(__ELK_TREE_DOCK_WINDOW__)
#define __ELK_TREE_DOCK_WINDOW__

#define IDC_EMBEDDED_CONTROL 200


class CTreeDockWindow : public SECControlBar
{
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

	CWnd *pwndMainFrame;
public:
	SECTreeCtrl wndTree;

	CTreeDockWindow();
	virtual ~CTreeDockWindow();

	void SetMainFrameWindow( CWnd *_pwndMainFrame );
};

#endif // !defined(__ELK_TREE_DOCK_WINDOW__)
