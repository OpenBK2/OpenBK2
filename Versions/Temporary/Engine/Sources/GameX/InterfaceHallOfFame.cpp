#include "stdafx.h"

#include "InterfaceStateInternal.h"
#include "InterfaceHallOfFame.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "DBGameRoot.h"
#include "Misc/StrProc.h"

#include "GameX_export.h"

// CHallOfFame

CHallOfFame::CHallOfFame()
{
	// write file
	CFileStream stream( "HallOfFame.bin", CFileStream::WIN_READ_ONLY );
	if ( !stream.IsOk() )
		return;

	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
	operator&( *pSaver.GetPtr() );
}

CHallOfFame theHallOfFame;

// CInterfaceHallOfFame

CInterfaceHallOfFame::CInterfaceHallOfFame() : 
CInterfaceScreenBase( "HallOfFame", "hall_of_fame" )
{
	AddObserver( "esc_pressed", &CInterfaceHallOfFame::MsgBack );
}

bool CInterfaceHallOfFame::Execute( const std::string &szSender, const std::string &szReaction )
{ 
	if ( szReaction == "menu_back" )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
		NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
		return true;
	}
	return false; 
}

bool CInterfaceHallOfFame::Init()
{
	theHallOfFame.AddRecord( L"Veselov", 12000 );

	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	AddUIScreen( pScreen, "HallOfFame", this );

	MakeInterior();
	return true;
}

void CInterfaceHallOfFame::MakeInterior()
{
	// determine max entries
	int nMaxEntries = 0;
	for ( int i = 1; ; ++i )
	{
		if ( !pScreen->GetChild( StrFmt( "%i", i ), true ) )
		{
			nMaxEntries = i-1;
			break;
		}
	}
	// ask data and fill it with default values
	const NDb::SGameRoot * pRoot = InterfaceState()->GetGameRoot();
	std::vector<SHallOfFameEntry> entries;
	entries.reserve( pRoot->hallOfFameDefaultRecords.size() + theHallOfFame.GetNRecords() );
	
	for ( int i = 0; i < pRoot->hallOfFameDefaultRecords.size(); ++i )
	{
		SHallOfFameEntry val;
		val.nScore = pRoot->hallOfFameDefaultRecords[i].nScore;
//		if ( pRoot->hallOfFameDefaultRecords[i].pName )
//			val.wszName = pRoot->hallOfFameDefaultRecords[i].pName->wszText;
		entries.push_back( val );
	}

	// insert player's records
	for ( int i = 0; i < theHallOfFame.GetNRecords(); ++i )
		entries.push_back( theHallOfFame.GetEntry( i ) );

	// resort altogether
	SHallOfFameEntryCompare pr;
	sort( entries.begin(), entries.end(), pr );
	
	// fill screen
	for ( int i = 0; i < nMaxEntries && i < entries.size(); ++i )
	{
		IWindow * pEntry = pScreen->GetChild( StrFmt( "%i", i+1 ), true );
		ITextView * pName = GetChildChecked<ITextView>( pEntry, "NameEntry", false );
		ITextView * pScore = GetChildChecked<ITextView>( pEntry, "ScoreEntry", false );
		if ( pName && pScore )
		{
			pName->SetText( pName->GetDBText() + entries[i].wszName );
			pScore->SetText( pScore->GetDBText() + NStr::ToUnicode( StrFmt( "%i", entries[i].nScore ) ) );
		}
	}
}

bool CInterfaceHallOfFame::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );		

	return bResult;
}

void CInterfaceHallOfFame::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

void CInterfaceHallOfFame::MsgBack( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
}

// CICHallOfFame

void CICHallOfFame::PreCreate()
{
}

void CICHallOfFame::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICHallOfFame::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1126D301, CInterfaceHallOfFame )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_SHOW_HALL_OF_FAME, CICHallOfFame )


