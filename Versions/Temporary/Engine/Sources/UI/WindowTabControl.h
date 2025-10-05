#pragma once
#include "Window.h"



//CWindowTabControl


class CWindowMSButton;
class CWindowScrollableContainer;
struct SWindowScrollableContainerBase;
struct SWindowMSButton;

// contain 0 or more windows. switches between them by button or by function call
class CWindowTabControl : public CWindow, public ITabControl, public IButtonNotify
{
	OBJECT_BASIC_METHODS(CWindowTabControl);
	int nActiveTab;
	int nTabNumber;

	CDBPtr<NDb::SWindowTabControlShared> pShared;
	CPtr<NDb::SWindowTabControl> pInstance;
	std::vector<std::string> tabNames;

	CObj<CWindowScrollableContainer> pHeadersContainer;			// tab headers container
//	CObj<CWindow> pContainerSample;
//	CObj<CWindowMSButton> pButtonSample;

	void ShowTab( const int nTab, const bool bShow, const bool bUpdateButtons );
	const std::string &CreateTabName( const int nTab, CWindow *pTab );
	void SetActive( const int nTab, const bool bUpdateButtons );
	void AddTab( const std::wstring &szButtonName, CWindow *pTab );
protected:
	NDb::SWindow* GetInstance() { return pInstance; }

public:

	CWindowTabControl() : nActiveTab( -1 ), nTabNumber( 0 ) {  }
	void Reposition( const CTRect<float> &rcParent );
	void Init();
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	void AfterLoad();

	int operator&( IBinSaver &saver );
	//IButtonNotify{
	void Pushed( class CWindow *pWho ) {  }
	void Entered( class CWindow *pWho ) {  }
	void Leaved( class CWindow *pWho ) {  }
	void Released( class CWindow *pWho ) {  }
	void StateChanged( class CWindow *pWho );
	//IButtonNotify}

	//ITabControl{
	int GetNTabs() const;
	void SetActive( const int nTab ) { SetActive( nTab, true ); }
	int GetActive() const { return nActiveTab; }
	void AddTab( const std::wstring &szButtonName );
	void AddElement( const int nTab, IWindow *pWnd );
	//ITabControl}


};

