#include "stdafx.h"
#include "Executor.h"

#include "System/AsyncTrace.h"

#include <fmt/format.h>

extern IExecutorContainer *pTheExecutorsContainer;

CExecutor::CExecutor( const EExecutorTypeID _eTypeID, const int _nNextTime )
	: eTypeID( _eTypeID )
	, nNextTime( _nNextTime )
	, nID( 0 )
	, bActive( true )
	, nUniqueID( -1 )
{
	if ( pTheExecutorsContainer )
	{
		nUniqueID = pTheExecutorsContainer->CreateID();
	}
	// The id comes out of a free list, so it says how many executors have been
	// created and returned before this one. Two runs that disagree about it
	// disagree about something earlier.
	if ( NAsyncTrace::IsEnabled() )
	{
		NAsyncTrace::Write(
				"exec", fmt::format( "created uid={} type={} next={}", nUniqueID, static_cast<int>( eTypeID ), nNextTime ) );
	}
}

CExecutor::CExecutor()
	: nUniqueID( -1 )
	, nID( 0 )
	, nNextTime( 0 )
	, bActive( true )
	, eTypeID( TID_NONE )
{
}

CExecutor::~CExecutor()
{
	if ( NAsyncTrace::IsEnabled() )
	{
		NAsyncTrace::Write( "exec", fmt::format( "destroyed uid={} type={}", nUniqueID, static_cast<int>( eTypeID ) ) );
	}
	if ( -1 != nUniqueID && pTheExecutorsContainer )
	{
		pTheExecutorsContainer->ReturnID( nUniqueID );
	}
}
