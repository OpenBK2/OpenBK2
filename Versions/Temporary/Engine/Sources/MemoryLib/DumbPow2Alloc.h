#pragma once
#define _INC_NEW
#include <malloc.h>

namespace std
{
	class bad_alloc {};
	struct nothrow_t {};
	struct nothrow {};

	typedef void (__cdecl *new_handler)();
	inline new_handler set_new_handler(new_handler a) { return a; }
}

//void DebugRegister( size_t n, void *pRes );
//void DebugFree( void *p );

void* __cdecl operator new( size_t n );
void __cdecl operator delete( void *p );

void *__cdecl operator new[](size_t count); //_THROW1(std::bad_alloc)
void __cdecl operator delete[]( void * p );

inline void *__cdecl operator new(size_t, void *_P)
{return (_P); }
inline void __cdecl operator delete(void *, void *)
{return; }

