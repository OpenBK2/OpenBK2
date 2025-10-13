#include "stdafx.h"
#include "SymAccess.h"

#if BOOST_OS_WINDOWS
#include <DbgHelp.h>
#endif

#include <iterator>

template <class TYPE>
inline void ZeroSA( TYPE &val )
{
	memset( &val, 0, sizeof(val) );
}

CSymEngine::CSymEngine()
{
	hProcess = GetCurrentProcess();
	SymSetOptions ( SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS );
	if ( !SymInitialize( hProcess, 0, TRUE ) )
		hProcess = 0;
}

CSymEngine::~CSymEngine()
{
	if ( hProcess )
		SymCleanup( hProcess );
}

inline void Clear( CSymString *p ) { if ( p ) *p = "?"; }
bool CSymEngine::GetSymbol( DWORD dwAddress, CSymString *pszModule, CSymString *pszFile, int *pnLine, CSymString *pszFunc )
{
	Clear( pszModule );
	Clear( pszFile );
	Clear( pszFunc );
	if ( pnLine )
		*pnLine = 0;
	if ( !hProcess )
		return false;
	const int N_MAX_NAME_LENG = 1000;
	const int N_BUF_SIZE = sizeof(SYMBOL_INFO) + N_MAX_NAME_LENG;
	char szBuf[ N_BUF_SIZE ];
	memset( szBuf, 0, N_BUF_SIZE );

	if ( pszModule )
	{
		IMAGEHLP_MODULE64 moduleInfo;
		ZeroSA( moduleInfo );
		moduleInfo.SizeOfStruct = sizeof(moduleInfo);
		if ( SymGetModuleInfo64( hProcess, dwAddress, &moduleInfo ) )
			*pszModule = moduleInfo.ImageName;
	}

	if ( pszFunc )
	{
		SYMBOL_INFO *pInfo = (SYMBOL_INFO*)szBuf;
		pInfo->SizeOfStruct = std::size(szBuf);
		pInfo->MaxNameLen = N_MAX_NAME_LENG;
		if ( SymFromAddr( hProcess, dwAddress, 0, pInfo ) )
			*pszFunc = pInfo->Name;
	}

	if ( pnLine || pszFile )
	{
		IMAGEHLP_LINE64 lineInfo;
		ZeroSA( lineInfo );
		lineInfo.SizeOfStruct = sizeof(lineInfo);
		DWORD dwDisp;
		if ( SymGetLineFromAddr64( hProcess, dwAddress, &dwDisp, &lineInfo ) )
		{
			if ( pnLine )
				*pnLine = lineInfo.LineNumber;
			if ( pszFile )
				*pszFile = lineInfo.FileName;
		}
	}
	return true;
}

static void Assign( ADDRESS64 *pRes, DWORD dwSeg, DWORD64 dwOffset )
{
	pRes->Mode = AddrModeFlat;
	pRes->Offset = dwOffset;
	pRes->Segment = 0;//dwSeg;
}
int CollectCallStack( EXCEPTION_POINTERS *pExPtrs, SCallStackEntry *pRes, int nMaxEntries )
{
	CSymEngine se;
	STACKFRAME64 stkFrame;
	CONTEXT ctx;

	ZeroSA( stkFrame );
#if BOOST_ARCH_X86_32
	Assign( &stkFrame.AddrPC, pExPtrs->ContextRecord->SegCs, pExPtrs->ContextRecord->Eip );
	Assign( &stkFrame.AddrFrame, pExPtrs->ContextRecord->SegSs, pExPtrs->ContextRecord->Ebp );
	Assign( &stkFrame.AddrStack, pExPtrs->ContextRecord->SegSs, pExPtrs->ContextRecord->Eip );
#elif BOOST_ARCH_X86_64
	Assign( &stkFrame.AddrPC, pExPtrs->ContextRecord->SegCs, pExPtrs->ContextRecord->Rip );
	Assign( &stkFrame.AddrFrame, pExPtrs->ContextRecord->SegSs, pExPtrs->ContextRecord->Rbp );
	Assign( &stkFrame.AddrStack, pExPtrs->ContextRecord->SegSs, pExPtrs->ContextRecord->Rip );
#else
#error "Unsupported processor architecture"
#endif

	int nEntry = 0;
	for ( nEntry = 0; nEntry < nMaxEntries; ++nEntry )
	{
		BOOL bRes = StackWalk64( IMAGE_FILE_MACHINE_I386, se.GetProcess(), GetCurrentThread(), &stkFrame, &ctx, 0, 
			SymFunctionTableAccess64, SymGetModuleBase64, 0 );
		if ( !bRes || stkFrame.AddrPC.Offset == 0 )
			break;
		SCallStackEntry &res = pRes[nEntry];
		res.dwAddress = (DWORD)stkFrame.AddrPC.Offset;
		se.GetSymbol( res.dwAddress, 0, &res.szFile, &res.nLine, &res.szFunc );
	}
	return nEntry;
}

int CollectCallStack( SCallStackEntry *pRes, int nMaxEntries )
{
	CONTEXT ctx;
	EXCEPTION_POINTERS ep;
	ep.ContextRecord = &ctx;
	RtlCaptureContext(&ctx);
	return CollectCallStack( &ep, pRes, nMaxEntries );
}

CSymEngine &GetSymEngine()
{
	static CSymEngine se;
	return se;
}
