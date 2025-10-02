#include "StdAfx.h"
#include "executor.h"

extern IExecutorContainer *pTheExecutorsContainer;

CExecutor::CExecutor( const EExecutorTypeID _eTypeID, const int _nNextTime ) 
	: eTypeID( _eTypeID ), nNextTime( _nNextTime ),
	nID( 0 ), bActive( true ), nUniqueID( -1 )
{
	if ( pTheExecutorsContainer )
		nUniqueID = pTheExecutorsContainer->CreateID();
}

CExecutor::CExecutor()
: nUniqueID( -1 ), nID( 0 ), nNextTime( 0 ), bActive( true ), eTypeID( TID_NONE )
{
}

CExecutor::~CExecutor()
{
	if ( -1 != nUniqueID && pTheExecutorsContainer )
		pTheExecutorsContainer->ReturnID( nUniqueID );
}

