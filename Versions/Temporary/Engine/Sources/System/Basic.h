#include "System_export.h"
#pragma once
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

#pragma warning(disable:4250)

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
#if defined(_DEBUG) && !defined(FAST_DEBUG)
	void AddRef() { if ( nRefData == 0 ) strncpy( szObjectName, GetTypeName(), 12 ); ++nRefData; }
	void AddObj( int nRef ) { if ( nObjData == 0 ) strncpy( szObjectName, GetTypeName(), 12 ); nObjData += nRef; }
#else
	void AddRef() { ++nRefData; }
	void AddObj( int nRef ) { nObjData += nRef; }
#endif
	void DecRef() { --nRefData; }
	void DecObj( int nRef ) { nObjData -= nRef; }
	void ReleaseRef();
	void ReleaseObj( int nRef, int nMask );
	static void DestroyDelayed();
protected:
#if defined(_DEBUG) && !defined(FAST_DEBUG)
	char szObjectName[12];
#endif
	int nObjData;
	int nRefData;
	// function should clear contents of object, easy to implement via consequent calls to
	// destructor and constructor, this function should not be called directly, use Clear()
	virtual void DestroyContents() = 0;
	virtual CObjectBase* MakeCopy() const { ASSERT( 0 ); return 0; }
	virtual ~CObjectBase()
	{
#ifdef TRACK_OBJECTS_STATISTICS
		UnRegisterInObjectsSet( this );
#endif
	}
	inline void CopyValidFlag( const CObjectBase &a ) { nObjData &= 0x7fffffff; nObjData |= a.nObjData & 0x80000000; }
public:
	CObjectBase() : nObjData(0), nRefData(0)
	{
#ifdef TRACK_OBJECTS_STATISTICS
		RegisterInObjectsSet( this );
#endif
	}
	// do not copy refcount when copy object
	CObjectBase( const CObjectBase &a ) : nObjData(0), nRefData(0)
	{
		CopyValidFlag( a );
#ifdef TRACK_OBJECTS_STATISTICS
		RegisterInObjectsSet( this );
#endif
	}
	CObjectBase& operator=( const CObjectBase &a ) { CopyValidFlag( a ); return *this; }
	//
	int IsRefInvalid() const { return (nObjData & 0x80000000); }
	int IsRefValid() const { return !IsRefInvalid(); }
	// do not use
	void SetInvalid( bool b ) { nObjData &= 0x7fffffff; if ( b ) nObjData |= 0x80000000; }
	// reset data in class to default values, saves RefCount from destruction
	void Clear() { AddRef(); DestroyContents(); DecRef(); }
	//
	virtual int operator&( struct IBinSaver &f ) { return 0; }
	virtual DWORD CalcCheckSum() const { return 0; }
	const char* GetTypeName() const;
	const char* GetFullTypeName() const;
	virtual int GetSizeOf() const = 0;
	//
	// due to absense of template friend classes in vc these classes are needed
	struct SRefO
	{
		void AddRef( CObjectBase *pObj ) {  pObj->AddObj( 1 ); }
		void DecRef( CObjectBase *pObj ) {  pObj->DecObj( 1 ); }
		void Release( CObjectBase *pObj ) { pObj->ReleaseObj( 1, 0x000fffff ); }
	};
	struct SRefM
	{
		void AddRef( CObjectBase *pObj ) {  pObj->AddObj( 0x100000 ); }
		void DecRef( CObjectBase *pObj ) {  pObj->DecObj( 0x100000 ); }
		void Release( CObjectBase *pObj ) { pObj->ReleaseObj( 0x100000,0x7ff00000 ); }
	};
	struct SRef
	{
		void AddRef( CObjectBase *pObj ) {  pObj->AddRef(); }
		void DecRef( CObjectBase *pObj ) {  pObj->DecRef(); }
		void Release( CObjectBase *pObj ) { pObj->ReleaseRef(); }
	};
	friend struct CObjectBase::SRef;
	friend struct CObjectBase::SRefO;
	friend struct CObjectBase::SRefM;
};


// macro that helps to make CFundament derivative to be serializable
// and makes sure that class will not be copied or destroyed via obj references

// macro that helps to create neccessary members for proper operation of refcount system
// if class needs special destructor, use CFundament
#define OBJECT_BASIC_METHODS(classname)                                              \
	public:                                                                            \
		static CObjectBase* New##classname() { return new classname(); }                 \
		classname* Duplicate() const { return dynamic_cast<classname*>(MakeCopy()); }    \
		virtual int GetSizeOf() const { return sizeof(classname); }						\
protected:                                                                           \
		CObjectBase* MakeCopy() const { return new classname(*this); }                   \
		virtual void DestroyContents() { classname::~classname(); int nHoldRefs = nRefData, nHoldObjs = nObjData; new(this) classname(); nRefData += nHoldRefs; nObjData += nHoldObjs; }\
		virtual ~classname() {}                                                          \
	private:
#define OBJECT_NOCOPY_METHODS(classname)                                             \
	public:                                                                            \
		static CObjectBase* New##classname() { return new classname(); }                 \
		virtual int GetSizeOf() const { return sizeof(classname); }						\
	protected:                                                                         \
	virtual void DestroyContents() { classname::~classname(); int nHoldRefs = nRefData, nHoldObjs = nObjData; new(this) classname(); nRefData += nHoldRefs; nObjData += nHoldObjs; }\
	private:
#define BASIC_REGISTER_CLASS(classname) \
template<> __declspec(dllexport) CObjectBase* CastToObjectBaseImpl<classname >( classname *p, void* ) { return p; }  \
template<> __declspec(dllexport) classname* CastToUserObjectImpl<classname >( CObjectBase *p, classname*, void* ) { return dynamic_cast<classname*>( p ); }

template<class TUserObj> CObjectBase* CastToObjectBaseImpl( TUserObj *p, void* );
template<class TUserObj> CObjectBase* CastToObjectBaseImpl( TUserObj *p, CObjectBase* ) { return p; }
template<class TUserObj> TUserObj* CastToUserObjectImpl( CObjectBase *p, TUserObj*, void * );
template<class TUserObj> TUserObj* CastToUserObjectImpl( CObjectBase *p, TUserObj*, CObjectBase* ) { return dynamic_cast<TUserObj*>( p ); }
template<class TUserObj> inline CObjectBase* CastToObjectBase( TUserObj *p ) { return CastToObjectBaseImpl( p, p ); }
template<class TUserObj> inline TUserObj* CastToUserObject( CObjectBase *p, TUserObj *pu ) { return CastToUserObjectImpl( p, pu, pu ); }

// TObject - base object for reference counting, TUserObj - user object name
// TRef - struct with AddRef/DecRef/Release methods for refcounting to use
template< class TUserObj, class TRef>
class CPtrBase
{
private:
	TUserObj *ptr;
	//
	void AddRef( TUserObj *_ptr ) { TRef p; if ( _ptr ) p.AddRef( CastToObjectBase(_ptr) ); }
	void DecRef( TUserObj *_ptr ) { TRef p; if ( _ptr ) p.DecRef( CastToObjectBase(_ptr) ); }
	void Release( TUserObj *_ptr ) { TRef p; if ( _ptr ) p.Release( CastToObjectBase(_ptr) ); }
protected:
	void SetObject( TUserObj *_ptr ) { TUserObj *pOld = ptr; ptr = _ptr; AddRef( ptr ); Release( pOld ); }
	TUserObj* Get() const { return ptr; }
public:
	CPtrBase(): ptr( 0 ) {}
	CPtrBase( TUserObj *_ptr ): ptr( _ptr ) { AddRef( ptr ); }
	CPtrBase( const CPtrBase &a ): ptr( a.ptr ) { AddRef( ptr ); }
	~CPtrBase() { Release( ptr ); }
	//
	void Set( TUserObj *_ptr ) { SetObject( _ptr ); }
	TUserObj* Extract() { TUserObj *pRes = ptr; DecRef(ptr); ptr = 0; return pRes; }
	//
	// assignment operators
	CPtrBase& operator=( TUserObj *_ptr ) { Set( _ptr ); return *this; }
	CPtrBase& operator=( const CPtrBase &a ) { Set( a.ptr ); return *this; }
	// access
	TUserObj* operator->() const { return ptr; }
	operator TUserObj*() const { return ptr; }
	TUserObj* GetPtr() const { return ptr; }
	CObjectBase* GetBarePtr() const { return CastToObjectBase(ptr); }
	int operator&( IBinSaver &f );
};

template<class T> inline bool IsValid( T *p ) { return p != 0 && !CastToObjectBase(p)->IsRefInvalid(); }
template<class T, class TRef> inline bool IsValid( const CPtrBase< T, TRef > &p ) { return p.GetPtr() && !p.GetBarePtr()->IsRefInvalid(); }

#define BASIC_PTR_DECLARE( TPtrName, TRef )                                          \
template<class T>                                                                    \
class TPtrName: public CPtrBase< T, TRef >                                           \
{                                                                                    \
	typedef CPtrBase< T, TRef > CBase;                                                 \
public:                                                                              \
	typedef T CDestType;                                                               \
	TPtrName() {}                                                                      \
	TPtrName( T *_ptr ): CBase( _ptr ) {}                                              \
	TPtrName( const TPtrName &a ): CBase( a ) {}                                       \
	TPtrName& operator=( T *_ptr ) { CBase::Set( _ptr ); return *this; }                      \
	TPtrName& operator=( const TPtrName &a ) { CBase::SetObject( a.Get() ); return *this; }   \
	int operator&( IBinSaver &f ) { return (*(CBase*)this) & (f); }                    \
};
//
BASIC_PTR_DECLARE( CPtr, CObjectBase::SRef );
BASIC_PTR_DECLARE( CObj, CObjectBase::SRefO );
BASIC_PTR_DECLARE( CMObj, CObjectBase::SRefM );
// misuse guard
template<class T> inline bool IsValid( CObj<T> *p ) { return p->YouHaveMadeMistake(); }
template<class T> inline bool IsValid( CPtr<T> *p ) { return p->YouHaveMadeMistake(); }
template<class T> inline bool IsValid( CMObj<T> *p ) { return p->YouHaveMadeMistake(); }

// functor for STL tests
struct SPtrTest
{
	CObjectBase *pTest;
	SPtrTest( CObjectBase *_pTest ): pTest(_pTest) {}
	template <class T,class T1> 
		bool operator()( const CPtrBase<T,T1> &a ) const { return a == pTest; }
};

struct SPtrHash
{
	template <class T,class T1> 
		int operator()( const CPtrBase<T,T1> &a ) const { return (int)a.GetBarePtr(); }
};

// walks container of pointers and erases references on invalid entries
template<class TContainer>
inline bool EraseInvalidRefs( TContainer *pData )
{
	bool bRes = false;
	for ( typename TContainer::iterator i = pData->begin(); i != pData->end(); )
	{
		if ( IsValid( *i ) )
			++i;
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
	void **pFrameworkPtr;
public:
	template<class T>
		CFWContext( T **pF, T *pData ): pFrameworkPtr((void**)pF) { *pF = pData; }
	~CFWContext() { *pFrameworkPtr = 0; }
};

// assumes base class is CObjectBase
template<class T>
class CDynamicCast
{
	T *ptr;
public:
	template<class TT>
		CDynamicCast( TT *_ptr ) { ptr = dynamic_cast<T*>( CastToObjectBase( _ptr ) ); }
	template<class TT>
		CDynamicCast( const TT *_ptr ) { ptr = dynamic_cast<T*>( CastToObjectBase( const_cast<TT*>( _ptr ) ) ); }
	template<class T1, class T2>
		CDynamicCast( const CPtrBase<T1,T2> &_ptr ) { ptr = dynamic_cast<T*>( _ptr.GetBarePtr() ); }
	operator T*() const { return ptr; }
	T* operator->() const { return ptr; }
	T* GetPtr() const { return ptr; }
};
template <class T>
inline bool IsValid( const CDynamicCast<T> &p ) { return IsValid( p.GetPtr() ); }

