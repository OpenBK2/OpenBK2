#include "stdafx.h"
#include "./effectorconsolebuffercommand.h"


REGISTER_SAVELOAD_CLASS( UI, 0x110953C0, CEffectorConsoleBufferCommand )



//		CARSetSpecialAbility


int CEffectorConsoleBufferCommand::operator&( IBinSaver &saver )
{
	saver.Add( 1, &bFinished );
	saver.Add( 2, &szEditBoxName );
	return 0;
}

void CEffectorConsoleBufferCommand::Configure( const NDb::SUIStateBase *_pCmd,
																							struct IScreen *pScreen,
																							SWindowContext *pContext,
																							const std::string &szAnimatedWindow )
{
	const NDb::SUIConsoleCommand *pCmd( checked_cast<const NDb::SUIConsoleCommand*>( _pCmd ) );
	CParam<std::string> elementToAsk( pCmd->szEditBoxName );
	if ( pCmd->szEditBoxName.empty() )
		elementToAsk = szAnimatedWindow;

	szEditBoxName = elementToAsk.Get();
	bFinished = false;
}

const int CEffectorConsoleBufferCommand::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{
	// check IWindow that is passed as a param for string.
	// then pass this string to console buffer
	IWindow * pWnd = pScreen->GetElement( szEditBoxName, true );
	NI_ASSERT( pWnd != 0, StrFmt( "cannot find window \"%s\" to ask string", szEditBoxName.c_str() ) );
	IEditLine *pEdit = dynamic_cast<IEditLine*>( pWnd );
	NI_ASSERT( pEdit != 0, StrFmt( "it is not edit line \"%s\" ", szEditBoxName.c_str() ) );

	WriteToPipe( PIPE_CONSOLE_CMDS, pEdit->GetText() );
	pEdit->SetText( L"" );
	bFinished = true;
	return 0;
}

