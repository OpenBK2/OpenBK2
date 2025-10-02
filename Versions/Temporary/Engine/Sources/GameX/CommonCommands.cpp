#include "stdafx.h"

#include "CommonCommands.h"

#include "..\Input\Bind.h"

#include "../AILogic/B2AI.h"
#include "../Misc/2Darray.h"

// ************************************************************************************************************************ //
// **
// ** CControlSumCheckCommand
// **
// **
// **
// ************************************************************************************************************************ //

vector< list<unsigned long> > CControlSumCheckCommand::checkSums;
WORD CControlSumCheckCommand::wMask;

void CControlSumCheckCommand::Execute()
{
	checkSums[nPlayer].push_back( ulCheckSum );
}

void CControlSumCheckCommand::Init( const WORD _wMask )
{
	checkSums.clear();
	checkSums.resize( 16 );
	wMask = _wMask;
}

void CControlSumCheckCommand::SetMask( const WORD _wMask )
{
	wMask = _wMask;
}

void CControlSumCheckCommand::Check( const int nOurNumber, IAILogic *pAI )
{
	/*bool bFinished = false;
	NGlobal::SetVar( "out_of_sync", 0 );
	while ( !bFinished )
	{
		unsigned long checkSum;

		if ( checkSums[nOurNumber].empty() )
			return;
		else
			checkSum = checkSums[nOurNumber].front();

		int nChecks = 0;
		int nOutOfSyncs = 0;
		for ( int i = 0; i < checkSums.size() && !bFinished; ++i )
		{
			if ( wMask & ( 1UL << i ) )
			{
				if ( checkSums[i].empty() )
					bFinished = true;
				else
				{
					++nChecks;
					if ( checkSums[i].front() != checkSum )
					{
						//DEBUG{
						//Singleton<IGameTimer>()->Pause( true, 0 );
						//DEBUG}
						NInput::PostEvent( "player_state_changed", (i << 8) | 1, 0 );
						NGlobal::SetVar( "out_of_sync", 1 );
						++nOutOfSyncs;
					}
				}
			}
		}

		if ( NGlobal::GetVar( "out_of_sync", 0 ) )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 5, "MULTIPLAYER: OUT OF SYNC", 0xffff0000 );
		}
		else
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 5, "Multiplayer: is OK", 0xffff0000 );
		}

		if ( nOutOfSyncs > 0 )
		{
			// our player is async, if number of players is more than 2, we don't have to play with others
			if ( nChecks == 2 || nOutOfSyncs == nChecks - 1 )
			{
				NInput::PostEvent( "player_state_changed", 6, 0 );
				pAI->NoWin();
			}
			else if ( nChecks > 2 && nOutOfSyncs > 1 && nOutOfSyncs < nChecks - 1 )
			{
				NInput::PostEvent( "player_state_changed", 7, 0 );
				pAI->NoWin();
			}

			return;
		}

		if ( !bFinished )
		{
			for ( int j = 0; j < checkSums.size(); ++j )
			{
				if ( !checkSums[j].empty() )
					checkSums[j].pop_front();
			}
		}
	}*/
}

int CControlSumCheckCommand::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nPlayer );
	saver.Add( 2, &ulCheckSum );

	int nCheckSumsSize;
	if ( !saver.IsReading() )
		nCheckSumsSize = checkSums.size();
	saver.Add( 3, &nCheckSumsSize );

	if ( saver.IsReading() )
	{
		checkSums.clear();
		checkSums.resize( nCheckSumsSize );
	}

	return 0;
}

// ************************************************************************************************************************ //
// **
// ** CControlSumHistoryCommand
// **
// **
// **
// ************************************************************************************************************************ //

CArray2D<unsigned long> CControlSumHistoryCommand::checkSums;
WORD CControlSumHistoryCommand::wMask;

static const int nHistoryLength = 20;
static const int nHistoryTolerance = 5;

void CControlSumHistoryCommand::Execute()
{
	checkSums[nPlayer][nSegment % nHistoryLength] = ulCheckSum;
}

void CControlSumHistoryCommand::Init( const WORD _wMask )
{
	checkSums.Clear();
	checkSums.SetSizes( nHistoryLength, 16 );
	wMask = _wMask;
}

void CControlSumHistoryCommand::SetMask( const WORD _wMask )
{
	wMask = _wMask;
}

unsigned long CControlSumHistoryCommand::GetValue( const int _nPlayer, const int _nSegment )
{
	return checkSums[_nPlayer][_nSegment % nHistoryLength];
}

void CControlSumHistoryCommand::Check( const int nOurNumber, const int nStartSegment, IAILogic *pAI )
{
	NGlobal::SetVar( "out_of_sync", 0 );
	int nOutOfSyncs = 0;
	int nOutOfSyncWith = 0;		// number of players we are async with
	vector<bool> asyncWith;
	asyncWith.resize( 16, false );
	unsigned long checkSum;

	const int nIndex = nStartSegment % nHistoryLength;

	if ( nIndex >= 0 )			// First few segments give negative index
	{
		checkSum = checkSums[nOurNumber][nIndex];

		// Check preferred segment first
		for ( int j = 0; j < 16; ++j )
		{
			if ( j == nOurNumber || ( wMask & ( 1UL << j ) ) == 0 )
				continue;		// Own player / No player

			if ( checkSums[j][nIndex] != checkSum )
			{
				++nOutOfSyncs;
				break;
			}
		}
		if ( nOutOfSyncs == 0 )
		{
			// Preferred (current) segment is ok
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 5, "MP PERFECT", 0xffff0000 );
			return;
		}
	}

	nOutOfSyncs = 0;
	for ( int i = 0; i < nHistoryLength; ++i )
	{
		checkSum = checkSums[nOurNumber][i];

		for ( int j = 0; j < 16; ++j )
		{
			if ( j == nOurNumber || ( wMask & ( 1UL << j ) ) == 0 )
				continue;		// Own player / No player

			if ( checkSums[j][i] != checkSum )
			{
				++nOutOfSyncs;
				if ( !asyncWith[j] )
				{
					asyncWith[j] = true;
					++nOutOfSyncWith;
				}
			}
		}
	}

	if ( nOutOfSyncs > nHistoryTolerance )
	{
		//DEBUG{
		Singleton<IGameTimer>()->Pause( true, 0 );
		//DEBUG}
		NGlobal::SetVar( "out_of_sync", 1 );
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 1, 
			StrFmt( "ASYNC( %d with %d )", nOutOfSyncs, nOutOfSyncWith ) );

		// Drop out if many players
		if ( nOutOfSyncWith > 1 )
		{
			//TODO
		}
	}

	if ( NGlobal::GetVar( "out_of_sync", 0 ) )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 5, "MP OUT OF SYNC", 0xffff0000 );
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 5, "MP OK", 0xffff0000 );
	}
}

int CControlSumHistoryCommand::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nPlayer );
	saver.Add( 2, &ulCheckSum );
	saver.Add( 3, &nSegment );

	return 0;
}

// ************************************************************************************************************************ //
// **
// ** CDropPlayerCommand
// **
// **
// **
// ************************************************************************************************************************ //

void CDropPlayerCommand::Execute()
{
	Singleton<IAILogic>()->NeutralizePlayer( nPlayerToDrop );
}

REGISTER_SAVELOAD_CLASS( 0x300A73C5, CControlSumCheckCommand )
REGISTER_SAVELOAD_CLASS( 0x19191B40, CControlSumHistoryCommand )
REGISTER_SAVELOAD_CLASS( 0x300A73C6, CDropPlayerCommand )
