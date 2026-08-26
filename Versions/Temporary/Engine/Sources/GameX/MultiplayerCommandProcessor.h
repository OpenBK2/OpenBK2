
#pragma once

#include "Misc/HashFuncs.h"

enum EMPUIMessageType : int;
struct SMPUIMessage;

#define REGISTER_MPUI_MESSAGE_HANDLER( msgType, msgClass, proc ) \
	RegisterMessageHandler( msgType, MakeMPUIMessageHandler< msgClass >( this, (proc) ) );

struct IMPUIMessageHandler : public CObjectBase
{
	virtual bool HandleMessage( SMPUIMessage *pMsg ) = 0;
};

template<typename TMessage, typename TObj, typename TMFn>
class CMPUIMessageHandler : public IMPUIMessageHandler
{
	OBJECT_NOCOPY_METHODS( CMPUIMessageHandler );
	
	TObj *pObj;
	TMFn pMFn;
public:
	CMPUIMessageHandler() {}
	CMPUIMessageHandler( TObj *_pObj, TMFn _pMFn ) : 
		pObj( _pObj ),
		pMFn( _pMFn )
	{
	}

	bool HandleMessage( SMPUIMessage *_pMsg )
	{
		TMessage *pMsg = dynamic_cast<TMessage*>( _pMsg );
		NI_VERIFY( pMsg, "Wrong message class", return false );
		
		return (pObj->*pMFn)( pMsg );
	}
};

template<typename TMessage, typename TObj, typename TMFn>
IMPUIMessageHandler* MakeMPUIMessageHandler( TObj *pObj, TMFn pMFn )
{
	return new CMPUIMessageHandler< TMessage, TObj, TMFn >( pObj, pMFn );
}

class CMPUIMessageTranslator
{
	typedef std::unordered_map< EMPUIMessageType, CPtr<IMPUIMessageHandler>, SEnumHash > CHandlers;

	CHandlers handlers;
protected:
	// registers handler for incoming messages
	void RegisterMessageHandler( EMPUIMessageType eMsgType, IMPUIMessageHandler *pHandler );
public:
	// handles incoming message
	bool HandleMessage( SMPUIMessage *pMsg );
};

class CMPUIMessageProcessor : public CMPUIMessageTranslator
{
	typedef std::list< CPtr<SMPUIMessage> > CMessages;
	
	CMessages messages;
protected:
	// stores outgoing message	
	void PushMessage( SMPUIMessage *pMsg );
public:
	// gets outgoing message
	SMPUIMessage* GetMessage();
	// peeks outgoing message
	SMPUIMessage* PeekMessage();
};


