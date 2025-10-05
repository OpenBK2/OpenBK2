#include "stdafx.h"

namespace NProgressHook
{
struct SLockInfo
{
	std::string szFileName;
	int nLine;
	DWORD dwTime;

	SLockInfo() : nLine( 0 ), dwTime( 0 ) { }
	SLockInfo( const std::string &_szFileName, const int _nLine, const DWORD _dwTime )
		: szFileName( _szFileName ), nLine( _nLine ), dwTime( _dwTime ) { }
};

static std::list<SLockInfo> locks;

void DebugLock( const std::string &szFileName, const int nLine )
{
	locks.push_back( SLockInfo( szFileName, nLine, GetTickCount() ) );
}

void DebugUnLock( const std::string &szFileName, const int nLine )
{
	NI_VERIFY( !locks.empty(), "wrong lock/unlock sequence", return );
	const DWORD dwTime = GetTickCount();
	const SLockInfo lockStart = locks.back();
	locks.pop_back();

	DbgTrc( "lock info: %d ms\n\t%s(%d) lock start\n\t%s(%d) lock end", 
					dwTime - lockStart.dwTime, lockStart.szFileName.c_str(), lockStart.nLine, szFileName.c_str(), nLine );
}

}


