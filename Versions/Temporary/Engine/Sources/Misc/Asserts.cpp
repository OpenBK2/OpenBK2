#include "stdafx.h"
#include "BSAssertDialog.h"
#include "BSExceptionDialog.h"
#include "MemoryLib/SymAccess.h"
#include "BSUtil.h"
#include "BSUInit.h"


static void TypeDebugTrace( const char *buff, const char *pszWhat, const std::vector<SCallStackEntry> &entries )
{
	OutputDebugString( "*********************************************************************************************************\n" );
	OutputDebugString( buff );
	OutputDebugString( "\n" );
	OutputDebugString( pszWhat );
	OutputDebugString( "\n" );
	OutputDebugString( "CallStack entries dump:\n" );
	for ( int i = 0; i < entries.size(); ++i )
	{
		const SCallStackEntry &e = entries[i];
		char buff[1024];
		sprintf( buff, "%s(%d): %s\n", e.szFile.szStr, e.nLine, e.szFunc.szStr );
		OutputDebugString( buff );
	}
	OutputDebugString( "CallStack entries dump done\n" );
	OutputDebugString( "*********************************************************************************************************\n" );
}

// Sets the filter function that will be called when there is a fatal crash
static LPTOP_LEVEL_EXCEPTION_FILTER g_pfnOrigFilt = NULL ;
static bool bFilterIsSet;
//typedef LONG ( __stdcall *PFNCHFILTFN ) ( struct _EXCEPTION_POINTERS * pExPtrs );
static LONG __stdcall CrashHandlerFilter( struct _EXCEPTION_POINTERS *pExPtrs );
void SetCrashHandler()
{
	ASSERT( !bFilterIsSet );
	g_pfnOrigFilt = SetUnhandledExceptionFilter( CrashHandlerFilter );
	bFilterIsSet = true;
}
void ResetCrashHandler()
{
	if ( !bFilterIsSet )
		return;
	SetUnhandledExceptionFilter( g_pfnOrigFilt ) ;
	g_pfnOrigFilt = 0;
	bFilterIsSet = false;
}

// convert windows exception code to string
const char* ExceptionCodeToString( DWORD dwExceptionCode )
{
	switch ( dwExceptionCode )
	{
	case EXCEPTION_ACCESS_VIOLATION: 
		return "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: 
		return "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
	case EXCEPTION_BREAKPOINT: 
		return "A breakpoint was encountered.";
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		return "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.";
	case EXCEPTION_FLT_DENORMAL_OPERAND: 
		return "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: 
		return "The thread tried to divide a floating-point value by a floating-point divisor of zero.";
	case EXCEPTION_FLT_INEXACT_RESULT: 
		return "The result of a floating-point operation cannot be represented exactly as a decimal fraction.";
	case EXCEPTION_FLT_INVALID_OPERATION: 
		return "This exception represents any floating-point exception not included in this list.";
	case EXCEPTION_FLT_OVERFLOW: 
		return "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
	case EXCEPTION_FLT_STACK_CHECK: 
		return "The stack overflowed or underflowed as the result of a floating-point operation.";
	case EXCEPTION_FLT_UNDERFLOW: 
		return "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
	case EXCEPTION_ILLEGAL_INSTRUCTION: 
		return "The thread tried to execute an invalid instruction.";
	case EXCEPTION_IN_PAGE_ERROR: 
		return "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
	case EXCEPTION_INT_DIVIDE_BY_ZERO: 
		return "The thread tried to divide an integer value by an integer divisor of zero.";
	case EXCEPTION_INT_OVERFLOW: 
		return "The result of an integer operation caused a carry out of the most significant bit of the result.";
	case EXCEPTION_INVALID_DISPOSITION: 
		return "An exception handler returned an invalid disposition to the exception dispatcher.";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: 
		return "The thread tried to continue execution after a noncontinuable exception occurred.";
	case EXCEPTION_PRIV_INSTRUCTION: 
		return "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
	case EXCEPTION_SINGLE_STEP: 
		return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
	case EXCEPTION_STACK_OVERFLOW: 
		return "The thread used up its stack.";
	case 0xE06D7363:
		return "Microsoft C++ Exception";
	default:
		return "Unknown exception.";
	}
}

static const char *GetRegisterString( char *pszBuf, EXCEPTION_POINTERS * pExPtrs )
{
	// Check the parameter.
	ASSERT ( FALSE == IsBadReadPtr( pExPtrs, sizeof ( EXCEPTION_POINTERS ) ) ) ;
	if ( TRUE == IsBadReadPtr ( pExPtrs, sizeof ( EXCEPTION_POINTERS ) ) )
		return "???";

#ifdef _WIN64
	ASSERT ( !"IA64 is not supported (YET!) " ) ;
#else
	// This call puts 48 bytes on the stack, which could be a problem when
	// the stack is blown.
	sprintf ( pszBuf,
		"EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X  ESI=%08X\n"\
		"EDI=%08X  EBP=%08X  ESP=%08X  EIP=%08X  FLG=%08X\n"\
		"CS=%04X   DS=%04X  SS=%04X  ES=%04X   "\
		"FS=%04X  GS=%04X",
		pExPtrs->ContextRecord->Eax,
		pExPtrs->ContextRecord->Ebx,
		pExPtrs->ContextRecord->Ecx,
		pExPtrs->ContextRecord->Edx,
		pExPtrs->ContextRecord->Esi,
		pExPtrs->ContextRecord->Edi,
		pExPtrs->ContextRecord->Ebp,
		pExPtrs->ContextRecord->Esp,
		pExPtrs->ContextRecord->Eip,
		pExPtrs->ContextRecord->EFlags,
		pExPtrs->ContextRecord->SegCs,
		pExPtrs->ContextRecord->SegDs,
		pExPtrs->ContextRecord->SegSs,
		pExPtrs->ContextRecord->SegEs,
		pExPtrs->ContextRecord->SegFs,
		pExPtrs->ContextRecord->SegGs ) ;
#endif
	return pszBuf;
}

static LONG __stdcall CrashHandlerFilter( EXCEPTION_POINTERS *pExPtrs )
{
	char buff[1024];
	// first, get stack trace...
	std::vector<SCallStackEntry> entries;
	entries.resize( 1000 );
	int nEntries = CollectCallStack( pExPtrs, &entries[0], entries.size() );
	entries.resize( nEntries );

	// form <description> for unhandled exception
	if ( !entries.empty() )
		sprintf( buff, "First-chance exception in %s: 0x%.8X: %s.", entries[0].szFile.szStr, pExPtrs->ExceptionRecord->ExceptionCode, ExceptionCodeToString(pExPtrs->ExceptionRecord->ExceptionCode) );
	else
		sprintf( buff, "Unhandled Exception (0x%X) at the 0x%X: %s", pExPtrs->ExceptionRecord->ExceptionCode, pExPtrs->ExceptionRecord->ExceptionAddress, ExceptionCodeToString(pExPtrs->ExceptionRecord->ExceptionCode) );

	TypeDebugTrace( buff, ExceptionCodeToString( pExPtrs->ExceptionRecord->ExceptionCode ), entries );
	//
	char g_szBuf[1024];
	EBSUReport eRetCode = NBSU::ShowExceptionDlg( GetBSUInstance(), 0, buff, 
		ExceptionCodeToString( pExPtrs->ExceptionRecord->ExceptionCode ), 
		entries, GetRegisterString( g_szBuf, pExPtrs ) );

	// pExceptionRecord->ExceptionFlags != EXCEPTION_NONCONTINUABLE
	//
	switch ( eRetCode )
	{
	case BSU_DEBUG:
		return EXCEPTION_CONTINUE_SEARCH;
	case BSU_ABORT:
		ResetCrashHandler();					// reset crash handler filter
		FatalExit( 0xDEAD );
		return EXCEPTION_CONTINUE_SEARCH;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

namespace NBSU
{

static SIgnoresList ignores;
EBSUReport __stdcall ReportAssert( const char *pszCondition, const char *pszDescription, 
	const char *pszFileName, int nLineNumber )
{
	// first, check for ignore
	if ( IsIgnore( ignores, pszFileName, nLineNumber ) )
		return BSU_IGNORE;
	//
	std::vector<SCallStackEntry> entries;
	{
		entries.resize( 1000 );
		int nEntries = CollectCallStack( &entries[0], entries.size() );
		entries.resize( nEntries );
		if ( entries.size() >= 2 )
			entries.erase( entries.begin(), entries.begin() + 2 );
	}
	TypeDebugTrace( pszCondition, pszDescription, entries );

	EBSUReport eRetCode = ShowAssertionDlg( GetBSUInstance(), 0, pszFileName, nLineNumber, pszCondition, pszDescription, entries, ignores, 0 );

	return eRetCode;
}
}

