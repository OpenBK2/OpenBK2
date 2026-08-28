#include "stdafx.h"
#include "UI/UI.h"
#include "InterfaceOptionsMenu.h"
#include "GameXClassIDs.h"
#include "DBGameRoot.h"
#include "UI/Window.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "SceneB2/Cursor.h"
#include "Misc/StrProc.h"
#include "Main/MainLoopCommands.h"
#include "InterfaceState.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "System/Commands.h"

#include "3Dmotor/GAutoDetect.h"
#include "3Dmotor/GfxBenchmark.h"


const int ITEM_DELTA_X = 3;
const int ITEM_DELTA_Y = 3;

int FindStateByValue( const std::string &szValue, const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &entry )
{
	for ( int i = 0; i < entry.states.size(); ++i )
	{
		if ( entry.states[i].szValue == szValue )
			return i;
	}

	// Find nearest float value
	float fValue = -FP_MAX_VALUE;
	fValue = NStr::ToFloat( szValue );
	if ( fValue == -FP_MAX_VALUE )
		return -1;

	int nNearest = -1;
	float fMinDifference = FP_MAX_VALUE;

	for ( int i = 0; i < entry.states.size(); ++i )
	{
		float fOptionValue = FP_MIN_VALUE;
		fOptionValue = NStr::ToFloat( entry.states[i].szValue );
		if ( fOptionValue == FP_MIN_VALUE )
			return -1;

		if ( fabs(fOptionValue - fValue) < fMinDifference )
		{
			fMinDifference = fabs( fOptionValue - fValue );
			nNearest = i;
		}
	}

	return nNearest;
}

// CInterfaceOptionsMenu::CReactions

bool CInterfaceOptionsMenu::CReactions::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_category_select" )
	{
		pInterface->OnSelectCategory( szSender );
		return true;
	}
	if ( szReaction == "react_on_restore_defaults" )
	{
		pInterface->RestoreDefaultValues( );
		return true;
	}
	if ( szReaction == "react_on_option_change" )
	{
		pInterface->OnControlChange( szSender );
		return true;
	}
	return false;
}

int CInterfaceOptionsMenu::CReactions::Check( const std::string &szCheckName ) const
{
	return 0;
}

// CInterfaceOptionsMenu

CInterfaceOptionsMenu::CInterfaceOptionsMenu() : 
	CInterfaceScreenBase( "OptionsMenu", "options_menu" )
{
	AddObserver( "back", &CInterfaceOptionsMenu::MsgBack );
	AddObserver( "accept", &CInterfaceOptionsMenu::MsgAccept );
}

CInterfaceOptionsMenu::~CInterfaceOptionsMenu()
{
}

bool CInterfaceOptionsMenu::Init()
{
	screenMode = MODE_NORMAL;					//Redundant?

	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	pReactions = new CReactions( pScreen, this );
	AddScreen( pReactions );
	
	return true;
}

void CInterfaceOptionsMenu::FillScreen()
{
	int nButtonX, nButtonY, nButtonSX, nButtonSY;
	CPtr< IWindow >	pButtonOffset;
	DWORD nOptionMask = 0xff;			

	IWindow *pMPFilters = GetChildChecked<IWindow>( pScreen, "MPFilters", true );
	if ( pMPFilters )
		pMPFilters->ShowWindow( false );
	IWindow *pOptionsMenu = GetChildChecked<IWindow>( pScreen, "OptionsMenu", true );
	if ( pOptionsMenu )
		pOptionsMenu->ShowWindow( false );
	
	//Get the appropriate screen
	if ( screenMode == MODE_FILTERS ) 
	{
		nOptionMask = 0x08;				//Filters only
		pMain = pMPFilters;
		pMain->ShowWindow( true );
	}
	else
	{
		nOptionMask = 0x07;				//All but filters
		pMain = pOptionsMenu;
		pMain->ShowWindow( true );
	}

	if ( !IsValid( pMain ) )
	{
		NMainLoop::Command( CreateICCloseInterface() );
		return;
	}

	//Get elements
	pOptionListTemplate = GetChildChecked<IScrollableContainer>( pMain, "OptionList", true );
	pOptionListTemplate->ShowWindow( false );
	pOptionList = 0;
	pButtonTemplate = GetChildChecked<IButton>( pMain, "CategoryButton", true );
	if ( pButtonTemplate )
		pButtonTemplate->ShowWindow( false );
	pButtonOffset = GetChildChecked<IButton>( pMain, "CategoryButtonOffset", true );
	pGroupPanel = GetChildChecked<IWindow>( pMain, "GroupPanel", true );
	if ( pButtonOffset )
		pButtonOffset->ShowWindow( false );
	//Get category button step
	pButtonOffset->GetPlacement( &nButtonX, &nButtonY, &nButtonSX, &nButtonSY );
	categoryOffset.x = nButtonX;
	categoryOffset.y = nButtonY;
	pButtonTemplate->GetPlacement( &nButtonX, &nButtonY, &nButtonSX, &nButtonSY );
	categoryOffset.x -= nButtonX;
	categoryOffset.y -= nButtonY;
  
	pCheckBoxItemTemplate = GetChildChecked<IWindow>( pMain, "CheckBoxTemplate", true );
	pEditLineItemTemplate = GetChildChecked<IWindow>( pMain, "EditLineTemplate", true );
	pSliderItemTemplate = GetChildChecked<IWindow>( pMain, "SliderTemplate", true );
	pMultichoiceTemplate = GetChildChecked<IWindow>( pMain, "MultichoiceTemplate", true );
	pEditNumberItemTemplate = GetChildChecked<IWindow>( pMain, "EditNumberTemplate", true );
	
	//Fill categories, create category buttons
	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	pOptionSystem = pGameRoot->pGameOptions;
	int nScreenPos = 0;
	for ( int i = 0; i < pOptionSystem->categories.size(); ++i)
	{
		//If filtering, need to have different counters
		SCategoryInstance category;
		category.nCategoryEntry = i;

		//Filter the empty categories out? 
		int nOptionsInCategory = 0;
		//Fill in the options
		for ( int j = 0 ; j <  pOptionSystem->categories[i].options.size(); ++j )
		{
			//Filter according to mode
			if ( !( pOptionSystem->categories[i].options[j].nModeFlags & nOptionMask ) )
				continue;

			SOptionInstance option;
			option.nCategoryEntry = i;
			option.nOptionEntry = j;
				option.szProgName = pOptionSystem->categories[i].options[j].szProgName;
			option.pWindow = 0;
			option.szSavedValue = NStr::ToMBCS( NGlobal::GetVar( option.szProgName, pOptionSystem->categories[i].options[j].szDefaultValue ) );
			option.szCurrentValue = option.szSavedValue;
			if ( pOptionSystem->categories[i].options[j].eEditorType ==
			     NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER) 
			{
				option.fSliderPosition = NStr::ToFloat( option.szSavedValue );
			}
			options.push_back(option);
			++nOptionsInCategory;
		}
		if ( nOptionsInCategory ) 
		{		//Add non-empty categories
			IWindow *pElement = AddWindowCopy( pGroupPanel, pButtonTemplate );
			pElement->SetName( StrFmt( "Category%d", i ) );
			pElement->ShowWindow( true );
			pElement->SetTextString( pElement->GetDBText() + GET_TEXT_PRE(pOptionSystem->categories[i].,Name) );
			pElement->SetPlacement( nButtonX + i * categoryOffset.x, nButtonY + nScreenPos * categoryOffset.y, 
				0, 0, EWPF_POS_X | EWPF_POS_Y );
			pElement->SetTooltip( GET_TEXT_PRE(pOptionSystem->categories[i].,Tooltip) );

			category.pWindow = pElement;
			categories.push_back(category);
			++nScreenPos;
		}
	}
	if ( categories.size() )
	{
		nSelectedCategory = 0;
		SelectCategory( nSelectedCategory, false );

		if ( categories.size() == 1 )				//No need to show only one button (feature used in filters)
		{
			//categories[0].pWindow ->ShowWindow( false );
		}
	} 
	else 
	{
		nSelectedCategory = -1;
	}
	
	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );
}

bool CInterfaceOptionsMenu::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// check for real resolution and 'gfx_resolution'
void CInterfaceOptionsMenu::ChangeResolution()
{
	CVec2 vScreenSize = Scene()->GetScreenRect();
	const std::wstring wszResolution = NStr::ToUnicode( StrFmt( "%dx%d", int(vScreenSize.x), int(vScreenSize.y) ) );

	bool bGFXOptionsChanged = false;
	for ( int i = 0; i < options.size(); ++i )
	{
		SOptionInstance *pOption = &( options[i] );
		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &entry = 
			pOptionSystem->categories[ categories[ pOption->nCategoryEntry ].nCategoryEntry ].options[ pOption->nOptionEntry ];

		if ( options[i].szProgName.find( "gfx_" ) != -1 && 
			options[i].szSavedValue != options[i].szCurrentValue )
		{
			bGFXOptionsChanged = true;
			break;
		}
	}

	for ( int i = 0; i < options.size(); ++i )
	{
		if ( options[i].szProgName == "gfx_quality" ) 
		{
			SOptionInstance *pOption = &( options[i] );
			const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &entry = 
				pOptionSystem->categories[ categories[ pOption->nCategoryEntry ].nCategoryEntry ].options[ pOption->nOptionEntry ];

			if ( entry.eEditorType == NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER )
			{
				NGlobal::ProcessCommand( std::wstring( L"set_quality " ) + NStr::ToUnicode( StrFmt( "%f", options[i].fSliderPosition ) ) );
				options[i].fSliderPosition = NGlobal::GetVar( "gfx_quality" );
			}
			else
			{
				NGlobal::ProcessCommand( std::wstring( L"set_quality " ) + NStr::ToUnicode( options[i].szCurrentValue ) );
				options[i].szCurrentValue = NStr::ToMBCS( NGlobal::GetVar( "gfx_quality" ).GetString() );
			}
			break;
		}
	}


	if ( wszResolution != NGlobal::GetVar("gfx_resolution").GetString() || bGFXOptionsChanged )
	{
		NGlobal::ProcessCommand( L"gfx_recreate" );
		CVec2 vNewScreenSize = Scene()->GetScreenRect();
		Cursor()->SetBounds( 0, 0, vNewScreenSize.x, vNewScreenSize.y );
	}
}

void CInterfaceOptionsMenu::SelectCategory( int nCategory, bool bForceRecreate )
{
	std::wstring wszUnicode;
	int nItemX, nItemY, nItemSX, nItemSY;
	int nWidthAll, nWidthLeft, nWidthRight;

	// Adjust button states
	bool bNoChanges = false;
	nSelectedCategory = nCategory;
	for( int i = 0; i < categories.size(); ++i )
	{
		IButton *pButton = dynamic_cast_ptr<IButton*>( categories[i].pWindow );
		if ( i == nCategory )
		{
			if ( pButton->GetState() == 1 )
				bNoChanges = true;
			pButton->SetState( 1 );
		} 
		else 
		{
			pButton->SetState( 0 );
		}
	}
	if ( nCategory == -1 || bNoChanges && !bForceRecreate )
		return;


	//Create an empty container
	if ( pOptionList )
	{
		pMain->RemoveChild( pOptionList );
		pOptionList = 0;
	}

	pOptionList = dynamic_cast<IScrollableContainer*>( AddWindowCopy( pMain, pOptionListTemplate ) );
	IWindow *pBorder = GetChildChecked<IWindow>( pOptionList, "ListBorder", true );
	pBorder->GetPlacement( &nItemX, &nItemY, &nItemSX, &nItemSY );
	nWidthAll = nItemSX;
	nWidthLeft = nWidthAll / 2 - ITEM_DELTA_X;
	nWidthRight = nWidthAll - nWidthLeft - 3 * ITEM_DELTA_X;

	//Pointer to current category
  const NDb::SOptionSystem::SOptionsCategory *pCat = 
		&( pOptionSystem->categories[ categories[ nCategory ].nCategoryEntry ] );

	pCategoryTitle = GetChildChecked<ITextView>( pOptionList, "CategoryTitle", true );
	if ( pCategoryTitle )
	{
		pCategoryTitle->SetText( pCategoryTitle->GetDBText() + GET_TEXT_PRE(pCat->,Name) );
		pCategoryTitle->SetTooltip( GET_TEXT_PRE(pCat->,Tooltip) );
	}

	for ( int i = 0; i < options.size(); ++i )			//Cycle for all options: remove windows
		options[i].pWindow = 0;

	// Fill in the options
	for ( int i = 0; i < options.size(); ++i )			//Cycle for all options in the current category
	{
		if ( options[i].nCategoryEntry != categories[ nCategory ].nCategoryEntry ) 
			continue;
		SOptionInstance *pOption = &( options[i] );
		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &pEntry = pCat->options[pOption->nOptionEntry];

		IWindow *pElement = 0;
		// Create a corresponding control
		switch ( pEntry.eEditorType ) 
		{
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_CHECKBOX:
				pElement = AddWindowCopy( pOptionList, pCheckBoxItemTemplate );
				break;
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE:
				pElement = AddWindowCopy( pOptionList, pEditLineItemTemplate );
				break;
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER:
				pElement = AddWindowCopy( pOptionList, pEditNumberItemTemplate );
				break;
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER:
				pElement = AddWindowCopy( pOptionList, pSliderItemTemplate );
				break;
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST:
				pElement = AddWindowCopy( pOptionList, pMultichoiceTemplate );
				break;
			default:
				NI_ASSERT( 0, "Unknown Option editor type");
		}
		NI_ASSERT( pElement, "Could not create Option Item entry" );
		pElement->GetPlacement( &nItemX, &nItemY, &nItemSX, &nItemSY );
		pElement->SetPlacement( 0, i * ( nItemSY + ITEM_DELTA_Y ), nWidthAll, 0, 
			EWPF_POS_X | EWPF_POS_Y | EWPF_SIZE_X );
		pElement->SetName( StrFmt("Option%d", i ) );
		pElement->SetTooltip( GET_TEXT_PRE(pEntry.,Tooltip) );

		ITextView *pOptionName = GetChildChecked<ITextView>( pElement, "OptionName", true );
		pOptionName->SetText( pOptionName->GetDBText() + GET_TEXT_PRE(pEntry.,Name) );
		pOptionName->SetTooltip( GET_TEXT_PRE(pEntry.,Tooltip) );
		pOptionName->SetPlacement( ITEM_DELTA_X, ITEM_DELTA_Y, nWidthLeft, 0, 
			EWPF_POS_X | EWPF_POS_Y | EWPF_SIZE_X );

		IWindow *pOptionControl = GetChildChecked<IWindow>( pElement, "OptionControl", true );
		pOptionControl->SetName( StrFmt( "Control%d", i ) );
		pOptionControl->SetTooltip( GET_TEXT_PRE(pEntry.,Tooltip) );
		DWORD dwFlags = EWPF_POS_X | EWPF_POS_Y;
		switch ( pEntry.eEditorType )
		{
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE:
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER:
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER:
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST:
			{
				dwFlags |= EWPF_SIZE_X;
				break;
			}
		}
		pOptionControl->SetPlacement( nWidthLeft + ITEM_DELTA_X + ITEM_DELTA_X, ITEM_DELTA_Y, nWidthRight, 0, dwFlags );
		//
		IButton *pButtonControl = 0;
		ISlider *pSliderControl = 0;
		IEditLine *pEditLineControl = 0;
		//Fill in control-values
		pOption->pWindow = pOptionControl;
		switch ( pEntry.eEditorType ) 
		{
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE:
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER:
				pEditLineControl = dynamic_cast<IEditLine*> ( pOptionControl );
				NI_ASSERT( pEditLineControl, "EditLine dynamic cast failed" );
				wszUnicode.clear();
				NStr::ToUnicode( &wszUnicode, pOption->szCurrentValue );
				pEditLineControl->SetText( wszUnicode.data() );
				break;

			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_CHECKBOX:
				pButtonControl = dynamic_cast<IButton*> ( pOptionControl );
				NI_ASSERT( pButtonControl, "Button dynamic cast failed" );
				if ( pOption->szCurrentValue == "1" )
					pButtonControl->SetStateWithVisual( 1 );
				else
					pButtonControl->SetStateWithVisual( 0 );
				break;

			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER:
				{
					pSliderControl = dynamic_cast<ISlider*> ( pOptionControl );
					NI_ASSERT( pSliderControl, "Slider dynamic cast failed" );
					const int nMax = pSliderControl->GetNSpecialPositions();
					pSliderControl->SetRange( 0, nMax - 1, 1 );
					pSliderControl->SetPos( pOption->fSliderPosition * nMax );
					pSliderControl->SetNotifySink( this );
				}
				break;
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST:
				pButtonControl = dynamic_cast<IButton*> ( pOptionControl );
				NI_ASSERT( pButtonControl, "MultiButton dynamic cast failed" );
				{
					if ( pOption->szProgName == "gfx_resolution" )
					{
						const NGlobal::CValue resolutionValue = NGlobal::GetVar( "gfx_resolution", "1024x768" );
						const std::string szCurrentMode = NStr::ToMBCS( resolutionValue.GetString() );
						pOptionControl->SetTextString( NStr::ToUnicode( szCurrentMode ) );
						pOptionControl->SetTooltip( GET_TEXT_PRE(pEntry.states[0].,Tooltip) );
						NGlobal::SetVar( "gfx_resolution", szCurrentMode );
					}
					else
					{
						const int nPos = FindStateByValue( pOption->szCurrentValue, pEntry );
						if ( nPos != -1 )
						{
							if ( CHECK_TEXT_NOT_EMPTY_PRE(pEntry.states[nPos].,Name) ) 
								pOptionControl->SetTextString( GET_TEXT_PRE(pEntry.states[nPos].,Name) );
							pOptionControl->SetTooltip( GET_TEXT_PRE(pEntry.states[nPos].,Tooltip) );
						}
						else
							pOptionControl->SetTextString( L"-" );
					}
				}
				break;
		}
		pOptionList->PushBack( pElement, true );
	}
}

void CInterfaceOptionsMenu::OnSelectCategory( const std::string &szSender )
{
	CommitEditLineChanges();

	for ( int i = 0; i < categories.size(); ++i )
	{
		if ( categories[i].pWindow->GetName() == szSender )
		{
			SelectCategory( i, false );
			return;
		}
	}
	SelectCategory( -1, false );
}

void CInterfaceOptionsMenu::OnControlChange( const std::string &szSender )
{
	IButton					*pButtonControl;
	IWindow					*pWindow;
	IEditLine				*pEditLineControl;
	SOptionInstance *pOption;
	std::string					szMBCS;

	for ( int i = 0; i < options.size(); ++i )			//Cycle for all options in the current category
	{
		if ( options[i].nCategoryEntry != categories[nSelectedCategory].nCategoryEntry )
			continue;
		pOption = &( options[i] );
		if ( pOption->pWindow->GetName() == szSender )
		{
			//Display selection?
			//pList = dynamic_cast<IScrollableContainer*>( pOptionList.GetPtr() );
			//pList->Select( pOption->pWindow->GetParentWindow() );
			//Store it
			const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &entry = 
				pOptionSystem->categories[ categories[ nSelectedCategory ].nCategoryEntry ].options[ pOption->nOptionEntry ];

			switch ( entry.eEditorType ) 
			{
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_CHECKBOX:
				pWindow = pOption->pWindow;
				pButtonControl = dynamic_cast<IButton*>( pWindow );
				if ( pButtonControl->GetState() < entry.states.size() )
				{
					pOption->szCurrentValue = entry.states[pButtonControl->GetState()].szValue;
					NGlobal::SetVar( pOption->szProgName, pOption->szCurrentValue );
				}
				break;

			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE:
			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER:
				pWindow = pOption->pWindow;
				pEditLineControl = dynamic_cast<IEditLine*>( pWindow );
				szMBCS.clear();
				NStr::ToMBCS( &szMBCS, pEditLineControl->GetText() );
				NGlobal::SetVar( pOption->szProgName, szMBCS );
				pOption->szCurrentValue = szMBCS;
				break;

			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST:
				pWindow = pOption->pWindow;
				pButtonControl = dynamic_cast<IButton*>( pWindow );
				if ( pOption->szProgName == "gfx_resolution" )
				{
					std::vector<NGfx::SVideoMode> modeList;
					NGfx::GetModesList( &modeList, 32 );
					std::vector<std::string> resolutions;
					for ( std::vector<NGfx::SVideoMode>::iterator it = modeList.begin(); it != modeList.end(); ++it )
					{
						if ( it->nXSize > 600 && it->nYSize > 450 )
							resolutions.push_back( StrFmt( "%dx%d", it->nXSize, it->nYSize ) );
					}
					const NGlobal::CValue resolutionValue = NGlobal::GetVar( "gfx_resolution", "1024x768" );
					const std::string szCurrentMode = NStr::ToMBCS( resolutionValue.GetString() );
					std::vector<std::string>::iterator nextModeIt = resolutions.end();
					for ( std::vector<std::string>::iterator it = resolutions.begin(); it != resolutions.end(); ++it )
					{
						if ( *it == szCurrentMode )
						{
							nextModeIt = it;
							++nextModeIt;
							break;
						}
					}
					if ( nextModeIt == resolutions.end() )
						nextModeIt = resolutions.begin();
					const std::string szResolution = *nextModeIt;
					pWindow->SetTextString( NStr::ToUnicode( szResolution ) );
					pWindow->SetTooltip( GET_TEXT_PRE(entry.states[0].,Tooltip) );
					NGlobal::SetVar( "gfx_resolution", szResolution );
				}
				else if ( !entry.states.empty() )
				{
					const int nPos = ( FindStateByValue( pOption->szCurrentValue, entry ) + 1 ) % entry.states.size(); // Switch to next state
					if ( CHECK_TEXT_NOT_EMPTY_PRE(entry.states[nPos].,Name) )
						pWindow->SetTextString( GET_TEXT_PRE(entry.states[nPos].,Name) );
					pWindow->SetTooltip( GET_TEXT_PRE(entry.states[nPos].,Tooltip) );
					pOption->szCurrentValue = entry.states[nPos].szValue;
					NGlobal::SetVar( pOption->szProgName, pOption->szCurrentValue );
				}

				break;

			case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER:
				//Not going to happen here, see CInterfaceOptionsMenu::SliderPosition()
			default:
				NI_ASSERT( 0, "Unknown/Invalid Option editor type");
			}

			return;
		}
	}
}

void CInterfaceOptionsMenu::SliderPosition( const float fPosition, CWindow *pWho )
{
	for ( int i = 0 ; i <  options.size(); ++i )			//Cycle for all options in the current category
	{
		if ( options[i].nCategoryEntry != categories[nSelectedCategory].nCategoryEntry )
			continue;

		SOptionInstance *pOption = &( options[i] );
		if ( !pOption->pWindow )
			continue;
		if ( pOption->pWindow->GetName() == pWho->GetName() ) 
		{
			ISlider* pSliderControl = dynamic_cast<ISlider*> ( pOption->pWindow.GetPtr() );
			NI_ASSERT( pSliderControl, "Slider dynamic cast failed" );
			const int nMax = pSliderControl->GetNSpecialPositions();

			pOption->fSliderPosition = fPosition / nMax;

			NGlobal::SetVar( pOption->szProgName, pOption->fSliderPosition );
		}
	}
}

void CInterfaceOptionsMenu::CommitEditLineChanges()
{
	IWindow					*pWindow;
	IEditLine				*pEditLineControl;
	SOptionInstance *pOption;
	std::string					szMBCS;

	if ( nSelectedCategory == -1 )
		return;
	for ( int i = 0; i < options.size(); ++i )			//Cycle for all options in the current category
	{
		if ( options[i].nCategoryEntry != categories[nSelectedCategory].nCategoryEntry )
			continue;
		pOption = &( options[i] );
		//Store it
		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &pEntry = 
			pOptionSystem->categories[categories[nSelectedCategory].nCategoryEntry].options[pOption->nOptionEntry];

		if ( pEntry.eEditorType == NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE ||
			pEntry.eEditorType == NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER ) 
		{
			pWindow = pOption->pWindow;
			pEditLineControl = dynamic_cast<IEditLine*>( pWindow );
			szMBCS.clear();
			NStr::ToMBCS( &szMBCS, pEditLineControl->GetText() );
			NGlobal::SetVar( pOption->szProgName, szMBCS );
			pOption->szCurrentValue = szMBCS;
		}
	}
}

void CInterfaceOptionsMenu::RestoreDefaultValues()
{
	//? Ask a question first?
	//TextEntry: T_OPTIONS_MENU_RESTORE_DEFAULTS_QUESTION

	for ( int j = 0 ; j < options.size(); ++j )
	{
		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &pEntry = 
			pOptionSystem->categories[options[j].nCategoryEntry].options[options[j].nOptionEntry];

		NGlobal::SetVar( options[j].szProgName, pEntry.szDefaultValue );
		options[j].szCurrentValue = pEntry.szDefaultValue;
		if ( pEntry.eEditorType == NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER) 
			options[j].fSliderPosition = NStr::ToFloat( pEntry.szDefaultValue );
	}
	
	ChangeResolution();
	SelectCategory( nSelectedCategory, true );		//and redraw current option list
}

void CInterfaceOptionsMenu::RollbackChanges()
{
	for ( int j = 0 ; j < options.size(); ++j )
	{
		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry &pEntry = 
			pOptionSystem->categories[options[j].nCategoryEntry].options[options[j].nOptionEntry];

		if ( pEntry.eEditorType == NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER) 
			options[j].fSliderPosition = NStr::ToFloat( options[j].szSavedValue );

		NGlobal::SetVar( options[j].szProgName, options[j].szSavedValue );
	}
	ChangeResolution();
}

void CInterfaceOptionsMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

void CInterfaceOptionsMenu::MsgAccept( const SGameMessage &msg )
{
	CommitEditLineChanges();
	ChangeResolution();
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
}

void CInterfaceOptionsMenu::MsgBack( const SGameMessage &msg )
{
	RollbackChanges();
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
}

bool CInterfaceOptionsMenu::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceOptionsMenu::SetMode( const std::string &szMode )
{
	if ( szMode == "mp_filters" ) 
		screenMode = MODE_FILTERS;
	else
		screenMode = MODE_NORMAL;
}

// CICOptionsMenu

void CICOptionsMenu::PreCreate()
{
}

void CICOptionsMenu::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	pInterface->SetMode( szMode );
	pInterface->FillScreen();
}

void CICOptionsMenu::Configure( const char *pszConfig )
{
	szMode = pszConfig;
}

static float s_fQuality = -1.0f;
static void CommandQuality( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		return;

	std::string szParam;
	NStr::ToMBCS( &szParam, paramsSet[0] );

	float fParam = -1;
	fParam = NStr::ToFloat( szParam );

	if ( fParam < 0 )
	{
		NGlobal::ProcessCommand( L"autodetect" ); 
		return;
	}

	CDBPtr<NDb::SGameRoot> pGameRoot = NGameX::GetGameRoot();
	CDBPtr<NDb::SOptionSystem> pOptionSystem = pGameRoot->pGameOptions;

	const NDb::SOptionSystem::SOptionsCategory::SOptionEntry *pQualityOption = 0;
	for ( int iCategorie = 0; iCategorie < pOptionSystem->categories.size(); ++iCategorie )
	{
		const NDb::SOptionSystem::SOptionsCategory &categorie = pOptionSystem->categories[iCategorie];
		for ( int iEntry = 0; iEntry < categorie.options.size(); ++iEntry )
		{			
			if ( categorie.options[iEntry].szProgName == "gfx_quality" )
			{
				pQualityOption = &categorie.options[iEntry];
				iCategorie = pOptionSystem->categories.size();
				break;
			}
		}
	}

	if ( !pQualityOption )
		return;

	// Update values that are described in the slider
	int nNumValues = pQualityOption->sliderValues[0].values.size();
	int nValue = Float2Int( fParam*nNumValues - 0.5f );
	nValue = Clamp( nValue, 0, nNumValues - 1 );

	for ( int iSliderVariable = 0; iSliderVariable < pQualityOption->sliderValues.size(); ++iSliderVariable )
	{
		ASSERT( nNumValues == pQualityOption->sliderValues[iSliderVariable].values.size() );

		const NDb::SOptionSystem::SOptionsCategory::SOptionEntry::SSliderSingleValue &sliderVariable = pQualityOption->sliderValues[iSliderVariable];
		const std::string &szProgName = sliderVariable.szProgName;
		const std::wstring wszValue = NStr::ToUnicode( sliderVariable.values[nValue] );

		NGlobal::SetVar( szProgName, wszValue );
	}

	NGlobal::SetVar( "gfx_quality", fParam );
}

static void CommandAutodetect( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( NGlobal::GetVar( "game_mode_editor", 0 ) != 0 )
	{
		// For editor should be the hightest quality
		NGlobal::ProcessCommand( std::wstring( L"set_quality 1" ) );
		return;
	}

	// Detecting computers perfomance and setting 
	NGScene::AutoDetectVideoConfig();
	const NGfx::SPerformanceInfo &performanceInfo = NGfx::GetPerformanceInfo();

	int nSpeed = NGScene::GetSpeedMode();

	float fPerfomance = max( 0.5f, (performanceInfo.fCPUclock - 1500.0f)/1500.0f );	
	fPerfomance = min( fPerfomance, max( 0, (performanceInfo.fFillRate - 100.0f)/100.0f ) );
	fPerfomance = min( fPerfomance, max( 0, (performanceInfo.fTriangleRate - 5.0f)/5.0f ) );
	fPerfomance = Clamp( fPerfomance, 0.0f, 1.0f );

	NGlobal::ProcessCommand( std::wstring( L"set_quality " ) + NStr::ToUnicode( StrFmt( "%f", fPerfomance ) ) );
}

START_REGISTER(CInterfaceOptionsMenu)
REGISTER_CMD( "autodetect", CommandAutodetect )
REGISTER_CMD( "set_quality", CommandQuality )
REGISTER_VAR_EX( "gfx_quality", NGlobal::VarFloatHandler, &s_fQuality, -1.0f, STORAGE_USER )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x170BDBC0, CInterfaceOptionsMenu )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_OPTIONS_MENU, CICOptionsMenu )
REGISTER_SAVELOAD_CLASS_NM( 0x170BDBC1, CReactions, CInterfaceOptionsMenu );

