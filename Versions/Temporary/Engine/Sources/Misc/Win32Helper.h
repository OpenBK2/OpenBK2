
#pragma once

#include <boost/predef.h>

#include <cfenv>
// _control87 and the _PC_ constants, for the x87 precision field only
#include <cfloat>

namespace NWin32Helper
{

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

// Saves the floating-point environment and puts it back on scope exit.
// fegetenv/fesetenv cover the whole environment, where the Win32 version
// restored the rounding and denormal bits on x64 and the entire control word
// on x86. The one thing they also carry that the control word did not is the
// accumulated exception status, which nothing here reads.
class CControl87Guard
{
	std::fenv_t prevState;
public:
	CControl87Guard() { std::fegetenv( &prevState ); }
	~CControl87Guard() { std::fesetenv( &prevState ); }
};

// Precision control is a field of the x87 control word: it says what width
// arithmetic is rounded to regardless of the operand type. SSE has no such
// thing and neither does <cfenv>, so this stays Windows x86 only, which is
// what it already was - the x64 path never set it.
class CPrecisionControl
{
	CControl87Guard guard;
public:
	enum EPrecisionControlMode{ PCM_HIGH, PCM_MEDIUM, PCM_LOW };
	CPrecisionControl( EPrecisionControlMode mode = PCM_HIGH ) {
		Set( mode );
	}
	void Set( EPrecisionControlMode mode ) {
#if BOOST_OS_WINDOWS && BOOST_ARCH_X86_32
		switch ( mode )
		{
			case PCM_HIGH: _control87( _PC_64, _MCW_PC ); break;
			case PCM_MEDIUM: _control87( _PC_53, _MCW_PC ); break;
			case PCM_LOW: _control87( _PC_24, _MCW_PC ); break;
		}
#else
		(void)mode;
#endif
	}
};

// Rounding, unlike precision, is standard. Note that this now takes effect on
// x64, where Set used to do nothing at all and the guard therefore restored an
// environment nothing had changed. Every caller asks for RCM_NEAR, which is
// already the default, so this only matters if something else moved it.
class CRoundingControl
{
	CControl87Guard guard;
public:
	enum ERoundingControlMode{ RCM_NEAR = FE_TONEAREST, RCM_DOWN = FE_DOWNWARD, RCM_UP = FE_UPWARD, RCM_CHOP = FE_TOWARDZERO };
	CRoundingControl( ERoundingControlMode mode = RCM_NEAR ) {
		Set( mode );
	}
	void Set( ERoundingControlMode mode ) {
		std::fesetround( mode );
	}
};

}


