#ifndef __QUEUE_UNIT__
#define __QUEUE_UNIT__

#pragma once

#include "ListsSet.h"

class CAICommand;
interface IUnitState;
namespace NDb
{
	struct SUnitSpecialAblityDesc;
	enum EUnitSpecialAbility;
	enum EUnitAckType;
}

class CQueueUnit
{
	
	// деки команд
	static CDecksSet< CObj<CAICommand> > cmds;

	ZDATA
	// состояние
	CObj<IUnitState> pState;

	bool bCmdFinished; // состояние само завершилось

	CObj<CAICommand> pCmdCurrent; // текущая команда, выполняемая этим юнитом

	NTimer::STime lastChangeStateTime;
	
	// some abilities must interrupt current state. But some of them will and new command to
	// units queue during NotifyAbilityRun. This variable will guard command queue from
	// erasing by initial command, when new command was added.
	bool bCommandAdded;
	// sets to true when SWARM commands interrupts due attack
	bool bDoNotDeleteMeFromCommand;

	//
	void PopCmd( const int nID );
public:
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pState); f.Add(3,&bCmdFinished); f.Add(4,&pCmdCurrent); f.Add(5,&lastChangeStateTime); f.Add(6,&bCommandAdded); f.Add(7,&bDoNotDeleteMeFromCommand); return 0; }
private:
protected:
	virtual const int GetUniqueIdQU() const = 0;
public:
	void Init();

	bool IsEmptyCmdQueue() const;
	CAICommand* GetCurCmd() const;
	const bool IsCurCmdFinished() const { return bCmdFinished; }

	static void Clear();
	static void CheckCmdsSize( const int id );
	static void DelCmdQueue( const int id );

	virtual interface IStatesFactory* GetStatesFactory() const = 0;
	virtual IUnitState* GetState() const { return pState; }
	virtual void SetCurState( interface IUnitState *pState );

	// текущая команда кладётся в голову очереди, а сверху - pCommand
	virtual void InsertUnitCommand( class CAICommand *pCommand );
	// текущая команда прерывается, а в голову очереди кладётся pCommand
	virtual void PushFrontUnitCommand( class CAICommand *pCommand );
	virtual void UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand );
	
	void SetCommandFinished();
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand ) = 0;
	// может ли команда быть выполнена, исходя из статов юнита
	virtual bool CanCommandBeExecutedByStats( class CAICommand *pCommand ) = 0;
	virtual void NotifyAbilityRun( class CAICommand * pCommand ) = 0;

	void Segment();
	
	// возвращает команду, лежащую верхней деке команд
	class CAICommand* GetNextCommand() const;
	class CAICommand* GetLastCommand() const;

	// очистить очередь команд и обнулить текущее состояние
	void KillStatesAndCmdsInfo();
	virtual void SendAcknowledgement( EUnitAckType ack, bool bForce = false ) = 0;
	virtual void SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce = false ) = 0;

	// в очередь перенести текущую команду pUnit и все команды из его очереди  
	void InitWCommands( CQueueUnit *pUnit );
	
	virtual const NTimer::STime GetNextSegmTime() const { return 0; }
	// обнулить время, которое проходит между вызовами сегментов у юнита
	virtual void NullSegmTime() { }
	
	const NTimer::STime GetLastChangeStateTime() const { return lastChangeStateTime; }

	virtual void FreezeByState( const bool bFreeze ) = 0;

	const int GetBeginCmdsIter() const;
	const int GetEndCmdsIter() const;
	const int GetNextCmdsIter( const int nIter ) const;
	class CAICommand* GetCommand( const int nIter ) const;

	// Get ability desc if the unit has it, 0 if doesn't have it
	virtual const NDb::SUnitSpecialAblityDesc * GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType ) { return 0; }

	friend class CStaticMembers;
};

#endif // __QUEUE_UNIT__
