#include "stdafx.h"
#include "uiinternal.h"
#include "./effectorsenduimessage.h"


//CEffectorSendUIMessage

REGISTER_SAVELOAD_CLASS(0x11075C06, CEffectorSendUIMessage );

int CEffectorSendUIMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pElement );
	saver.Add( 2, &szMessageID );
	saver.Add( 3, &szParam);
	saver.Add( 4, &nForwardParam );
	saver.Add( 5, &nBackParam );
	saver.Add( 6, &szAnimatedWindow );
	saver.Add( 7, &bForward );
	saver.Add( 9, &bFinished );
	return 0;
}

void CEffectorSendUIMessage::Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &_szAnimatedWindow )
{
	const NDb::SUISSendUIMessage *pCmd( checked_cast<const NDb::SUISSendUIMessage*>( _pCmd ) );
	CParam<string> messageID( pCmd->szMessageID );
	CParam<string> param( pCmd->szParam );
	CParam<int> forwardParam( pCmd->nForwardParam );
	CParam<int> backParam( pCmd->nBackParam );
	szAnimatedWindow = _szAnimatedWindow;
	
	if ( pContext )
	{
		messageID.Merge( pContext->szMessageID );
		param.Merge( pContext->szParam );
		if ( !szAnimatedWindow.empty() )
			param.Merge( szAnimatedWindow );
		forwardParam.Merge( pContext->nForwardParam );
		backParam.Merge( pContext->nBackParam );
	}
	NI_ASSERT( messageID.IsValid(), "message id is invalid" );
	NI_ASSERT( param.IsValid(), "Param is invalid" );
	NI_ASSERT( forwardParam.IsValid(), "forwardParam is invalid" );
	NI_ASSERT( backParam.IsValid(), "backParam is invalid" );
	szMessageID = messageID.Get();
	szParam = param.Get();
	nForwardParam = forwardParam.Get();
	nBackParam = backParam.Get();

	bForward = true;
	bFinished = false;
}

const int CEffectorSendUIMessage::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{
	bool bResult = false;
	if ( !szAnimatedWindow.empty() )
	{
		IWindow *pWnd = pScreen->GetElement( szAnimatedWindow, true );
	//	NI_ASSERT( pWnd != NULL, StrFmt( "Animated window not found: %s", szAnimatedWindow.c_str() ) );
		if ( pWnd )
		{
			bResult = pWnd->ProcessMessage( SBUIMessage( szMessageID, szParam, bForward ? nForwardParam : nBackParam ) );
		}
	}
	if ( !bResult )
		pScreen->ProcessUIMessage( SBUIMessage( szMessageID, szParam, bForward ? nForwardParam : nBackParam ) );

	bFinished = true;
	return 0;
}

void CEffectorSendUIMessage::Reverse() 
{  
	bForward = !bForward;
	bFinished = false;
}

