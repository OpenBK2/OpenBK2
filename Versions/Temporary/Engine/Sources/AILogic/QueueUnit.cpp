#include "stdafx.h"

#include "QueueUnit.h"
#include "StatesFactory.h"
#include "UnitStates.h"
#include "Commands.h"

#include "Artillery.h"
#include "Stats_B2_M1/ActionsRemap.h"

#include <cstdint>

extern NTimer::STime curTime;

CDecksSet< CObj<CAICommand> > CQueueUnit::cmds( SConsts::AI_START_VECTOR_SIZE );

void CQueueUnit::Init()
{
	bCmdFinished = false;
	pState = GetStatesFactory()->ProduceRestState( this );
	pCmdCurrent = 0;
	lastChangeStateTime = 0;
	bCommandAdded = true;
	bDoNotDeleteMeFromCommand = false;
}

void CQueueUnit::Clear() 
{ 
	cmds.Clear();
}

bool CQueueUnit::IsEmptyCmdQueue() const 
{
	return cmds.IsEmpty( GetUniqueIdQU() );
}

CAICommand* CQueueUnit::GetCurCmd() const 
{
	return pCmdCurrent;
}

void CQueueUnit::CheckCmdsSize( const int id )
{
	if ( id >= cmds.GetDecksNum() )
		cmds.IncreaseDecksNum( id * 1.5 );
}

void CQueueUnit::DelCmdQueue( const int id )
{
	for ( int i = cmds.begin( id ); i != cmds.end(); i = cmds.GetNext( i ) )
		cmds.GetEl( i ) = 0;
	cmds.DelDeck( id );
}

void CQueueUnit::InitWCommands( CQueueUnit *pUnit )
{
	FreezeByState( false );
	
	const uint16_t wThisID = GetUniqueIdQU();
	const uint16_t wUnitID = pUnit->GetUniqueIdQU();

	DelCmdQueue( wThisID );
	
	if ( pUnit->GetCurCmd() && CanCommandBeExecuted( pUnit->GetCurCmd() ) )
		cmds.Push( wThisID, pUnit->GetCurCmd() );

	while( !cmds.IsEmpty(wUnitID) )
	{
		CObj<CAICommand> pCmd = cmds.Peek( wUnitID );
		PopCmd( wUnitID );

		if ( CanCommandBeExecuted( pCmd ) )
			cmds.Push( wThisID, pCmd );
	}
}

void CQueueUnit::InsertUnitCommand( CAICommand *pCommand )
{
	CPtr<CAICommand> pGarbage = pCommand;
	
	FreezeByState( false );
	
	if ( !cmds.IsEmpty( GetUniqueIdQU() ) && cmds.Peek( GetUniqueIdQU() )->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR )
		return;

	if ( CanCommandBeExecuted( pCommand ) &&
			 GetStatesFactory()->CanCommandBeExecuted( pCommand ) )
	{
		//вставить текущую команду в очередь
		if ( IsValid( pCmdCurrent ) )
		{
			CAICommand *pOldCommand = new CAICommand( *pCmdCurrent );
			pOldCommand->SetFromAI( true );
			cmds.PushFront( GetUniqueIdQU(), pOldCommand );
		}

		if ( IsValid( pCmdCurrent ) )
		{
			bDoNotDeleteMeFromCommand = pCmdCurrent->ToUnitCmd().nCmdType == ACTION_COMMAND_SWARM_TO && 
				( pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||	pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_SWARM_ATTACK_FORMATION || pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_SWARM_ATTACK_OBJECT );

			pState->TryInterruptState( 0 );// грубо прервать состояние
			NullSegmTime();

			bDoNotDeleteMeFromCommand = false;
		}

		//вставить полученную команду в очередь
		cmds.PushFront( GetUniqueIdQU(), pCommand );
	}
}

void CQueueUnit::PushFrontUnitCommand( CAICommand *pCommand )
{
	CPtr<CAICommand> pGarbage = pCommand;
	
	FreezeByState( false );
	
	if ( !cmds.IsEmpty( GetUniqueIdQU() ) && cmds.Peek( GetUniqueIdQU() )->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR )
		return;

	if ( CanCommandBeExecuted( pCommand ) && GetStatesFactory()->CanCommandBeExecuted( pCommand ) )
	{
		if ( IsValid( pCmdCurrent ) )
		{
			pState->TryInterruptState( 0 );
			NullSegmTime();
		}

		cmds.PushFront( GetUniqueIdQU(), pCommand );
	}
}

void CQueueUnit::UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand )
{
	CPtr<CAICommand> pGarbage = pCommand;
	
	FreezeByState( false );
	
	CPtr<CAICommand> pCmd = pCommand;
	if ( !CanCommandBeExecutedByStats( pCommand ) )
	{
 		const EActionCommand &cmd = pCommand->ToUnitCmd().nCmdType;					
		if ( cmd == ACTION_COMMAND_MOVE_TO || cmd == ACTION_COMMAND_REVERSE_TO || cmd == ACTION_COMMAND_SWARM_TO )
			SendAcknowledgement( pCommand, ACK_NEGATIVE );

		return;
	}

	const SAIUnitCmd &unitCmd = pCmd->ToUnitCmd();
	NI_ASSERT( unitCmd.nCmdType != ACTION_COMMAND_DIE, "Can't process DIE command in CQueueUnit" );
	
	if ( !cmds.IsEmpty( GetUniqueIdQU() ) && cmds.Peek( GetUniqueIdQU() )->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR )
		return;
	
	if ( bPlaceInQueue )
		cmds.Push( GetUniqueIdQU(), pCmd.GetPtr() );
	//если текущее состояние завершено
	else if ( bCmdFinished || !pState )
	{
		DelCmdQueue( GetUniqueIdQU() );

		bCmdFinished = true;

		//добавить текушую команду в очередь
		cmds.Push( GetUniqueIdQU(), pCmd.GetPtr() );
	}
	//текущее состояние не завершено
	else 
	{
		if ( pCmd && pCmd->ToUnitCmd().nCmdType == ACTION_COMMAND_STOP_THIS_ACTION ) // прервать насильно
		{
			if ( !bCmdFinished )
			{
				pState->TryInterruptState( 0 );
				NullSegmTime();
			}
		}
		else  
		{
			const NDb::EUnitSpecialAbility eAbility1 = GetAbilityByCommand( pCmd->ToUnitCmd().nCmdType );
			// A hack to allow Entrench (which is not an ability) have a progress indicator (like a normal ability)
			const NDb::EUnitSpecialAbility eAbility = ( eAbility1 == NDb::ABILITY_ENTRENCH_SELF ) ?	NDb::ABILITY_NOT_ABILITY : eAbility1;
			bool bNeedToInterruptsState = false; 
						
			// ability commands
			if ( eAbility != NDb::ABILITY_NOT_ABILITY )
			{
				bCommandAdded = false;
				NotifyAbilityRun( pCommand ); // can add new command to queue and change bCommandAdded
				
				const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( eAbility );
				if ( !bCommandAdded && pSA )				
					bNeedToInterruptsState = pSA->bStopCurrentAction;
			}
			else
				bCommandAdded = true;

			const NDb::ESpecialAbilityParam eSAaction = NDb::ESpecialAbilityParam(int(pCmd->ToUnitCmd().fNumber));

			if ( eAbility == NDb::ABILITY_NOT_ABILITY || 
				!bCommandAdded && bNeedToInterruptsState && 
				eSAaction != NDb::PARAM_ABILITY_AUTOCAST_ON && eSAaction != NDb::PARAM_ABILITY_AUTOCAST_OFF ) 
			{						// спрашиваем у текущего состояния можно ли его прервать

				switch ( pState->TryInterruptState( pCommand ) )
				{
				case TSIR_YES_IMMIDIATELY:
					NullSegmTime();
				case TSIR_YES_WAIT:
					// почистить очередь
					DelCmdQueue( GetUniqueIdQU() );
					//добавить текушую команду в очередь
					cmds.Push( GetUniqueIdQU(), pCmd.GetPtr() );

					break;
				case TSIR_YES_MANAGED_ALREADY:
					// команду уже поместили в очередь
					break;
				case TSIR_NO_COMMAND_INCOMPATIBLE:
					// игнорировать команду
					break;
				}
			}
		}
	}
	// команду игнорировать
}

void CQueueUnit::SetCommandFinished()
{
	if ( !bDoNotDeleteMeFromCommand && IsValid( pCmdCurrent ) )
		pCmdCurrent->DeleteUnit( GetUniqueIdQU() );

	bCmdFinished = true;
	pCmdCurrent = 0;
}

void CQueueUnit::PopCmd( const int nID )
{
	cmds.Peek( nID ) = 0;
	cmds.Pop( nID );
}

void CQueueUnit::Segment()
{
	// отдыхаем, а есть команды, которые нужно выполнить
	if ( !bCmdFinished && IsValid( pState ) && pState->IsRestState() && !cmds.IsEmpty( GetUniqueIdQU() ) )
	{
		CObj<CAICommand> pCmd = cmds.Peek( GetUniqueIdQU() );
		PopCmd( GetUniqueIdQU() );
		switch ( pState->TryInterruptState( pCmd ) )
		{
			case TSIR_YES_IMMIDIATELY:
				NullSegmTime();
				cmds.PushFront( GetUniqueIdQU(), pCmd.GetPtr() );
				break;
			case TSIR_YES_WAIT:
				cmds.PushFront( GetUniqueIdQU(), pCmd.GetPtr() );
				// это нужно, если юнит захотел задержаться в RestState ( например если ждем починки )
				pState->Segment();
				break;
			case TSIR_YES_MANAGED_ALREADY:
				break;
			case TSIR_NO_COMMAND_INCOMPATIBLE:
				break;
		}
	} 
	// текущее состояние завершилось само
	else if ( bCmdFinished ) 
	{
		//bCmdFinished = false;
		CPtr<IUnitState> pNewState;
		do
		{
			if ( cmds.IsEmpty(GetUniqueIdQU()) )
			{
				pCmdCurrent = 0;
				pNewState = GetStatesFactory()->ProduceRestState( this );

				lastChangeStateTime = curTime;
				FreezeByState( false );
			}
			else // очередь команд не пуста
			{
				CObj<CAICommand> pCmd = cmds.Peek( GetUniqueIdQU() );
				
				// self-action
				const EActionCommand cmdType = pCmd->ToUnitCmd().nCmdType;
				if ( cmdType & 0x8000 )
				{
					pCmd = new CAICommand( *(cmds.Peek( GetUniqueIdQU() )) );
					pCmd->ToUnitCmd().nCmdType = EActionCommand( cmdType & ~0x8000 );
					pCmd->ToUnitCmd().vPos = checked_cast<CCommonUnit*>(this)->GetCenterPlain();
				}
				PopCmd( GetUniqueIdQU() );

				if ( pCmd && pCmd->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR )
				{
					checked_cast<CCommonUnit*>(this)->Disappear();
					return;
				}
				else if ( pCmd && pCmd->ToUnitCmd().nCmdType == ACTION_COMMAND_STOP_THIS_ACTION )
				{
					if ( IsValid( pState ) && !bCmdFinished )
					{
						pState->TryInterruptState( 0 );
						NullSegmTime();
					}

					pState = 0;
//					DebugTrace( "pState = 0 in CQueueUnit::Segment()" );
				}
				else if ( CanCommandBeExecuted( pCmd ) )
				{
					NotifyAbilityRun( pCmd );
					if ( GetStatesFactory()->CanCommandBeExecuted( pCmd ) )
					{
//						if ( curTime == 750 && GetUniqueIdQU() == 19 )
//							breakpoint();

						pNewState = GetStatesFactory()->ProduceState( this, pCmd );
						if ( pNewState )
							pCmdCurrent = pCmd;

						lastChangeStateTime = curTime;

						FreezeByState( false );
					}
				}
				else
				{
					// команда игнорируется					
				}
			}
		} while( pNewState == 0 );

		pState = pNewState;
		bCmdFinished = false;
	}
	else
		pState->Segment();
}

CAICommand* CQueueUnit::GetNextCommand() const
{
	if ( cmds.IsEmpty( GetUniqueIdQU() ) )
		return 0;
	else
		return cmds.Peek( GetUniqueIdQU() );
}

CAICommand* CQueueUnit::GetLastCommand() const
{
	if ( cmds.IsEmpty( GetUniqueIdQU() ) )
		return 0;
	else
		return cmds.GetLastEl( GetUniqueIdQU() );
}

void CQueueUnit::KillStatesAndCmdsInfo()
{
	DelCmdQueue( GetUniqueIdQU() );
	pCmdCurrent = 0;
	pState = 0;
//	DebugTrace( "pState = 0 in CQueueUnit::KillStatesAndCmdsInfo()" );
	bCmdFinished = false;
}

void CQueueUnit::SetCurState( IUnitState *_pState )
{
//	DebugTrace( "pState = 0x%p in CQueueUnit::SetCurState( ... )", _pState );
	pState = _pState;
}

const int CQueueUnit::GetBeginCmdsIter() const
{
	return cmds.begin( GetUniqueIdQU() );
}

const int CQueueUnit::GetEndCmdsIter() const
{
	return cmds.end();
}

const int CQueueUnit::GetNextCmdsIter( const int nIter ) const
{
	return cmds.GetNext( nIter );
}

CAICommand* CQueueUnit::GetCommand( const int nIter ) const
{
	return cmds.GetEl( nIter );
}


