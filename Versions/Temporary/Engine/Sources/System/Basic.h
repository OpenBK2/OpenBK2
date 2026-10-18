#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "Misc/Asserts.h" // ASSERT

#include "System_export.h"

#include <cstdint>

// single thread version
// базовые классы для системы подсчета ссылок на объекты с целью упрощения memory management
//
//   Есть разные шаблоны указателей - простые указатели - CPtr с добавлением ссылки и
// указатели, обладающие правом собственности - CObj/CMObj. После уничтожения CObj объект, на который
// он ссылался очищается и помечается как невалидный. Аналогично действует CMObj организуя
// паралелльное владение объектом (используется когда объект может быть уничточжен из-за
// влияния внешних обстоятельств)
//   Ограничения - иногда может потребоваться в .cpp файле использовать BASIC_REGISTER_CLASS()
// - при использовании указателей на forward declared classes
// Нельзя переопределять operator new, так как удалятся объекты будут с помощью стандартного
// operator`a delete (из-за delete this)

#pragma warning( disable : 4250 )

//#define TRACK_OBJECTS_STATISTICS
// Если включить этот макрос и вызвать PrintObjectsStatistics(), то в Output будет выведена
// статистика для всех типов объектов, производных от CObjectBase, существующих в текущий момент.
// Функция PrintObjectsStatistics() вызывается по нажатию клавиш Ctrl+Shift+M

class CObjectBase;
#ifdef TRACK_OBJECTS_STATISTICS
void RegisterInObjectsSet( CObjectBase * );
void UnRegisterInObjectsSet( CObjectBase * );
void PrintObjectsStatistics();
#else
inline void PrintObjectsStatistics() {}
#endif

class SYSTEM_EXPORT CObjectBase
{
private:
#if defined( _DEBUG ) && !defined( FAST_DEBUG )
	void AddRef()
	{
		if ( nRefData == 0 )
		{
			strncpy( szObjectName, GetTypeName(), 12 );
		}
		++nRefData;
	}

	void AddObj( int nRef )
	{
		if ( nObjData == 0 )
		{
			strncpy( szObjectName, GetTypeName(), 12 );
		}
		nObjData += nRef;
	}
#else
	void AddRef()
	{
		++nRefData;
	}

	void AddObj( int nRef )
	{
		nObjData += nRef;
	}
#endif
	void DecRef()
	{
		--nRefData;
	}

	void DecObj( int nRef )
	{
		nObjData -= nRef;
	}

	void ReleaseRef();
	void ReleaseObj( int nRef, int nMask );
	static void DestroyDelayed();

protected:
#if defined( _DEBUG ) && !defined( FAST_DEBUG )
	char szObjectName[12]{};
#endif
	int nObjData = 0;
	int nRefData = 0;
	// function should clear contents of object, easy to implement via consequent calls to
	// destructor and constructor, this function should not be called directly, use Clear()
	virtual void DestroyContents() = 0;

	virtual CObjectBase *MakeCopy() const
	{
		ASSERT( 0 );
		return 0;
	}

	virtual ~CObjectBase()
	{
#ifdef TRACK_OBJECTS_STATISTICS
		UnRegisterInObjectsSet( this );
#endif
	}

	inline void CopyValidFlag( const CObjectBase &a )
	{
		nObjData &= 0x7fffffff;
		nObjData |= a.nObjData & 0x80000000;
	}

public:
	CObjectBase()
		: nObjData( 0 )
		, nRefData( 0 )
	{
#ifdef TRACK_OBJECTS_STATISTICS
		RegisterInObjectsSet( this );
#endif
	}

	// do not copy refcount when copy object
	CObjectBase( const CObjectBase &a )
		: nObjData( 0 )
		, nRefData( 0 )
	{
		CopyValidFlag( a );
#ifdef TRACK_OBJECTS_STATISTICS
		RegisterInObjectsSet( this );
#endif
	}

	CObjectBase &operator=( const CObjectBase &a )
	{
		CopyValidFlag( a );
		return *this;
	}

	//
	int IsRefInvalid() const
	{
		return ( nObjData & 0x80000000 );
	}

	int IsRefValid() const
	{
		return !IsRefInvalid();
	}

	// do not use
	void SetInvalid( bool b )
	{
		nObjData &= 0x7fffffff;
		if ( b )
		{
			nObjData |= 0x80000000;
		}
	}

	// reset data in class to default values, saves RefCount from destruction
	void Clear()
	{
		AddRef();
		DestroyContents();
		DecRef();
	}

	//
	virtual int operator&( struct IBinSaver &f )
	{
		return 0;
	}

	virtual uint32_t CalcCheckSum() const
	{
		return 0;
	}

	const char *GetTypeName() const;
	const char *GetFullTypeName() const;
	virtual int GetSizeOf() const = 0;

	//
	// due to absense of template friend classes in vc these classes are needed
	struct SRefO
	{
		void AddRef( CObjectBase *pObj )
		{
			pObj->AddObj( 1 );
		}

		void DecRef( CObjectBase *pObj )
		{
			pObj->DecObj( 1 );
		}

		void Release( CObjectBase *pObj )
		{
			pObj->ReleaseObj( 1, 0x000fffff );
		}
	};

	struct SRefM
	{
		void AddRef( CObjectBase *pObj )
		{
			pObj->AddObj( 0x100000 );
		}

		void DecRef( CObjectBase *pObj )
		{
			pObj->DecObj( 0x100000 );
		}

		void Release( CObjectBase *pObj )
		{
			pObj->ReleaseObj( 0x100000, 0x7ff00000 );
		}
	};

	struct SRef
	{
		void AddRef( CObjectBase *pObj )
		{
			pObj->AddRef();
		}

		void DecRef( CObjectBase *pObj )
		{
			pObj->DecRef();
		}

		void Release( CObjectBase *pObj )
		{
			pObj->ReleaseRef();
		}
	};
	friend struct CObjectBase::SRef;
	friend struct CObjectBase::SRefO;
	friend struct CObjectBase::SRefM;
};

// macro that helps to make CFundament derivative to be serializable
// and makes sure that class will not be copied or destroyed via obj references

// macro that helps to create neccessary members for proper operation of refcount system
// if class needs special destructor, use CFundament

// DestroyContents saves the two refcounts across the rebuild, and it has to
// address them through pointers taken before the destructor runs rather than
// through this->.
//
// nObjData and nRefData live in CObjectBase, and CObjectBase is a *virtual*
// base of a good deal of the UI (IWindow, IScreen, IClickNotify and the rest
// all say "virtual public CObjectBase"). Reaching a virtual base's member
// through this-> means reading the object's vptr to find where that base sits.
// After ~classname() has run there is no vptr left to read: the destructor
// walks it down through each base and the storage is dead. GCC reloads it
// anyway, gets whatever the destructor left there, and the "virtual base
// offset" it computes from that is a wild number.
//
// That is not theoretical. It is the crash the Linux port sat on at main menu
// teardown: a virtual thunk to CWindowScreen::DestroyContents faulting at the
// first read of this->nRefData, with the whole body inlined into the thunk.
// MSVC survives it, which is why this went twenty years without being noticed,
// but reading a dead object's vptr has always been undefined, and that half of
// the problem is what the hoisted pointers fix.
//
// The offset of a member within CObjectBase is fixed, so a pointer taken while
// the object was still alive stays correct across both the destructor and the
// placement new, and needs no vptr. Reading after the destructor is deliberate:
// the destructor can drop references to this object, and those decrements have
// to be carried over.
//
// Whether that read is undefined is less clear than it looks, so the
// suppression below is a judgement call rather than either a defence or a
// confession. [basic.life]/1 ends an object's lifetime when "the destructor call
// starts" only for a class type with a non-trivial destructor; otherwise the
// lifetime runs until the storage is released or reused. nRefData and nObjData
// are plain ints, a destructor destroys only the members that are of class type,
// and nothing reuses the storage until the placement new below. On that reading
// the two int subobjects are still within their lifetime when they are read, the
// pointers above were taken while they were alive, and this is well defined. The
// reading that goes the other way is that [basic.life] also forbids using a
// glvalue that names a member of an object whose lifetime has ended, and
// *pnRefData does name one. The compilers do not agree either, so treat this as
// grey rather than as one colour or the other.
//
// GCC reports it because its lifetime model is coarser than the standard's: it
// clobbers the whole object at the end of a destructor and does not track that
// scalar members outlive the class object. It also merges the two adjacent int
// loads into one 8-byte vector load, which is where the "((__vector(2)
// int*)this)[1] is used uninitialized" wording comes from. clang does not report
// it at all, which is not the same as clang promising anything.
//
// That clobber is an optimisation input and not only a diagnostic, so the
// question that actually matters is whether it lets the destructor's decrements
// be dropped as dead stores. Checked rather than assumed, on the shape that
// matters (virtual base, destructor decrementing both counts, GCC 14.2 at -O1,
// -O2 and -O3, 2026-10-18): the decrements are emitted and survive, the virtual
// base offset is computed from the vptr before the destructor runs, and the
// carry-over below comes out correct, with the constructor's zero stores and the
// += folded into a single copy. One compiler version on a reduced case is not a
// promise about the next one, which is why this paragraph exists: if counts ever
// come out wrong at teardown, recheck this instead of trusting it.
//
// Sampling the counts before the destructor instead, which is the first thing
// everyone tries, cannot work. The destructor body and the member destructors it
// runs can both change our own counts: a member dropping its last reference to a
// texture can put us on a delayed cleanup list, and anything holding a CPtr to
// us releases it as it dies. The value that has to be carried over is the one
// that exists once all of that has run, so there is no earlier moment to read.
//
// The library has nothing that helps:
//   - std::destroy_at is specified as p->~T() and std::construct_at as
//     ::new ( voidify( p ) ) T(). They are these same two operations with
//     different spellings, not safer ones, and the lifetime question is about
//     what happens between them. construct_at does hand back a pointer to the
//     new object, which is worth having for the two += below, but placement new
//     already returns one.
//   - std::launder yields a pointer to an object that exists. In the window here
//     nothing exists at that address, so there is nothing to launder; it is the
//     tool for the opposite problem, where the object is alive and a pointer's
//     provenance is stale.
//   - std::start_lifetime_as would be exactly the tool if the read were
//     undefined, since it starts an int lifetime over the leftover bytes and
//     yields their value. It is C++23 (P2590), this tree is C++17, and the
//     libstdc++ the Linux build uses does not implement it yet.
//
// Fixing it properly means getting the counts out of the destructor, and a
// destructor has no return value and no parameter, so there are exactly two
// channels for that.
//
// A well-known location: arm a slot in DestroyContents, have ~CObjectBase copy
// its own counts into it while they are indisputably alive, then read the slot
// after the destructor returns. About twenty lines, and no new property for this
// subsystem to carry, since Basic.cpp already drives this very path through the
// file statics bDestroyInProgress, pDestroy and pInvalidate and the header above
// says "single thread version". It costs one compare per destruction, to reject
// the nested destructions that run inside our own, and thread_local is the
// answer if the single-thread assumption ever goes away.
//
// Counters outside the object, which is the design this wants: a wrapper owning
// the counts with the object nested inside it, the object addressing them
// indirectly, the wrapper outliving it by construction. That is
// std::shared_ptr's control block, and it is why weak_ptr may inspect a
// destroyed pointee at all. It would delete the hard part rather than patch it:
// the only reason to rebuild the object in place is that holders need something
// valid to point at once the contents are cleared, so if CPtr and CObj pointed
// at the wrapper, invalidation would become "destroy the object, mark the
// wrapper dead", the 0x80000000 bit would move into the wrapper, and
// DestroyContents would leave all 1606 expansions. The price, counted in this
// tree rather than guessed: 1751 direct "new <macro class>" sites across 982 of
// these classes, 234 by-value declarations across 51 of them with no wrapper to
// bind to at all (SAIEntranceUpdate is a stack local in
// B2_M1_World/UpdatableWorld.cpp), the delete this in ReleaseObj/ReleaseRef
// having to destroy the wrapper instead so the object has to be able to find it,
// and MakeCopy having to produce a wrapped copy from somewhere that is not a
// copy constructor. A prefix block reached through a custom operator new is the
// same idea and is ruled out outright by the no-overriding-operator-new rule at
// the top of this file.
//
// The cheapest in-place alternative stays what it was: CObjectBase::operator=
// deliberately leaves both counts alone and copies only the valid flag, which is
// the invariant this code rebuilds by hand, and the comment on DestroyContents
// calls destructor-plus-constructor merely "easy to implement" rather than
// required. So the body wants to collapse to:
//
//     void DestroyContents() override { *this = classname(); }
//
// no lifetime ends, no placement new, no vptr to read, and the destructor's
// decrements land on live counts so nothing needs carrying over. Not done
// because it is not a one-liner at this scale:
//   - 897 classes use OBJECT_BASIC_METHODS and 709 use OBJECT_NOCOPY_METHODS.
//     The nocopy half is not assignable by definition and needs a different
//     answer, and that is the half the second suppression below sits in.
//   - MakeCopy proves these classes are copy-constructible, not copy-assignable;
//     any const or reference member breaks the assignment form.
//   - It changes the valid flag. The release path sets nObjData |= 0x80000000
//     before calling here, and the current code restores it because nHoldObjs
//     carries that bit. CopyValidFlag would instead copy the flag from the fresh
//     temporary, which is valid, so a dying object would come back marked live.
// Reviving this needs the refcount tests and a teardown pass, not just a build.
#if defined( __GNUC__ )
// _Pragma is standard C++11 ([cpp.pragma.op]) and is the only way to emit a
// pragma from a macro body; the GCC diagnostic payload is the extension. clang
// defines __GNUC__ and understands this spelling. MSVC is excluded on purpose:
// an unrecognised pragma there is C4068 at warning level 1, once per expansion
// over 1606 classes, and nothing in this tree sets /Zc:preprocessor so _Pragma
// itself may not be available in the preprocessor mode it builds with.
#define OBJECT_HELD_REFCOUNT_READ_PUSH \
	_Pragma( "GCC diagnostic push" ) _Pragma( "GCC diagnostic ignored \"-Wuninitialized\"" )
#define OBJECT_HELD_REFCOUNT_READ_POP _Pragma( "GCC diagnostic pop" )
#else
#define OBJECT_HELD_REFCOUNT_READ_PUSH
#define OBJECT_HELD_REFCOUNT_READ_POP
#endif
#define OBJECT_BASIC_METHODS( classname )                           \
public:                                                             \
	static CObjectBase *New##classname()                              \
	{                                                                 \
		return new classname();                                         \
	}                                                                 \
	classname *Duplicate() const                                      \
	{                                                                 \
		return dynamic_cast<classname *>( MakeCopy() );                 \
	}                                                                 \
	int GetSizeOf() const override                                    \
	{                                                                 \
		return sizeof( classname );                                     \
	}                                                                 \
                                                                    \
protected:                                                          \
	CObjectBase *MakeCopy() const override                            \
	{                                                                 \
		return new classname( *this );                                  \
	}                                                                 \
	void DestroyContents() override                                   \
	{                                                                 \
		int *pnRefData = &this->nRefData, *pnObjData = &this->nObjData; \
		this->classname::~classname();                                  \
		OBJECT_HELD_REFCOUNT_READ_PUSH                                  \
		int nHoldRefs = *pnRefData, nHoldObjs = *pnObjData;             \
		OBJECT_HELD_REFCOUNT_READ_POP                                   \
		new ( this ) classname();                                       \
		this->nRefData += nHoldRefs;                                    \
		this->nObjData += nHoldObjs;                                    \
	}                                                                 \
	~classname() override {}                                          \
                                                                    \
private:
#define OBJECT_NOCOPY_METHODS( classname )                          \
public:                                                             \
	static CObjectBase *New##classname()                              \
	{                                                                 \
		return new classname();                                         \
	}                                                                 \
	int GetSizeOf() const override                                    \
	{                                                                 \
		return sizeof( classname );                                     \
	}                                                                 \
                                                                    \
protected:                                                          \
	void DestroyContents() override                                   \
	{                                                                 \
		int *pnRefData = &this->nRefData, *pnObjData = &this->nObjData; \
		this->classname::~classname();                                  \
		OBJECT_HELD_REFCOUNT_READ_PUSH                                  \
		int nHoldRefs = *pnRefData, nHoldObjs = *pnObjData;             \
		OBJECT_HELD_REFCOUNT_READ_POP                                   \
		new ( this ) classname();                                       \
		this->nRefData += nHoldRefs;                                    \
		this->nObjData += nHoldObjs;                                    \
	}                                                                 \
                                                                    \
private:
#define BASIC_REGISTER_CLASS( module, classname )                                                    \
	template <>                                                                                        \
	module##_EXPORT CObjectBase *CastToObjectBaseImpl<classname>( classname * p, void * )              \
	{                                                                                                  \
		return p;                                                                                        \
	}                                                                                                  \
	template <>                                                                                        \
	module##_EXPORT classname *CastToUserObjectImpl<classname>( CObjectBase * p, classname *, void * ) \
	{                                                                                                  \
		return dynamic_cast<classname *>( p );                                                           \
	}

template <class TUserObj>
CObjectBase *CastToObjectBaseImpl( TUserObj *p, void * );

template <class TUserObj>
CObjectBase *CastToObjectBaseImpl( TUserObj *p, CObjectBase * )
{
	return p;
}
template <class TUserObj>
TUserObj *CastToUserObjectImpl( CObjectBase *p, TUserObj *, void * );

template <class TUserObj>
TUserObj *CastToUserObjectImpl( CObjectBase *p, TUserObj *, CObjectBase * )
{
	return dynamic_cast<TUserObj *>( p );
}

template <class TUserObj>
inline CObjectBase *CastToObjectBase( TUserObj *p )
{
	return CastToObjectBaseImpl( p, p );
}

template <class TUserObj>
inline TUserObj *CastToUserObject( CObjectBase *p, TUserObj *pu )
{
	return CastToUserObjectImpl( p, pu, pu );
}

// TObject - base object for reference counting, TUserObj - user object name
// TRef - struct with AddRef/DecRef/Release methods for refcounting to use
template <class TUserObj, class TRef>
class CPtrBase
{
private:
	TUserObj *ptr = nullptr;

	//
	void AddRef( TUserObj *_ptr )
	{
		TRef p;
		if ( _ptr )
		{
			p.AddRef( CastToObjectBase( _ptr ) );
		}
	}

	void DecRef( TUserObj *_ptr )
	{
		TRef p;
		if ( _ptr )
		{
			p.DecRef( CastToObjectBase( _ptr ) );
		}
	}

	void Release( TUserObj *_ptr )
	{
		TRef p;
		if ( _ptr )
		{
			p.Release( CastToObjectBase( _ptr ) );
		}
	}

protected:
	void SetObject( TUserObj *_ptr )
	{
		TUserObj *pOld = ptr;
		ptr = _ptr;
		AddRef( ptr );
		Release( pOld );
	}

	TUserObj *Get() const
	{
		return ptr;
	}

public:
	CPtrBase()
		: ptr( 0 )
	{
	}

	CPtrBase( TUserObj *_ptr )
		: ptr( _ptr )
	{
		AddRef( ptr );
	}

	CPtrBase( const CPtrBase &a )
		: ptr( a.ptr )
	{
		AddRef( ptr );
	}

	~CPtrBase()
	{
		Release( ptr );
	}

	//
	void Set( TUserObj *_ptr )
	{
		SetObject( _ptr );
	}

	TUserObj *Extract()
	{
		TUserObj *pRes = ptr;
		DecRef( ptr );
		ptr = 0;
		return pRes;
	}

	//
	// assignment operators
	CPtrBase &operator=( TUserObj *_ptr )
	{
		Set( _ptr );
		return *this;
	}

	CPtrBase &operator=( const CPtrBase &a )
	{
		Set( a.ptr );
		return *this;
	}

	// access
	TUserObj *operator->() const
	{
		return ptr;
	}

	operator TUserObj *() const
	{
		return ptr;
	}

	TUserObj *GetPtr() const
	{
		return ptr;
	}

	CObjectBase *GetBarePtr() const
	{
		return CastToObjectBase( ptr );
	}

	int operator&( IBinSaver &f );
};

template <class T>
inline bool IsValid( T *p )
{
	return p != 0 && !CastToObjectBase( p )->IsRefInvalid();
}

template <class T, class TRef>
inline bool IsValid( const CPtrBase<T, TRef> &p )
{
	return p.GetPtr() && !p.GetBarePtr()->IsRefInvalid();
}

#define BASIC_PTR_DECLARE( TPtrName, TRef )  \
	template <class T>                         \
	class TPtrName : public CPtrBase<T, TRef>  \
	{                                          \
		typedef CPtrBase<T, TRef> CBase;         \
                                             \
	public:                                    \
		typedef T CDestType;                     \
		TPtrName() {}                            \
		TPtrName( T *_ptr )                      \
			: CBase( _ptr )                        \
		{                                        \
		}                                        \
		TPtrName( const TPtrName &a )            \
			: CBase( a )                           \
		{                                        \
		}                                        \
		TPtrName &operator=( T *_ptr )           \
		{                                        \
			CBase::Set( _ptr );                    \
			return *this;                          \
		}                                        \
		TPtrName &operator=( const TPtrName &a ) \
		{                                        \
			CBase::SetObject( a.Get() );           \
			return *this;                          \
		}                                        \
		int operator&( IBinSaver &f )            \
		{                                        \
			return ( *(CBase *)this ) & ( f );     \
		}                                        \
	};
//
BASIC_PTR_DECLARE( CPtr, CObjectBase::SRef );
BASIC_PTR_DECLARE( CObj, CObjectBase::SRefO );
BASIC_PTR_DECLARE( CMObj, CObjectBase::SRefM );

template <typename T>
struct std::hash<CPtr<T>>
{
	std::size_t operator()( const CPtr<T> &p ) const
	{
		return std::hash<T *>()( p.GetPtr() );
	}
};

template <typename T>
struct std::hash<CObj<T>>
{
	std::size_t operator()( const CObj<T> &p ) const
	{
		return std::hash<T *>()( p.GetPtr() );
	}
};

template <typename T>
struct std::hash<CMObj<T>>
{
	std::size_t operator()( const CMObj<T> &p ) const
	{
		return std::hash<T *>()( p.GetPtr() );
	}
};

// misuse guard
template <class T>
inline bool IsValid( CObj<T> *p )
{
	return p->YouHaveMadeMistake();
}

template <class T>
inline bool IsValid( CPtr<T> *p )
{
	return p->YouHaveMadeMistake();
}

template <class T>
inline bool IsValid( CMObj<T> *p )
{
	return p->YouHaveMadeMistake();
}

// functor for STL tests
struct SPtrTest
{
	CObjectBase *pTest = nullptr;

	SPtrTest( CObjectBase *_pTest )
		: pTest( _pTest )
	{
	}

	template <class T, class T1>
	bool operator()( const CPtrBase<T, T1> &a ) const
	{
		return a == pTest;
	}
};

struct SPtrHash
{
	template <class T, class T1>
	std::size_t operator()( const CPtrBase<T, T1> &a ) const
	{
		return std::hash<const void *>()( a.GetBarePtr() );
	}
};

// walks container of pointers and erases references on invalid entries
template <class TContainer>
inline bool EraseInvalidRefs( TContainer *pData )
{
	bool bRes = false;
	for ( typename TContainer::iterator i = pData->begin(); i != pData->end(); )
	{
		if ( IsValid( *i ) )
		{
			++i;
		}
		else
		{
			i = pData->erase( i );
			bRes = true;
		}
	}
	return bRes;
}

// class for convinient handling of framework`s contexts
class CFWContext
{
	void **pFrameworkPtr = nullptr;

public:
	template <class T>
	CFWContext( T **pF, T *pData )
		: pFrameworkPtr( (void **)pF )
	{
		*pF = pData;
	}

	~CFWContext()
	{
		*pFrameworkPtr = 0;
	}
};

// assumes base class is CObjectBase
template <class T>
class CDynamicCast
{
	T *ptr = nullptr;

public:
	template <class TT>
	CDynamicCast( TT *_ptr )
	{
		ptr = dynamic_cast<T *>( CastToObjectBase( _ptr ) );
	}

	template <class TT>
	CDynamicCast( const TT *_ptr )
	{
		ptr = dynamic_cast<T *>( CastToObjectBase( const_cast<TT *>( _ptr ) ) );
	}

	template <class T1, class T2>
	CDynamicCast( const CPtrBase<T1, T2> &_ptr )
	{
		ptr = dynamic_cast<T *>( _ptr.GetBarePtr() );
	}

	operator T *() const
	{
		return ptr;
	}

	T *operator->() const
	{
		return ptr;
	}

	T *GetPtr() const
	{
		return ptr;
	}
};

template <class T>
inline bool IsValid( const CDynamicCast<T> &p )
{
	return IsValid( p.GetPtr() );
}
