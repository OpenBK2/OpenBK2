#include "StdAfx.h"
#include "MultiplayerCommandProcessor.h"
#include "MultiplayerCommandManager.h"

// CMPUIMessageTranslator

void CMPUIMessageTranslator::RegisterMessageHandler( EMPUIMessageType eMsgType, IMPUIMessageHandler *pHandler )
{
	NI_ASSERT( handlers.find( eMsgType ) == handlers.end(), "Message handler already registered" );
	handlers[eMsgType] = pHandler;
}

bool CMPUIMessageTranslator::HandleMessage( SMPUIMessage *pMsg )
{
	CHandlers::iterator it = handlers.find( pMsg->eMessageType );
	if ( it == handlers.end() )
		return false;
	return it->second->HandleMessage( pMsg );
}

// CMPUIMessageProcessor

void CMPUIMessageProcessor::PushMessage( SMPUIMessage *pMsg )
{
	messages.push_back( pMsg );
}

SMPUIMessage* CMPUIMessageProcessor::GetMessage()
{
	if ( messages.empty() )
		return 0;
		
	CPtr<SMPUIMessage> pMsg = messages.front();
	messages.pop_front();
	return pMsg.Extract();
}

SMPUIMessage* CMPUIMessageProcessor::PeekMessage()
{
	if ( messages.empty() )
		return 0;
		
	return messages.front();
}


