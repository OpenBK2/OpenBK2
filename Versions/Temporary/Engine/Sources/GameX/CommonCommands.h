#pragma once

#include "AILogic/AILogicCommand.h"

#include <cstdint>

class CControlSumCheckCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CControlSumCheckCommand );

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nPlayer = {};
	uint32_t ulCheckSum = {};

	// не сэйвится!
	static std::vector<std::list<uint32_t>> checkSums;

public:
	static uint16_t wMask;

public:
	CControlSumCheckCommand() {}

	CControlSumCheckCommand( const int _nPlayer, const uint32_t _ulCheckSum )
		: nPlayer( _nPlayer )
		, ulCheckSum( _ulCheckSum )
	{
	}

	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return false;
	}

	static void Check( const int nOurNumber, struct IAILogic *pAI );
	static void Init( const uint16_t wMask );
	static void SetMask( const uint16_t wMask );

	int operator&( IBinSaver &saver ) override;
};

class CControlSumHistoryCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CControlSumHistoryCommand );

public:
	int nPlayer = {};
	uint32_t ulCheckSum = {};
	int nSegment = {};

	// не сэйвится!
	static CArray2D<uint32_t> checkSums;
	static uint16_t wMask;

public:
	CControlSumHistoryCommand() {}

	CControlSumHistoryCommand( const int _nPlayer, const int _nSegment, const uint32_t _ulCheckSum )
		: nPlayer( _nPlayer )
		, nSegment( _nSegment )
		, ulCheckSum( _ulCheckSum )
	{
	}

	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return false;
	}

	// only if CheckSum on nStartSegment is different, check other segments
	static void Check( const int nOurNumber, const int nStartSegment, struct IAILogic *pAI );

	static void Init( const uint16_t wMask );
	static void SetMask( const uint16_t wMask );

	static uint32_t GetValue( const int _nPlayer, const int _nSegment );

	int operator&( IBinSaver &saver ) override;
};

class CDropPlayerCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CDropPlayerCommand );

	ZDATA
	int nPlayerToDrop = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nPlayerToDrop );
		return 0;
	}

public:
	CDropPlayerCommand()
		: nPlayerToDrop( -1 )
	{
	}

	explicit CDropPlayerCommand( int _nPlayerToDrop )
		: nPlayerToDrop( _nPlayerToDrop )
	{
	}

	//
	void Execute() override;

	//
	// нужно ли сохранять в истории команд
	bool NeedToBeStored() const override
	{
		return true;
	}
};
