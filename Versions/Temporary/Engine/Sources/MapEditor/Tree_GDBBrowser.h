#if !defined(__TREE__GDB_BROWSER__)
#define __TREE__GDB_BROWSER__

#include "Tree_GDBBrowserBase.h"

class CTreeGDBBrowser : public CTreeGDBBrowserBase
{
	static const char TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE[];
public:
	CTreeGDBBrowser( bool _bNeedTranslateAccelerators, bool _bModal, int _nGDBBrowserID ) : CTreeGDBBrowserBase( _bNeedTranslateAccelerators, _bModal, _nGDBBrowserID ) {}
	//CTreeGDBBrowserBase
	void Load();
	bool CanLoad();
	bool CanAutoLoadAfterBuildingObject() { return true; }
	bool GetLoadContextMenuLabel( string *pszLabel );
	bool GetSaveHeaderWidthLabel( string *pszLabel );
	void LoadHeaderWidth();
	void SaveHeaderWidth();
};

#endif // !defined(__TREE__GDB_BROWSER__)
