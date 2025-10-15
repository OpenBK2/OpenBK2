#include "stdafx.h"

#include "InTransportStates.h"
#include "Commands.h"
#include "Soldier.h"
#include "Technics.h"

#include "AILogic_export.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D491,	CInTransportStatesFactory );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D492, CSoldierRestOnBoardState );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*										  CInTransportStatesFactory										*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPtr<CInTransportStatesFactory> CInTransportStatesFactory::pFactory = 0;

IStatesFactory* CInTransportStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CInTransportStatesFactory();

	return pFactory;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInTransportStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		(	cmdType == ACTION_COMMAND_DIE ||
			cmdType == ACTION_COMMAND_DISAPPEAR );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IUnitState* CInTransportStatesFactory::ProduceState( class CQueueUnit *pUnit, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );		
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;
	
	switch ( cmd.nCmdType )
	{
		case ACTION_COMMAND_DIE:
			NI_ASSERT( false, "Command to die in the queue" );

			break;
		default:
			NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IUnitState* CInTransportStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );		
	return CSoldierRestOnBoardState::Instance( checked_cast<CSoldier*>(pUnit), 0 );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											CSoldierRestOnBoardState										*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IUnitState* CSoldierRestOnBoardState::Instance( CSoldier *pSoldier, CMilitaryCar *pTransport )
{
	return new CSoldierRestOnBoardState( pSoldier, pTransport );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSoldierRestOnBoardState::CSoldierRestOnBoardState( CSoldier *_pSoldier, CMilitaryCar* pTransport )
: pSoldier( _pSoldier )
{
	// если ещё не внутри транспорта
	if ( pTransport != 0 )
	{
		if ( pTransport->IsRefValid() && pTransport->IsAlive() )
		{
			pSoldier->SetInTransport( pTransport );
			pTransport->AddPassenger( pSoldier );
		}
		else 
			pSoldier->SetCommandFinished();
	}
	else
		NI_ASSERT( pSoldier->IsInTransport(), "Wrong unit state" );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSoldierRestOnBoardState::Segment()
{
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ETryStateInterruptResult CSoldierRestOnBoardState::TryInterruptState( class CAICommand *pCommand )
{ 
	pSoldier->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec2 CSoldierRestOnBoardState::GetPurposePoint() const
{
	if ( pSoldier && pSoldier->IsRefValid() && pSoldier->IsAlive() )	
		return pSoldier->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

