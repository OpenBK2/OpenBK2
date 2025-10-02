#pragma once

#include "Tree_GDBBrowserBase.h"

class CTreeGDBLinkBrowser : public CTreeGDBBrowserBase
{
	static const char TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE[];
public:
	CTreeGDBLinkBrowser( bool _bNeedTranslateAccelerators, bool _bModal, int _nGDBBrowserID ) : CTreeGDBBrowserBase( _bNeedTranslateAccelerators, _bModal, _nGDBBrowserID ) {}
	//CTreeGDBBrowserBase
	void Load();
	bool CanLoad();
	bool CanAutoLoadAfterBuildingObject() { return false; }
	bool GetLoadContextMenuLabel( string *pszLabel );
	bool GetSaveHeaderWidthLabel( string *pszLabel );
	void LoadHeaderWidth();
	void SaveHeaderWidth();
};
