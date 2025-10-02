#if !defined(__ELK_TREE_WINDOW__)
#define __ELK_TREE_WINDOW__

#include "resource.h"
#include "TreeDockWindow.h"
#include "ELK_Types.h"


class CELKTreeWindow : public CTreeDockWindow
{
	friend class CELK;
protected:
	enum IMAGES
	{
		IMAGE_ROOT_NOT_TRANSLATED								= 0,
		IMAGE_ROOT_OUTDATED											= 1,
		IMAGE_ROOT_TRANSLATED										= 2,
		IMAGE_ROOT_APPROVED											= 3,
		IMAGE_ROOT_NOT_TRANSLATED_EXPANDED			= 4,
		IMAGE_ROOT_OUTDATED_EXPANDED						= 5,
		IMAGE_ROOT_TRANSLATED_EXPANDED					= 6,
		IMAGE_ROOT_APPROVED_EXPANDED						= 7,

		IMAGE_FOLDER_NOT_TRANSLATED							= 8,
		IMAGE_FOLDER_OUTDATED										= 9,
		IMAGE_FOLDER_TRANSLATED									= 10,
		IMAGE_FOLDER_APPROVED										= 11,
		IMAGE_FOLDER_NOT_TRANSLATED_EXPANDED		= 12,
		IMAGE_FOLDER_OUTDATED_EXPANDED					= 13,
		IMAGE_FOLDER_TRANSLATED_EXPANDED				= 14,
		IMAGE_FOLDER_APPROVED_EXPANDED					= 15,

		IMAGE_TEXT_NOT_TRANSLATED								= 16,
		IMAGE_TEXT_OUTDATED											= 17,
		IMAGE_TEXT_TRANSLATED										= 18,
		IMAGE_TEXT_APPROVED											= 19,
		IMAGE_TEXT_NOT_TRANSLATED_EXPANDED			= 20,
		IMAGE_TEXT_OUTDATED_EXPANDED						= 21,
		IMAGE_TEXT_TRANSLATED_EXPANDED					= 22,
		IMAGE_TEXT_APPROVED_EXPANDED						= 23,

		IMAGE_COUNT															= 24,
	};
	CImageList imageListNormal;
	bool bCollapseDeselected;
	CWnd* pwndFormWindow;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
	afx_msg void OnBtState0();
	afx_msg void OnBtState1();
	afx_msg void OnBtState2();
	afx_msg void OnBtState3();
	afx_msg void OnViewTreeStatistic();
	DECLARE_MESSAGE_MAP()

	bool bNextFilterChanged;
	bool bPreviousFilterChanged;
	HTREEITEM cachedNextItem;
	HTREEITEM cachedPreviousItem;
	bool bCachedNextItemExists;
	bool bCachedPreviousItemExists;
		
	int FillFolder( HTREEITEM parentItem, const string &rszFolderName, const string &rszInitialItemPath,  int nInitialELKElement, class CProgressDialog *pwndProgressDialog );
	int UpdateFolderState( HTREEITEM item );

	void SelectItem( HTREEITEM item );
	void DeselectItem( HTREEITEM item );

	HTREEITEM GetNextItemInternal( HTREEITEM item );
	HTREEITEM GetPreviousItemInternal( HTREEITEM item );
	HTREEITEM GetFirstItemInternal();
	HTREEITEM GetLastItemInternal();

	hash_map<LONG, string> rootFolderMap;
	hash_map<LONG, string> rootNameMap;
	hash_map<LONG, int> rootNumberMap;

	int GetItemsCountInternal();
	void GetItemPathInternal( HTREEITEM item, string *pszItemPath, bool bFull = true );
	void GetXLSPathInternal( HTREEITEM item, string *pszItemPath );
	bool IsItemParent( HTREEITEM item, HTREEITEM parentItem );
	void GetELKElementPathInternal( HTREEITEM item, string *pszPath );
	void GetELKElementNameInternal( HTREEITEM item, string *pszName );
	int GetELKElementNumberInternal( HTREEITEM item );

public:
	CELKTreeWindow();
	virtual ~CELKTreeWindow();

	void InitImageList();
	void ClearTree();
	void FillTree( const class CELK &rELK, const string &rszInitialItemPath, int nInitialELKElement, class CProgressDialog *pwndProgressDialog = 0 );
	void GetItemPath( string *pszItemPath, bool bFull = true );
	int GetELKElementNumber();

	bool GetNextItem( const struct SSimpleFilter *pELKFilter );
	bool GetPreviousItem( const struct SSimpleFilter *pELKFilter );
	bool GetFirstItem( const struct SSimpleFilter *pELKFilter );
	bool GetLastItem( const struct SSimpleFilter *pELKFilter );
	bool IsNextItemExists( const struct SSimpleFilter *pELKFilter );
	bool IsPreviousItemExists( const struct SSimpleFilter *pELKFilter );
	int GetItemsCount( const struct SSimpleFilter *pELKFilter );

	bool FindItem( const class CELK &rELK, struct SMainFrameParams::SSearchParam *pSearchParam, int nCodePage );
	void ViewTreeStatistic();
	void SetCollapseItem( bool bCollapseItem );
	void SetFormWindow( CWnd* _pwndFormWindow ) { pwndFormWindow = _pwndFormWindow; }
	void SetFilterChanged( bool _bFilterChanged = true )  { bNextFilterChanged = _bFilterChanged; bPreviousFilterChanged = _bFilterChanged; }
	void UpdateSelectedText( CELK *pELK, int nState );
	void UpdateSelectedFolder( CELK *pELK, int nState );
};

#endif // !defined(__ELK_TREE_WINDOW__)
