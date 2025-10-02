
#pragma once
#include <float.h>

namespace NWin32Helper
{

class CEvent
{
	HANDLE h;
	CEvent( const CEvent& ) {}
	CEvent& operator=( const CEvent& ) {}
public:
	CEvent( bool bInitState = false, bool bManualReset = true ) { h = CreateEvent(0, bManualReset, bInitState, 0 ); }
	~CEvent() { CloseHandle(h); }
	bool Set() { return SetEvent(h) != 0; }
	bool Pulse() { return SetEvent(h) != 0; }
	bool Reset() { return ResetEvent(h) != 0; }
	void Wait() { WaitForSingleObject(h, INFINITE ); }
	bool IsSet() { return WaitForSingleObject( h, 0 ) == WAIT_OBJECT_0; }
};

template<class TSection>
class CTLock
{
	TSection &lock;
public:
	CTLock( TSection &_lock ): lock(_lock) { lock.Enter(); }
	~CTLock() { lock.Leave(); }

	__declspec(deprecated) void Enter() { lock.Enter(); } // DANGEROUS!
	__declspec(deprecated) void Leave() { lock.Leave(); } // DANGEROUS!
};

class CCriticalSection
{
	CRITICAL_SECTION sect;
	CCriticalSection( const CCriticalSection & ) {}
	CCriticalSection& operator=( const CCriticalSection &) {}
	//
	void Enter() { EnterCriticalSection( &sect ); }
	void Leave() { LeaveCriticalSection( &sect ); }
public:
	CCriticalSection() { InitializeCriticalSection( &sect ); }
	~CCriticalSection() { DeleteCriticalSection( &sect ); }
	friend class CTLock<CCriticalSection>;
};

typedef CTLock<CCriticalSection> CCriticalSectionLock;

template <class T>
class com_ptr
{
	T *pData;
	void Assign( T *_pData ) { if ( _pData ) { _pData->AddRef(); } pData = _pData; }
	void Free() { if ( pData ) pData->Release(); }
public:
	com_ptr( T *_pData = 0 ) { Assign( _pData ); }
	~com_ptr() { Free(); }
	com_ptr( const com_ptr &a ) { Assign( a.pData ); }
	com_ptr& operator=( const com_ptr &a ) { if ( pData == a.pData ) return *this; Free(); Assign( a.pData ); return *this; }
	com_ptr& operator=( T *pObj ) { if ( pData == pObj ) return *this; Free(); Assign( pObj ); return *this; }
	operator T*() const { return pData; }
	T* operator->() const { return pData; }
	// not fair play
	void Create( T *_pData ) { Free(); pData = _pData; }
	T** GetAddr() { Free(); pData = 0; return &pData; }
};

class CControl87Guard
{
	unsigned int nPrevState;
public:
	CControl87Guard() { nPrevState = _control87( 0, 0 ); }
	~CControl87Guard() { _control87( nPrevState, 0xffffffff ); }
};

class CPrecisionControl
{
	CControl87Guard guard;
public:
	enum EPrecisionControlMode{ PCM_HIGH = _PC_64, PCM_MEDIUM = _PC_53, PCM_LOW = _PC_24 };
	CPrecisionControl( EPrecisionControlMode mode = PCM_HIGH ) { _control87( mode, _MCW_PC ); }
	void Set( EPrecisionControlMode mode ) { _control87( mode, _MCW_PC ); };
};

class CRoundingControl
{
	CControl87Guard guard;
public:
	enum ERoundingControlMode{ RCM_NEAR = _RC_NEAR, RCM_DOWN = _RC_DOWN, RCM_UP = _RC_UP, RCM_CHOP = _RC_CHOP };
	CRoundingControl( ERoundingControlMode mode = RCM_NEAR ) { _control87( mode, _MCW_RC ); }
	void Set( ERoundingControlMode mode ) { _control87( mode, _MCW_RC ); };
};

class CSimpleCriticalSection
{
	int nLock;
	CSimpleCriticalSection( const CSimpleCriticalSection & ) {}
	CSimpleCriticalSection& operator=( const CSimpleCriticalSection &) {}
	//
	void Enter() 
	{ 
		int *pData = &nLock;
		_asm
		{
			mov esi, pData
Retry:
			mov eax, 1
			lock xchg [esi], eax
			test eax, eax
			jz Ok
			//pause
			push 0
			call dword ptr[ Sleep ]
			jmp Retry
Ok:
		}
	}
	void Leave() { nLock = 0; }
public:
	CSimpleCriticalSection() : nLock(0) {}
	friend class CTLock<CSimpleCriticalSection>;
};
typedef CTLock<CSimpleCriticalSection> CSimpleCriticalSectionLock;

}


