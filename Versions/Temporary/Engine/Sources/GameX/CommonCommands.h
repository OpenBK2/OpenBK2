#pragma once

#include "../AILogic/AILogicCommand.h"

class CControlSumCheckCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CControlSumCheckCommand );

	int nPlayer;
	unsigned long ulCheckSum;

	// не сэйвится!
	static vector< list<unsigned long> > checkSums;
public:	
	static WORD wMask;
public:
	CControlSumCheckCommand() { }
	CControlSumCheckCommand( const int _nPlayer, const unsigned long _ulCheckSum ) 
		: nPlayer( _nPlayer ), ulCheckSum( _ulCheckSum ) { }

	void Execute();
	//
	bool NeedToBeStored() const { return false; }

	static void Check( const int nOurNumber, interface IAILogic *pAI );
	static void Init( const WORD wMask );
	static void SetMask( const WORD wMask );

	int operator&( IBinSaver &saver );
};

class CControlSumHistoryCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CControlSumHistoryCommand );

public:	
	int nPlayer;
	unsigned long ulCheckSum;
	int nSegment;

	// не сэйвится!
	static CArray2D<unsigned long> checkSums;
	static WORD wMask;
public:
	CControlSumHistoryCommand() { }
	CControlSumHistoryCommand( const int _nPlayer, const int _nSegment, const unsigned long _ulCheckSum ) 
		: nPlayer( _nPlayer ), nSegment( _nSegment ), ulCheckSum( _ulCheckSum ) { }

		void Execute();
		//
		bool NeedToBeStored() const { return false; }

		// only if CheckSum on nStartSegment is different, check other segments
		static void Check( const int nOurNumber, const int nStartSegment, interface IAILogic *pAI );

		static void Init( const WORD wMask );
		static void SetMask( const WORD wMask );

		static unsigned long GetValue( const int _nPlayer, const int _nSegment );

		int operator&( IBinSaver &saver );
};

class CDropPlayerCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CDropPlayerCommand );

	ZDATA
		int nPlayerToDrop;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nPlayerToDrop); return 0; }
public:
	CDropPlayerCommand() : nPlayerToDrop( -1 ) { }
	explicit CDropPlayerCommand( int _nPlayerToDrop ) : nPlayerToDrop( _nPlayerToDrop ) { }

	//
	void Execute();
	//
	// нужно ли сохранять в истории команд
	bool NeedToBeStored() const { return true; }
};

