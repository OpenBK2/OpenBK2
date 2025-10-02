#include "stdafx.h"

#include "Commands.h"
#include "CommonUnit.h"
#include "..\Common_RTS_AI\PathFinder.h"
#include "GroupMover.h"
REGISTER_SAVELOAD_CLASS( 0x1108D44D, CAICommand );


extern NTimer::STime curTime;


//*******************************************************************
//*			 								   CAICommand																*
//*******************************************************************

CQueuesSet<SGroupPathInfo> CAICommand::paths;
CFreeIds CAICommand::cmdIds;

//BASIC_REGISTER_CLASS( CAICommand );

CAICommand::CAICommand( const SAIUnitCmd &_unitCmd )
 : unitCmd( _unitCmd ), nFlag( -1 )
{ 
	InitCmdId(); 
	pMover = CreateGroupMover( _unitCmd );
}

CAICommand::CAICommand( const CAICommand &cmd )
: unitCmd( cmd.unitCmd ), nFlag( cmd.nFlag ), pMover( cmd.pMover )
{
	InitCmdId();
}

void CAICommand::InitCmdId()
{
	id = cmdIds.Get();
	if ( id >= paths.GetQueuesNum() )
		paths.IncreaseQueuesNum( id * 1.5 );
}

void CAICommand::AddUnit( CCommonUnit *pUnit )
{
	if ( pMover )
		pMover->AddUnit( pUnit );
}

void CAICommand::DeleteUnit( const int nUnitID )
{
	if ( pMover )
		pMover->DeleteUnit( nUnitID );
}

IStaticPath* CAICommand::CreateStaticPath( CCommonUnit *pUnit )
{
	if ( pMover )
		return pMover->CreateStaticPath( pUnit );
	else
	{
		CPtr<IStaticPath> pPath = CreateStaticPathToPoint( unitCmd.vPos, pUnit->GetGroupShift(), pUnit, false, GetAIMap() );
		pUnit->SetSubGroup( -1 );
		pUnit->SetGroupShift( VNULL2 );
		return pPath.Extract();
	}
}

