#pragma once

#include "XmlResource.h"

#include "System_export.h"

#include <cstdint>

#include <boost/config.hpp>

// database object identification
class CDBID
{
	ZDATA
		std::string szKeyName;
		ZSKIP;
		uint32_t dwHashKey;
public:
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szKeyName); f.Add(4,&dwHashKey); OnSerialize( f ); return 0; }
private:
	//
	void OnSerialize( IBinSaver &saver )
	{
		if ( saver.IsReading() && dwHashKey == 0 )
			dwHashKey = MakeHashKey();
	}
	//
	BOOST_FORCEINLINE char ConvertChar( const char chr ) const
	{
		const char temp = chr - '\\';
		const char mask = (temp >> 7) | ((-temp) >> 7);
		const char chr1 = (chr & mask) | ('/' & (~mask));
		return chr1 - ( ('A' - 'a') & ( (('A' - chr1 - 1) & (chr1 - 'Z' - 1)) >> 7 ) );
	}

	inline bool CompareEqual( const std::string &szOtherKeyName ) const
	{
		const int nSize = szOtherKeyName.size();
		if ( nSize != szKeyName.size() )
			return false;
		// compare from the tail - where we can have more differences
		for ( int i = nSize - 1; i >= 0; --i )
		{
			if ( ConvertChar( szOtherKeyName[i] ) != ConvertChar( szKeyName[i] ) )
				return false;
		}
		return true;
	}
	unsigned int MakeHashKey() const 
	{ 
		unsigned int __uHashKey = 0; 
		for ( std::string::const_iterator it = szKeyName.begin(); it != szKeyName.end(); ++it )
			__uHashKey = 5*__uHashKey + ConvertChar( *it );
		return __uHashKey;
	}
	//
public:
	CDBID(): dwHashKey(0) {}
	CDBID( const char *_pszKeyName ): szKeyName( _pszKeyName ), dwHashKey( MakeHashKey() ) {}
	CDBID( const std::string &_szKeyName ): szKeyName( _szKeyName ), dwHashKey( MakeHashKey() ) {}
	CDBID( const CDBID &dbid ): szKeyName( dbid.szKeyName ), dwHashKey(dbid.dwHashKey) {}
	//
	const CDBID &operator=( const CDBID &dbid )
	{ 
		if ( this != &dbid )
		{
			szKeyName = dbid.szKeyName;
			dwHashKey = dbid.dwHashKey;
		}
		return *this; 
	}
	//
	bool operator==( const CDBID &dbid ) const { return dwHashKey == dbid.dwHashKey ? CompareEqual( dbid.szKeyName ) : false; }
	bool operator!=( const CDBID &dbid ) const { return !( *this == dbid ); }
	bool operator <( const CDBID &dbid ) const 
	{ 
		const int nSize1 = szKeyName.size();
		const int nSize2 = dbid.szKeyName.size();
		const int nSize = (std::min)( nSize1, nSize2 );
		//
		for ( int i = 0; i < nSize; ++i )
		{
			const char c1 = ConvertChar( szKeyName[i] );
			const char c2 = ConvertChar( dbid.szKeyName[i] );
			if ( c1 < c2 )
				return true;
			if ( c2 < c1 )
				return false;
		}
		return nSize1 < nSize2;
	}
	//
	const std::string &ToString() const { return szKeyName; }
	unsigned int GetHashKey() const { return dwHashKey; }
	inline bool IsEmpty() const { return szKeyName.empty(); }
	inline void Clear() { szKeyName.clear(); dwHashKey = 0; }
};

template<> struct std::hash<CDBID>
{
	size_t operator()( const CDBID &dbid ) const { return dbid.GetHashKey(); }
};

namespace NDb
{
	enum EDBMode
	{
		DBMODE_GAMEDB,
		DBMODE_DATABASE
	};
	// base class for all GameDatabase resources
	class CResource : public CXmlResource
	{
		friend class CResourceSaver;
		friend class CResourceHelper;
		CDBID dbid;
		bool bLoaded;
		// disable assign operator
		CResource& operator=( const CResource &res ) = delete;
	protected:
		virtual void PostLoad( bool bInEditor ) {}
	public:
		CResource(): bLoaded(false) {}
		CResource( const CResource &res ): bLoaded(res.bLoaded) {}
		//
		bool IsLoaded() const { return bLoaded; }
		const CDBID &GetDBID() const { return dbid; }
		virtual int GetTypeID() const = 0;
		int GetRecordID() const { return -1; }
		//
		virtual uint32_t CalcCheckSum() const { return 0; }
	};
	//
	// database functions
	//
	//game database iterator
	struct IDBIterator : public CObjectBase
	{
		virtual bool MoveNext() = 0;
		virtual bool IsEnd() const = 0;
		virtual CResource *Get() const = 0;
		virtual void MoveFirst() = 0;
	};
	//
	SYSTEM_EXPORT IDBIterator *CreateDBIterator( int nTypeID );
	// add database resource file
	void AddResources( const std::string &szFile );
	// finish adding - resolve all references
	void FinishAddResources();
	// clear database resources
	void RemoveAllResources();
	// get specific entry from DB
	// conflict with #define GetObject GetObjectA (Windows SDK)
	SYSTEM_EXPORT CResource *(GetObject)( const CDBID &dbid );
	//
	void SetDBMode( EDBMode eMode );
	inline const std::string &GetFileName( const CDBID &dbid ) { return dbid.ToString(); }
	inline std::string GetFolderName( const CDBID &dbid )
	{
		const std::string &szFileName = NDb::GetFileName( dbid );
		int nPos = szFileName.size();
		while ( --nPos >= 0 )
		{
			if ( szFileName[nPos] == '\\' || szFileName[nPos] == '/' )
				break;
		}
		return szFileName.substr( 0, nPos );
	}
	inline const char *GetResName( const CResource *pRes ) { return pRes->GetDBID().ToString().c_str(); }
};

#define BASIC_REGISTER_DATABASE_CLASS(module, classname) \
BASIC_REGISTER_CLASS(module, classname)	\
template<> module##_EXPORT const NDb::CResource* CastToDBResourceImpl<classname >( const classname *p, const void* ) { return p; }  \
template<> module##_EXPORT const classname* CastToDBUserObjectImpl<classname >( const NDb::CResource *p, const classname*, const void* ) { return dynamic_cast<const classname*>( p ); }

template<class TUserObj> SYSTEM_EXPORT const NDb::CResource* CastToDBResourceImpl( const TUserObj *p, const void* );
template<class TUserObj> const NDb::CResource* CastToDBResourceImpl( const TUserObj *p, const NDb::CResource* ) { return p; }
template<class TUserObj> const TUserObj* CastToDBUserObjectImpl( const NDb::CResource *p, const TUserObj*, const void * );
template<class TUserObj> const TUserObj* CastToDBUserObjectImpl( const NDb::CResource *p, const TUserObj*, const NDb::CResource* ) { return dynamic_cast<const TUserObj*>( p ); }
template<class TUserObj> inline const NDb::CResource* CastToDBResource( const TUserObj *p ) { return CastToDBResourceImpl( p, p ); }
template<class TUserObj> inline const TUserObj* CastToDBUserObject( const NDb::CResource *p, const TUserObj *pu ) { return CastToDBUserObjectImpl( p, pu, pu ); }

// NDb::Get helper functions series
namespace NDb
{
	template <class T> inline const T *Get( const CDBID &dbid ) { return CastToDBUserObject( GetObject(dbid), (const T *)0 ); }
}
// ************************************************************************************************************************ //
// **
// ** database ptr
// **
// **
// **
// ************************************************************************************************************************ //

template <class TUserObj, typename TPtr = CPtr<TUserObj> >
class CDBPtr
{
	TPtr pObj;
	mutable bool bLoaded;
	//
	BOOST_FORCEINLINE void LoadObject() const
	{
		if ( !bLoaded )
		{
			if ( pObj && !CastToDBResource(pObj.GetPtr())->IsLoaded() )
				NDb::GetObject( CastToDBResource(pObj.GetPtr())->GetDBID() );
			bLoaded = true;
		}
	}
public:
	CDBPtr() : pObj( 0 ), bLoaded( false ) {  }
	CDBPtr( const TUserObj *_pObj ) : pObj( const_cast<TUserObj*>(_pObj) ), bLoaded( false ) {  }
	CDBPtr( const CDBPtr<TUserObj, TPtr> &ptr ) : pObj( ptr.pObj ), bLoaded( ptr.bLoaded ) {  }
	// assignment operators
	const CDBPtr<TUserObj, TPtr>& operator=( const TUserObj *_pObj ) { pObj = const_cast<TUserObj*>( _pObj ); bLoaded = false; return *this; }
	const CDBPtr<TUserObj, TPtr>& operator=( const CDBPtr<TUserObj, TPtr> &ptr ) { pObj = ptr.pObj; bLoaded = ptr.bLoaded; return *this; }
	// object access operators (dereference and pointer access)
	operator const TUserObj*() const { LoadObject(); return pObj; }
	const TUserObj* operator->() const { LoadObject(); return pObj; }
	// comparison operators
	bool operator==( const CDBPtr<TUserObj, TPtr> &ptr ) const { return ( pObj == ptr.pObj ); }
	bool operator==( const TUserObj *_pObj ) const { return ( pObj == _pObj ); }
	bool operator!=( const CDBPtr<TUserObj, TPtr> &ptr ) const { return ( pObj != ptr.pObj ); }
	bool operator!=( const TUserObj *_pObj ) const { return ( pObj != _pObj ); }
	// check for empty object
	bool IsEmpty() const { return ( pObj == 0 ); }
	// direct access to the object... ugly functions, but it is necessary,
	// because C++ can't cast from smartptr to polymorphics of the stored data
	const TUserObj* GetPtr() const { LoadObject(); return pObj; }
	const NDb::CResource* GetBarePtr() const { LoadObject(); return CastToDBResource( pObj.GetPtr() ); }
	// special functions to access pointer w/o loading data
	const TUserObj* GetPtrNoLoad() const { return pObj; }
	const NDb::CResource* GetBarePtrNoLoad() const { return CastToDBResource( pObj.GetPtr() ); }
	//
	int operator&( IBinSaver &saver )
	{
		if ( saver.IsReading() )
		{
			std::string szString;
			saver.Add( 3, &szString );
			if ( !szString.empty() )	// new DBPtr
			{
				if ( szString == "empty object placeholder" )
					pObj = 0;
				else
					pObj = const_cast<TUserObj*>( CastToDBUserObject( NDb::GetObject(CDBID(szString)), (TUserObj*)0 ) );
			}
			else
				pObj = 0;
		}
		else
		{
			// always save new DBPtr
			std::string szString;
			if ( pObj )
				szString = CastToDBResource(pObj.GetPtr())->GetDBID().ToString();
			saver.Add( 3, &szString );
		}
		return 0;
	}

	uint32_t CalcCheckSum() const;
};

struct SDBPtrHash
{
	template <class T>
		std::size_t operator()( const T &a ) const { return a.GetPtr() ? a.GetPtr()->GetDBID().GetHashKey() : 0; }
};

template<typename T> struct std::hash<CDBPtr<T>> {
	std::size_t operator()(const CDBPtr<T> &p) const { return std::hash<const T*>()(p.GetPtr()); }
};

#define REGISTER_DATABASE_CLASS( module, N, name )  \
	BASIC_REGISTER_DATABASE_CLASS( module, name ) \
	static struct name##Register##N { name##Register##N() {  \
	REGISTER_CLASS( N, name )			\
	CPtr<name> pTemp = new name;	\
	pTemp->ReportMetaInfo();			\
	} } init##name##N;


