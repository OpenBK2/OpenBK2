
#pragma once
#include "InterfaceScreenBase.h"
#include "DBGameOptions.h"

class CInterfaceOptionsMenu : public CInterfaceScreenBase, public ISliderNotify
{
	OBJECT_NOCOPY_METHODS( CInterfaceOptionsMenu );
public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS(CReactions);
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceOptionsMenu> pInterface;
	public:
		CReactions() {  }
		~CReactions() 
		{  
		}
		CReactions( IWindow *_pScreen, CInterfaceOptionsMenu *_pInterface ) 
			: pScreen( _pScreen ), pInterface( _pInterface ) {   }
		virtual bool Execute( const string &szSender, const string &szReaction );
		virtual int Check( const string &szCheckName ) const;
	};
	//
	void ChangeResolution();
	//
	~CInterfaceOptionsMenu();
public:
	CInterfaceOptionsMenu();

	bool Init();

	void OnGetFocus( bool bFocus );

	void SetMode( const string &szMode );

	void FillScreen();

	void SliderPosition( const float fPosition, CWindow *pWho );

	bool ProcessEvent( const SGameMessage &msg );
private:
	enum EOptionModes
	{
		OPTION_INTERMISSION = 1<<0,
		OPTION_MISSION			= 1<<1,
		OPTION_MULTIPLAYER	= 1<<2,
		OPTION_MPFILTER			= 1<<3
	};
	enum EScreenModes
	{
		MODE_NORMAL,
		MODE_FILTERS
	};
	// Associating options with controls and saved values
	struct SOptionInstance
	{
		int							nCategoryEntry;
		int							nOptionEntry;
		CPtr< IWindow > pWindow;
		float						fSliderPosition;
		string					szProgName;
		string					szSavedValue;
		string					szCurrentValue;
	};
	typedef vector< SOptionInstance > CInstances;
	struct SCategoryInstance 
	{
		int							nCategoryEntry;
		CPtr< IWindow > pWindow;
	};
	typedef vector< SCategoryInstance > CCategories;

	ZDATA_(CInterfaceScreenBase)
	CObj<CReactions>	pReactions;

	CDBPtr< NDb::SOptionSystem > pOptionSystem;
	CCategories				categories;
	CInstances				options;
	int								nSelectedCategory;
	CPtr< IWindow >		pMain;
	CPtr< IScrollableContainer >		pOptionListTemplate;
	CPtr< ITextView >		pCategoryTitle;

	//Templates for generated controls
	CPtr< IButton >		pButtonTemplate;
	CTPoint<int>			categoryOffset;

	CPtr< IWindow >		pCheckBoxItemTemplate;
	CPtr< IWindow >		pEditLineItemTemplate;
	CPtr< IWindow >		pSliderItemTemplate;
	CPtr< IWindow >		pMultichoiceTemplate;
	CPtr< IWindow >		pEditNumberItemTemplate;

	CPtr< IScrollableContainer >		pOptionList;
	EScreenModes			screenMode;			

	CPtr<IWindow> pGroupPanel;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pReactions); f.Add(3,&pOptionSystem); f.Add(4,&categories); f.Add(5,&options); f.Add(6,&nSelectedCategory); f.Add(7,&pMain); f.Add(8,&pOptionListTemplate); f.Add(9,&pCategoryTitle); f.Add(10,&pButtonTemplate); f.Add(11,&categoryOffset); f.Add(12,&pCheckBoxItemTemplate); f.Add(13,&pEditLineItemTemplate); f.Add(14,&pSliderItemTemplate); f.Add(15,&pMultichoiceTemplate); f.Add(16,&pEditNumberItemTemplate); f.Add(17,&pOptionList); f.Add(18,&screenMode); f.Add(19,&pGroupPanel); return 0; }
protected:
	void MsgBack( const SGameMessage &msg );
	void MsgAccept( const SGameMessage &msg );
	void SelectCategory( int nCategory, bool bForceRecreate );
	void OnSelectCategory( const string &szSender );
	void OnControlChange( const string &szSender );
	void CommitEditLineChanges();					//Collect and store EditLine values
	void RollbackChanges();
	void RestoreDefaultValues();

public:
	bool StepLocal( bool bAppActive );
};

class CICOptionsMenu : public CInterfaceCommandBase<CInterfaceOptionsMenu>
{
	OBJECT_BASIC_METHODS( CICOptionsMenu );
	//
	ZDATA_(CInterfaceCommandBase<CInterfaceOptionsMenu>)
	string szMode;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceOptionsMenu>*)this); f.Add(2,&szMode); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};



