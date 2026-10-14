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
	OBJECT_BASIC_METHODS( CWindowTabControl );
	int nActiveTab;
	int nTabNumber;

	CDBPtr<NDb::SWindowTabControlShared> pShared;
	CPtr<NDb::SWindowTabControl> pInstance;
	std::vector<std::string> tabNames;

	CObj<CWindowScrollableContainer> pHeadersContainer; // tab headers container
																											//	CObj<CWindow> pContainerSample;
																											//	CObj<CWindowMSButton> pButtonSample;

	void ShowTab( const int nTab, const bool bShow, const bool bUpdateButtons );
	const std::string &CreateTabName( const int nTab, CWindow *pTab );
	void SetActive( const int nTab, const bool bUpdateButtons );
	void AddTab( const std::wstring &szButtonName, CWindow *pTab );

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	CWindowTabControl()
		: nActiveTab( -1 )
		, nTabNumber( 0 )
	{
	}

	void Reposition( const CTRect<float> &rcParent ) override;
	void Init() override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void AfterLoad() override;

	int operator&( IBinSaver &saver ) override;

	//IButtonNotify{
	void Pushed( class CWindow *pWho ) override {}

	void Entered( class CWindow *pWho ) override {}

	void Leaved( class CWindow *pWho ) override {}

	void Released( class CWindow *pWho ) override {}

	void StateChanged( class CWindow *pWho ) override;
	//IButtonNotify}

	//ITabControl{
	int GetNTabs() const override;

	void SetActive( const int nTab ) override
	{
		SetActive( nTab, true );
	}

	int GetActive() const override
	{
		return nActiveTab;
	}

	void AddTab( const std::wstring &szButtonName ) override;
	void AddElement( const int nTab, IWindow *pWnd ) override;
	//ITabControl}
};
