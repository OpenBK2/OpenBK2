#pragma once

namespace NMeta
{

//! Statically (compile-time!) checks, that conversion from T to U exists
template <class T, class U>
class CConversion
{
	static int Test( U );
	static char Test( ... );
	static T MakeT();
public:
	enum { exists = sizeof( Test(MakeT()) ) == sizeof(int), sameType = 0 };
};

//! Partial specialization to determine same type
template <class T>
class CConversion<T, T>
{
public:
	enum { exists = 1, sameType = 1 };
};

//! Type traits. TStripType - strip type w/o const, pointer and ref qualifiers. volatile not supported!
template <class T>
class CTypeTraits
{
	template <class U> struct SStrip { typedef U Type; };
	template <class U> struct SStrip<U *> { typedef U Type; };
	template <class U> struct SStrip<const U> { typedef U Type; };
	template <class U> struct SStrip<const U *> { typedef U Type; };
	template <class U> struct SStrip<U &> { typedef U Type; };
	template <class U> struct SStrip<const U &> { typedef U Type; };
public:
	typedef typename SStrip<T>::Type TStripType;
};

}

//! Calculates, whenever class U derived from class T
#define SUPERSUBCLASS( T, U ) \
( (NMeta::CConversion<const U *, const T *>::exists) && \
  (!NMeta::CConversion<const T *, const void *>::sameType) )
//! Calculates, whenever class U derived from class T and they are not the same type
#define SUPERSUBCLASS_STRICT( T, U ) \
	( SUPERSUBCLASS( T, U ) && \
	(!NMeta::CConversion<const T, const U>::sameType) )


