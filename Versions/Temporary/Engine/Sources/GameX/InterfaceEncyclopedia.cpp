#include "StdAfx.h"

#include "../3Dmotor/3Dmotor_export.h"

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../input/gamemessage.h"
#include "../ui/ui.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../ui/uifactory.h"
#include "InterfaceEncyclopedia.h"
#include "GameXClassIDs.h"
#include "../B2_M1_World/MapObj.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "../UISpecificB2/EffectorB2Move.h"
#include "../System/Text.h"

#ifdef _PROFILER
#include <VTuneAPI.h>
#pragma comment( lib, "vtuneapi.lib" )
#endif // _PROFILER

namespace NGScene
{
_3DMOTOR_EXPORT EXTERNVAR int nTextureUseMip;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
	CDBPtr<NDb::SUnitBaseRPGStats> pRunUnit;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CInterfaceEncyclopedia::CDataViewer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::CDataViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *_pData ) const
{
	const CUnitData *pData = dynamic_cast<const CUnitData*>( _pData );
	if ( pData )
	{
		if ( IListControlItem *pItem = dynamic_cast<IListControlItem*>( pWindow ) )
		{
			IWindow *pIconWnd = GetChildChecked<IWindow>( pItem, "Icon", true );
			IWindow *pIconShadowWnd = GetChildChecked<IWindow>( pItem, "IconShadow", true );
			ITextView *pDescView = GetChildChecked<ITextView>( pItem, "Desc", true );
			
			if ( pData->pStats )
			{
				if ( pIconWnd )
					pIconWnd->SetTexture( pData->pStats->pIconTexture );
				if ( pIconShadowWnd )
					pIconShadowWnd->SetTexture( pData->pStats->pIconTexture );
			}
			if ( pDescView )
				pDescView->SetText( pDescView->GetDBText() + pData->wszBriefDesc );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CInterfaceEncyclopedia::SDataSorter
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::SDataSorter::operator()( const CUnitData *pData1, const CUnitData *pData2 ) const
{
	if ( !pData1->pStats || !pData2->pStats )
		return pData1 < pData2;

	NDb::EEncyclopediaFilterUnitType eType1 = pData1->pStats->eEncyclopediaFilterUnitType;
	NDb::EEncyclopediaFilterUnitType eType2 = pData2->pStats->eEncyclopediaFilterUnitType;
	//{ CRAP - correct sort order
	if ( eType1 == NDb::EFUT_ARTILLERY )
		eType1 = NDb::EFUT_ARMOR;
	else if ( eType1 == NDb::EFUT_ARMOR )
		eType1 = NDb::EFUT_ARTILLERY;

	if ( eType2 == NDb::EFUT_ARTILLERY )
		eType2 = NDb::EFUT_ARMOR;
	else if ( eType2 == NDb::EFUT_ARMOR )
		eType2 = NDb::EFUT_ARTILLERY;
	//}
	if ( eType1 != eType2 )
		return eType1 < eType2;
	if ( pData1->pStats->ePoliticalSide != pData2->pStats->ePoliticalSide )
		return pData1->pStats->ePoliticalSide < pData2->pStats->ePoliticalSide;

	wstring wszName1;
	wstring wszName2;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pData1->pStats->,LocalizedName) )
		wszName1 = GET_TEXT_PRE(pData1->pStats->,LocalizedName);
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pData2->pStats->,LocalizedName) )
		wszName2 = GET_TEXT_PRE(pData2->pStats->,LocalizedName);

	transform( wszName1.begin(), wszName1.end(), wszName1.begin(), towlower ); 
	transform( wszName2.begin(), wszName2.end(), wszName2.begin(), towlower ); 

	if ( wszName1 != wszName2 )
		return wszName1 < wszName2;
	return pData1 < pData2;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CInterfaceEncyclopedia
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceEncyclopedia::CInterfaceEncyclopedia() :
	CInterfaceScreenBase( "Encyclopedia", "encyclopedia" )
{
	AddObserver( "menu_back", &CInterfaceEncyclopedia::MsgBack );
	AddObserver( "StepBackward", &CInterfaceEncyclopedia::MsgStepBackward );
	AddObserver( "StepForward", &CInterfaceEncyclopedia::MsgStepForward );
	AddObserver( "UnitSelectionChanged", &CInterfaceEncyclopedia::MsgUnitSelectionChanged );
	AddObserver( "HelpScreen", &CInterfaceEncyclopedia::MsgHelpScreen );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceEncyclopedia::~CInterfaceEncyclopedia()
{
	// Restore old texture resolution
	NGScene::nTextureUseMip = NGlobal::GetVar( "gfx_texture_mip", 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::Init()
{
	// Force all textures in the encyclopedia to be in the hightest resolution
	NGScene::nTextureUseMip = 0;

	// Init interface
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

//	pScreen->Enable( false );

	nLoading = 2;

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::EffectStart( bool bEnter )
{
	bEffectEnter = bEnter;
	bEffectBorder = false;

//	pScreen->Enable( false );

	CPtr<SWindowContextB2Move> pContext = new SWindowContextB2Move();
	pContext->bGoOut = !bEnter;
	pContext->fMaxMoveTime = 0.0f;
	nEffectCounter = 6;
	GetScreen()->RunStateCommandSequience( "effect_on_top", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_left", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_left2", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_right", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_right2", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_bottom", pScreen, pContext, true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::EffectFinish()
{
	if ( bEffectEnter )
	{
		EffectBorderStart( bEffectEnter );
	}
	else
	{
#ifdef _PROFILER
		VTPause();
#endif // _PROFILER

		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		if ( !pRunUnit )
			NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
		else
			NInput::PostEvent( "menu_return_from_encyclopedia", 0, 0 );
		pRunUnit = 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::EffectBorderStart( bool bEnter )
{
	bEffectEnter = bEnter;
	bEffectBorder = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::EffectBorderFinish()
{
	if ( bEffectEnter )
	{
//		pScreen->Enable( true );

		for ( vector<SFilterCache>::iterator it = filterCache.begin(); it != filterCache.end(); ++it )
		{
			SFilterCache &cache = *it;
			cache.pUnitListCont->Update();
		}
		UpdateSelectedUnitInfo();
	}
	else
	{
		EffectStart( bEffectEnter );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MakeInterior()
{
	CPtr<IWindow> pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	
	pUnitNameView = GetChildChecked<ITextView>( pMain, "UnitNameView", true );
	pUnitDescCont = GetChildChecked<IScrollableContainer>( pMain, "UnitDescContainer", true );
	pUnitDescView = GetChildChecked<ITextView>( pUnitDescCont, "UnitDescriptionView", true );
	p3DCtrl = GetChildChecked<IWindow3DControl>( pMain, "3DControl", true );
	pStepBackward = GetChildChecked<IButton>( pMain, "StepBackward", true );
	pStepForward = GetChildChecked<IButton>( pMain, "StepForward", true );
	IListControl* pUnitListCont = GetChildChecked<IListControl>( pMain, "UnitListContainer", true );

	filterCountries.resize( EFC_COUNT );
	filterCountries[EFC_ALL].szBtnName = "CountryAllBtn";
	filterCountries[EFC_US].szBtnName = "CountryUSBtn";
	filterCountries[EFC_UK].szBtnName = "CountryUKBtn";
	filterCountries[EFC_USSR].szBtnName = "CountryUSSRBtn";
	filterCountries[EFC_GERMANY].szBtnName = "CountryGermanyBtn";
	filterCountries[EFC_JAPAN].szBtnName = "CountryJapanBtn";

	filterUnitTypes.resize( EFUT_COUNT );
	filterUnitTypes[EFUT_ALL].szBtnName = "UnitTypesAllBtn";
	filterUnitTypes[EFUT_ARTILLERY].szBtnName = "UnitTypesArtilleryBtn";
	filterUnitTypes[EFUT_ARMOR].szBtnName = "UnitTypesArmorBtn";
	filterUnitTypes[EFUT_AIR].szBtnName = "UnitTypesAirBtn";
	filterUnitTypes[EFUT_SEA].szBtnName = "UnitTypesSeaBtn";
	filterUnitTypes[EFUT_TRANSPORT].szBtnName = "UnitTypesTransportBtn";
	filterUnitTypes[EFUT_MISC].szBtnName = "UnitTypesMiscBtn";

	for ( vector< SFilterBtn >::iterator it = filterCountries.begin(); it != filterCountries.end(); ++it )
	{
		SFilterBtn &filter = *it;
		filter.pBtn = GetChildChecked<IButton>( pMain, filter.szBtnName, true );
	}
	for ( vector< SFilterBtn >::iterator it = filterUnitTypes.begin(); it != filterUnitTypes.end(); ++it )
	{
		SFilterBtn &filter = *it;
		filter.pBtn = GetChildChecked<IButton>( pMain, filter.szBtnName, true );
	}
	
	if ( pUnitDescCont && pUnitDescView )
		pUnitDescCont->PushBack( pUnitDescView, false );
	if ( pStepBackward )
		pStepBackward->Enable( false );
	if ( pStepForward )
		pStepForward->Enable( false );
	if ( pUnitListCont )
		pUnitListCont->ShowWindow( false );
		
	pDataViewer = new CDataViewer();

	eSelCountry = EFC_ALL;
	eSelUnitType = EFUT_ALL;

	selectedUnits.reserve( 100 );
	nSelUnit = -1;
	
	MakeFilterCache( pUnitListCont );

	if ( pRunUnit )
	{
		UpdateFilters();
		AddSelectedUnit( GetUnitData( pRunUnit ), EFC_ALL, EFUT_ALL );
		UpdateSelectedUnitInfo();
		UpdateListCtrlSelection();
	}
	else
		UpdateInfo();

	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "FilterBtn" )
		return OnFilterBtn( szSender );

	if ( szReaction == "effect_finished" )
		return OnEffectFinished( szSender );

	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CInterfaceEncyclopedia::Check( const string &szCheckName ) const
{
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MsgBack( const SGameMessage &msg )
{
#ifdef _PROFILER
	VTResume();
#endif // _PROFILER
//	EffectBorderStart( false );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	if ( !pRunUnit )
		NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
	else
		NInput::PostEvent( "menu_return_from_encyclopedia", 0, 0 );
	pRunUnit = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MsgStepBackward( const SGameMessage &msg )
{
	if ( nSelUnit > 0 )
	{
		--nSelUnit;
		SetFiltersBySelection();
		UpdateListCtrlSelection();
		UpdateSelectedUnitInfo();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MsgStepForward( const SGameMessage &msg )
{
	if ( nSelUnit + 1 < selectedUnits.size() )
	{
		++nSelUnit;
		SetFiltersBySelection();
		UpdateListCtrlSelection();
		UpdateSelectedUnitInfo();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MsgUnitSelectionChanged( const SGameMessage &msg )
{
	SFilterCache &cache = filterCache[eSelCountry * EFUT_COUNT + eSelUnitType];
	if ( cache.pUnitListCont )
	{
		if ( IListControlItem *pItem = cache.pUnitListCont->GetSelectedListItem() )
		{
			AddSelectedUnit( dynamic_cast<CUnitData*>( pItem->GetUserData() ), eSelCountry, eSelUnitType );
			UpdateSelectedUnitInfo();
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MsgHelpScreen( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_HELP_SCREEN, GetInterfaceType().c_str() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::OnFilterBtn( const string &szName )
{
	for ( int i = 0; i < filterCountries.size(); ++i )
	{
		SFilterBtn &filter = filterCountries[i];
		if ( filter.szBtnName == szName )
		{
			if ( eSelCountry != i )
			{
				eSelCountry = (EFilterCountries)( i );
				UpdateInfo();
			}
			return true;
		}
	}

	for ( int i = 0; i < filterUnitTypes.size(); ++i )
	{
		SFilterBtn &filter = filterUnitTypes[i];
		if ( filter.szBtnName == szName )
		{
			if ( eSelUnitType != i )
			{
				eSelUnitType = (EFilterUnitTypes)( i );
				UpdateInfo();
			}
			return true;
		}
	}
	
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::OnEffectFinished( const string &szName )
{
	if ( bEffectBorder )
	{
		EffectBorderFinish();
	}
	else
	{
		nEffectCounter--;
		if ( nEffectCounter <= 0 )
			EffectFinish();
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::MakeFilterCache( IWindow *pSample )
{
	const NDb::SGameRoot *pRoot = InterfaceState()->GetGameRoot();
	
	filterCache.resize( EFC_COUNT * EFUT_COUNT );

	IListControlItem *pItemToSelect = 0;
	for ( int nCountry = 0; nCountry < EFC_COUNT; ++nCountry )
	{
		for ( int nUnitType = 0; nUnitType < EFUT_COUNT; ++nUnitType )
		{
			SFilterCache &cache = filterCache[nCountry * EFUT_COUNT + nUnitType];
			if ( pSample )
				cache.pUnitListCont = dynamic_cast<IListControl*>( AddWindowCopy( pSample->GetParentWindow(), pSample ) );
			if ( !cache.pUnitListCont )
				continue;

			cache.pUnitListCont->ShowWindow( false );
			cache.pUnitListCont->SetViewer( pDataViewer );

			cache.filteredUnits.reserve( 300 );
			if ( pRoot )
			{
				for ( int i = 0; i < pRoot->encyclopediaMechUnits.size(); ++i )
				{
					const NDb::SMechUnitRPGStats *pStats = pRoot->encyclopediaMechUnits[i];
					if ( !pStats )
						continue;

					EFilterCountries eCountry;
					switch ( pStats->ePoliticalSide )
					{
						case NDb::POLITICAL_SIDE_ALLIES:
							eCountry = EFC_UK;
							break;

						case NDb::POLITICAL_SIDE_GERMAN:
							eCountry = EFC_GERMANY;
							break;

						case NDb::POLITICAL_SIDE_JAPAN:
							eCountry = EFC_JAPAN;
							break;

						case NDb::POLITICAL_SIDE_USSR:
							eCountry = EFC_USSR;
							break;
							
						case NDb::POLITICAL_SIDE_USA:
							eCountry = EFC_US;
							break;
							
						default:
							NI_ASSERT( 0, "Unknown diplomacy side" );
							eCountry = EFC_ALL;
							break;
					}
					if ( (EFilterCountries)( nCountry ) != EFC_ALL && (EFilterCountries)( nCountry ) != eCountry )
						continue;

					EFilterUnitTypes eUnitType = EFUT_ALL;
					switch ( pStats->eEncyclopediaFilterUnitType )
					{
						case NDb::EFUT_ARTILLERY:
							eUnitType = EFUT_ARTILLERY;
							break;

						case NDb::EFUT_ARMOR:
							eUnitType = EFUT_ARMOR;
							break;

						case NDb::EFUT_AIR:
							eUnitType = EFUT_AIR;
							break;

						case NDb::EFUT_SEA:
							eUnitType = EFUT_SEA;
							break;

						case NDb::EFUT_TRANSPORT:
							eUnitType = EFUT_TRANSPORT;
							break;

						case NDb::EFUT_MISC:
							eUnitType = EFUT_MISC;
							break;

						default:
							NI_ASSERT( 0, "Unknown unit type" );
							eUnitType = EFUT_ALL;
							break;
					}
					if ( (EFilterUnitTypes)( nUnitType ) != EFC_ALL && (EFilterUnitTypes)( nUnitType ) != eUnitType )
						continue;
					
					CUnitData *pData = GetUnitData( pStats );
					if ( pData )
					{
						if ( CHECK_TEXT_NOT_EMPTY_PRE(pStats->,LocalizedName) )
							pData->wszBriefDesc = GET_TEXT_PRE(pStats->,LocalizedName);
						cache.filteredUnits.push_back( pData );
					}
				}
			}
			sort( cache.filteredUnits.begin(), cache.filteredUnits.end(), SDataSorter() );
			for ( vector< CPtr<CUnitData> >::iterator it = cache.filteredUnits.begin(); it != cache.filteredUnits.end(); ++it )
			{
				CUnitData *pData = *it;
				IListControlItem *pItem = cache.pUnitListCont->AddItem( pData );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::UpdateInfo()
{
	UpdateFilters();
	
	SFilterCache &cache = filterCache[eSelCountry * EFUT_COUNT + eSelUnitType];
	if ( !cache.filteredUnits.empty() )
	{
		AddSelectedUnit( cache.filteredUnits.front(), eSelCountry, eSelUnitType );
	}

	UpdateSelectedUnitInfo();

	UpdateListCtrlSelection();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::UpdateFilters()
{
	for ( int i = 0; i < filterCountries.size(); ++i )
	{
		SFilterBtn &filter = filterCountries[i];
		if ( filter.pBtn )
			filter.pBtn->SetState( (i == eSelCountry) ? 1 : 0 );
	}

	for ( int i = 0; i < filterUnitTypes.size(); ++i )
	{
		SFilterBtn &filter = filterUnitTypes[i];
		if ( filter.pBtn )
			filter.pBtn->SetState( (i == eSelUnitType) ? 1 : 0 );
	}
	
	for ( int nCountry = 0; nCountry < EFC_COUNT; ++nCountry )
	{
		for ( int nUnitType = 0; nUnitType < EFUT_COUNT; ++nUnitType )
		{
			SFilterCache &cache = filterCache[nCountry * EFUT_COUNT + nUnitType];
			
			if ( cache.pUnitListCont )
			{
				bool bShow = (nCountry == eSelCountry && nUnitType == eSelUnitType);
				cache.pUnitListCont->ShowWindow( bShow );
				if ( bShow )
					cache.pUnitListCont->Update();
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::UpdateSelectedUnitInfo()
{
	if ( pStepBackward )
		pStepBackward->Enable( nSelUnit > 0 );
	if ( pStepForward )
		pStepForward->Enable( nSelUnit + 1 < selectedUnits.size() );
		
	if ( nSelUnit >= 0 && nSelUnit < selectedUnits.size() )
	{
		SSelectedUnit &unit = selectedUnits[nSelUnit];
		const NDb::SUnitBaseRPGStats *pStats = unit.pData->pStats;
		if ( pUnitDescView )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pStats->,FullDescription) )
				pUnitDescView->SetText( pUnitDescView->GetDBText() + GET_TEXT_PRE(pStats->,FullDescription) );
			else
				pUnitDescView->SetText( L"" );
		}
		if ( pUnitNameView )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pStats->,LocalizedName) )
				pUnitNameView->SetText( pUnitNameView->GetDBText() + GET_TEXT_PRE(pStats->,LocalizedName) );
			else
				pUnitNameView->SetText( L"" );
		}
		if ( pUnitDescCont )
		{
			pUnitDescCont->ResetScroller();
			pUnitDescCont->Update();
		}

		if ( p3DCtrl )
		{
			vector<IWindow3DControl::SObject> objects;
			
			IWindow3DControl::SParam param = p3DCtrl->GetDBObjectParam( 0 );
			IWindow3DControl::SObject object;
			object.nID = 0;
			object.pModel = GetModel( pStats->pvisualObject, NDb::SEASON_SUMMER );
			NI_ASSERT( object.pModel, StrFmt( "No model for object: \"%s\"", NDb::GetResName(pStats) ) );
			object.vPos = param.vPos;
			object.vSize = param.vSize;
			objects.push_back( object );
			
			p3DCtrl->SetObjects( objects );
		}
	}
	else
	{
		if ( pUnitDescView )
			pUnitDescView->SetText( L"" );
		if ( pUnitDescCont )
		{
			pUnitDescCont->ResetScroller();
			pUnitDescCont->Update();
		}
		if ( pUnitNameView )
			pUnitNameView->SetText( L"" );
		if ( p3DCtrl )
			p3DCtrl->SetObjects( vector<IWindow3DControl::SObject>() );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::AddSelectedUnit( CUnitData *pData, EFilterCountries eCountry, EFilterUnitTypes eUnitType )
{
	if ( !pData )
		return;
	if ( nSelUnit >= 0 && nSelUnit + 1 < selectedUnits.size() )
		selectedUnits.resize( nSelUnit + 1 ); // удалим лишние юниты в undo
	SSelectedUnit unit;
	unit.pData = pData;
	unit.eCountry = eCountry;
	unit.eUnitType = eUnitType;
	selectedUnits.push_back( unit );
	nSelUnit = selectedUnits.size() - 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::SetFiltersBySelection()
{
	if ( nSelUnit >= 0 && nSelUnit < selectedUnits.size() )
	{
		SSelectedUnit &unit = selectedUnits[nSelUnit];

		eSelCountry = unit.eCountry;
		eSelUnitType = unit.eUnitType;
		UpdateFilters();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceEncyclopedia::UpdateListCtrlSelection()
{
	if ( nSelUnit >= 0 && nSelUnit < selectedUnits.size() )
	{
		SSelectedUnit &unit = selectedUnits[nSelUnit];

		SFilterCache &cache = filterCache[eSelCountry * EFUT_COUNT + eSelUnitType];
		if ( cache.pUnitListCont )
			cache.pUnitListCont->SelectItem( unit.pData );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceEncyclopedia::CUnitData* CInterfaceEncyclopedia::GetUnitData( const NDb::SUnitBaseRPGStats *pStats )
{
	if ( !pStats )
		return 0;

	hash_map< CDBPtr<NDb::SUnitBaseRPGStats>, CPtr<CUnitData>, SDBPtrHash >::iterator it = knownUnits.find( pStats );
	if ( it != knownUnits.end() )
	{
		return it->second;
	}
	else
	{
		CUnitData *pData = new CUnitData();
		pData->pStats = pStats;
		knownUnits[pStats] = pData;
		return pData;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::RunWithUnit( const NDb::SUnitBaseRPGStats *pStats )
{
	pRunUnit = pStats;
	NMainLoop::Command( ML_COMMAND_ENCYCLOPEDIA_WAIT, "" );
	
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopedia::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	if ( nLoading > 0 )
	{
		--nLoading;
		if ( nLoading == 0 )
			EffectStart( true );
	}

	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CInterfaceEncyclopediaWait
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceEncyclopediaWait::CInterfaceEncyclopediaWait() :
	CInterfaceScreenBase( "EncyclopediaWait", "encyclopedia_wait" )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopediaWait::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	nLoading = 2;
	
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopediaWait::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( nLoading > 0 )
	{
		--nLoading;
		if ( nLoading == 0 )
		{ 
			NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
			if ( !pRunUnit )
			{
				NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
			}
			NMainLoop::Command( ML_COMMAND_ENCYCLOPEDIA, "" );
		}
	}
	
	return bResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceEncyclopediaWait::Execute( const string &szSender, const string &szReaction )
{
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CInterfaceEncyclopediaWait::Check( const string &szCheckName ) const
{
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CICEncyclopedia
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopedia::PreCreate()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopedia::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopedia::Configure( const char *pszConfig )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CICEncyclopediaWait
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopediaWait::PreCreate()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopediaWait::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICEncyclopediaWait::Configure( const char *pszConfig )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x17189481, CInterfaceEncyclopedia )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_ENCYCLOPEDIA, CICEncyclopedia )
REGISTER_SAVELOAD_CLASS_NM( 0x1718A380, CUnitData, CInterfaceEncyclopedia )
REGISTER_SAVELOAD_CLASS_NM( 0x1718A400, CDataViewer, CInterfaceEncyclopedia )
REGISTER_SAVELOAD_CLASS( 0x1718CB41, CInterfaceEncyclopediaWait )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_ENCYCLOPEDIA_WAIT, CICEncyclopediaWait )
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
