#pragma once

#include "System_export.h"

#define REGISTER_SAVELOAD_CLASS( N, name )  \
	BASIC_REGISTER_CLASS( name ) \
	static struct name##Register##N { \
		name##Register##N() { REGISTER_CLASS( N, name )	}	\
	} init##name##N;
#define REGISTER_SAVELOAD_TEMPL_CLASS( N, name, className )  \
	BASIC_REGISTER_CLASS( name ) \
	static struct className##Register##N { \
		className##Register##N() { REGISTER_TEMPL_CLASS( N, name, className ) }	\
	} init##className##N;
#define REGISTER_SAVELOAD_CLASS_NM( N, name, nmspace )  \
	BASIC_REGISTER_CLASS( nmspace::name ) \
	static struct name##Register##N { \
		name##Register##N() { REGISTER_CLASS_NM( N, name, nmspace )	}	\
	} init##name##N;
#define START_REGISTER(a) void StartRegisterHook##a() {} static struct a##Init { a##Init () {
#define FINISH_REGISTER } } init;

#define ZDATA_(a)
#define ZDATA
#define ZPARENT(a)
#define ZEND
#define ZSKIP
#define ZONSERIALIZE

//#define CHECK_VALID_SERIALIZED_POINTERS

template <class T> class CArray2D;

enum ESaverMode
{
	// always reads/writes pointers as 32-bit (4 bytes)
	SAVER_MODE_READ		= 1,
	SAVER_MODE_WRITE	= 2,
	SAVER_MODE_WRITE_COMPRESSED = 3,
	// on x64, reads/writes pointers as 64-bit (8 bytes)
	SAVER_MODE_READ_64 = 4,
	SAVER_MODE_WRITE_64 = 5,
	SAVER_MODE_WRITE_COMPRESSED_64 = 6,
};

// ************************************************************************************************************************ //
// **
// ** binary saver
// **
// **
// **
// ************************************************************************************************************************ //

struct IBinSaver : public CObjectBase
{
public:
	typedef unsigned char chunk_id;
	typedef std::string stdString;
	typedef std::wstring stdWString;
private:
	char __cdecl TestDataPath(...) { return 0; }
	int __cdecl TestDataPath( stdString* ) { return 0; }
	int __cdecl TestDataPath( stdWString* ) { return 0; }
	int __cdecl TestDataPath( CMemoryStream* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( CArray2D<T1>* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( std::vector<T1>* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( std::list<T1>* ) { return 0; }
	template<class T1, class T2, class T3>
		int __cdecl TestDataPath( std::unordered_map<T1,T2,T3>* ) { return 0; }
	template<class T1, class T2>
		int __cdecl TestDataPath( std::unordered_set<T1,T2>* ) { return 0; }
	template<class T1, class T2>
		int __cdecl TestDataPath( std::set<T1,T2>* ) { return 0; }
	template<class T1, class T2>
		int __cdecl TestDataPath( std::pair<T1,T2>* ) { return 0; }
	//
	template<class T>
	void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, ... )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		p->T::operator&( *this );
		FinishChunk();
	}
	template<class T>
	void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, SInt2Type<1> *pp )
	{
		DataChunk( idChunk, p, sizeof(T), nChunkNumber );
	}
	//
	// vector
	template <class T> void DoVector( std::vector<T> &data )
	{
		int i, nSize;
		if ( IsReading() )
		{
			data.clear();
			if ( GetVersion() < 2 )
				nSize = CountChunks( 1 );
			else
				Add( 2, &nSize );
			data.resize( nSize );
		}
		else
		{
			nSize = data.size();
			Add( 2, &nSize );
		}
		for ( i = 0; i < nSize; i++ )
			Add( 1, &data[i], i + 1 );
	}
	template <class T> void DoDataVector( std::vector<T> &data )
	{
		int nSize = data.size();
		Add( 1, &nSize );
		if ( IsReading() )
		{
			data.clear();
			data.resize( nSize );
		}
		if ( nSize > 0 )
			DataChunk( 2, &data[0], sizeof(T) * nSize, 1 );
	}
	// hash_map
	template <class T1,class T2,class T3> 
		void DoHashMap( std::unordered_map<T1,T2,T3> &data )
	{
		if ( IsReading() )
		{
			data.clear();
			int nSize, i;
			if ( GetVersion() < 2 )
				nSize = CountChunks( 1 );
			else
				Add( 3, &nSize );
			if ( GetVersion() > 3 )
			{
				int nBuckets;
				Add( 4, &nBuckets );
				data.rehash( nBuckets );
			}
			std::vector<T1> indices;
			indices.resize( nSize );
			for ( i = 0; i < nSize; ++i )
				Add( 1, &indices[i], i + 1 );
			for ( i = 0; i < nSize; ++i )
				Add( 2, &data[ indices[i] ], i + 1 );
		}
		else
		{
			int nSize = data.size(), nBuckets = data.bucket_count();
			Add( 3, &nSize );
			Add( 4, &nBuckets );

			std::vector<T1> indices;
			indices.resize( nSize );
			int i = 1;
			for ( typename std::unordered_map<T1,T2,T3>::iterator pos = data.begin(); pos != data.end(); ++pos, ++i )
				indices[ nSize - i ] = pos->first;
			for ( i = 0; i < nSize; ++i )
				Add( 1, &indices[i], i + 1 );
			for ( i = 0; i < nSize; ++i )
				Add( 2, &data[ indices[i] ], i + 1 );
		}
	}

	template <class T1, class T2>
		void DoHashSet( std::unordered_set<T1, T2> &data )
	{
		if (IsReading())
		{
			std::vector<T1> vectorData;
			Add(1, &vectorData);

			data.clear();
			data.insert(vectorData.begin(), vectorData.end());
		}
		else
		{
			std::vector<T1> vectorData(data.begin(), data.end());
			Add(1, &vectorData);
		}
	}

	template <class T1,class T2> 
		void DoSet( std::set<T1,T2> &data )
	{
		if ( IsReading() )
		{
			std::vector<T1> vectorData;
			Add( 1, &vectorData );

			data.clear();
			data.insert( vectorData.begin(), vectorData.end() );
		}
		else
		{
			std::vector<T1> vectorData( data.begin(), data.end() );
			Add( 1, &vectorData );
		}
	}
	template<class T> void Do2DArray( CArray2D<T> &a )
	{
		int nXSize = a.GetSizeX(), nYSize = a.GetSizeY();
		Add( 1, &nXSize );
		Add( 2, &nYSize );
		if ( IsReading() )
			a.SetSizes( nXSize, nYSize );
		for ( int i = 0; i < nXSize * nYSize; i++ )
			Add( 3, &a[i/nXSize][i%nXSize], i + 1 );
	}
	template<class T> void Do2DArrayData( CArray2D<T> &a )
	{
		int nXSize = a.GetSizeX(), nYSize = a.GetSizeY();
		Add( 1, &nXSize );
		Add( 2, &nYSize );
		if ( IsReading() )
			a.SetSizes( nXSize, nYSize );
		if ( nXSize * nYSize > 0 )
			DataChunk( 3, &a[0][0], sizeof(T) * nXSize * nYSize, 1 );
	}
	virtual bool StartChunk( const chunk_id idChunk, int nChunkNumber ) = 0;
	virtual void FinishChunk() = 0;
	virtual int CountChunks( const chunk_id idChunk ) = 0;

	virtual void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) = 0;
	virtual void DataChunkString( std::string &data ) = 0;
	virtual void DataChunkString( std::wstring &data ) = 0;
	// storing/loading pointers to objects
	virtual void StoreObject( CObjectBase *pObject ) = 0;
	virtual CObjectBase* LoadObject() = 0;
public:
	virtual bool IsReading() = 0;
	virtual bool IsChecksum() { return false; }
	virtual int GetVersion() const = 0;
	//
	void AddRawData( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber = 1 ) { DataChunk( idChunk, pData, nSize, nChunkNumber ); }
	//
	template<class T>
		void Add( const chunk_id idChunk, T *p, int nChunkNumber = 1 ) 
	{
		const int N_HAS_SERIALIZE_TEST = sizeof( (*p)&(*this) );
		SInt2Type<N_HAS_SERIALIZE_TEST> separator;
		CallObjectSerialize( idChunk, nChunkNumber, p, &separator );
	}
	void Add( const chunk_id idChunk, stdString *pStr, int nChunkNumber = 1 ) 
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DataChunkString( *pStr );
		FinishChunk();
	}
	void Add( const chunk_id idChunk, stdWString *pStr, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DataChunkString( *pStr );
		FinishChunk();
	}
	void Add( const chunk_id idChunk, CMemoryStream *pStr, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		int nSize = pStr->GetSize();
		Add( 1, &nSize );
		if ( !StartChunk( 2, 1 ) )
		{
			FinishChunk();
			return;
		}
		if ( IsReading() )
			pStr->SetSize( nSize );
		DataChunk( 1, pStr->GetBufferForWrite(), nSize, 1 );
		FinishChunk();
		FinishChunk();
	}
	template<class T1>
		void Add( const chunk_id idChunk, std::vector<T1> *pVec, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		if ( sizeof( TestDataPath( &(*pVec)[0] ) ) == 1 && sizeof( (*pVec)[0]&(*this) ) == 1 )
			DoDataVector( *pVec );
		else
			DoVector( *pVec );
		FinishChunk();
	}
	template<class T1, class T2, class T3>
		void Add( const chunk_id idChunk, std::unordered_map<T1,T2,T3> *pHash, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DoHashMap( *pHash );
		FinishChunk();
	}
	template<class T1, class T2>
		void Add( const chunk_id idChunk, std::unordered_set<T1,T2> *pHash, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DoHashSet( *pHash );
		FinishChunk();
	}
#if 0
	template<class T1, class T2>
		void Add( const chunk_id idChunk, std::unordered_set<T1,T2> *pHash, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DoHashMap( *pHash );
		FinishChunk();
	}
#endif
	template<class T1, class T2>
		void Add( const chunk_id idChunk, std::set<T1,T2> *pSet, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		DoSet( *pSet );
		FinishChunk();
	}
	template<class T1>
		void Add( const chunk_id idChunk, CArray2D<T1> *pArr, int nChunkNumber = 1 ) 
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		if ( sizeof( TestDataPath( &(*pArr)[0][0] ) ) == 1 && sizeof( (*pArr)[0][0]&(*this) ) == 1 )
			Do2DArrayData( *pArr );
		else
			Do2DArray( *pArr );
		FinishChunk();
	}
	template<class T1>
		void Add( const chunk_id idChunk, std::list<T1> *pList, int nChunkNumber = 1 )
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		std::list<T1> &data = *pList;
		if ( IsReading() )
		{
			int nSize;
			if ( GetVersion() < 2 )
				nSize = CountChunks( 1 );
			else
				Add( 2, &nSize );
			data.clear();
			data.insert( data.begin(), nSize, T1() );
		}
		else
		{
			int nSize = data.size();
			Add( 2, &nSize );
		}
		int i = 1;
		for ( typename std::list<T1>::iterator k = data.begin(); k != data.end(); ++k, ++i )
			Add( 1, &(*k), i );
		FinishChunk();
	}
	template <class T1, class T2> 
		void Add( const chunk_id idChunk, std::pair<T1, T2> *pData, int nChunkNumber = 1 )
	{
		if ( !StartChunk(idChunk, nChunkNumber) )
			return;
		Add( 1, &( pData->first ) );
		Add( 2, &( pData->second ) );
		FinishChunk();
	}
	void AddPolymorphicBase( chunk_id idChunk, CObjectBase *pObject, int nChunkNumber = 1 ) 
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		(*pObject) & (*this);
		FinishChunk();
	}
	//
	template <class T1, class T2> 
		void DoPtr( CPtrBase<T1,T2> *pData ) 
	{
#ifdef CHECK_VALID_SERIALIZED_POINTERS
		if ( !IsReading() ) 
		{
			ASSERT( !pData->GetPtr() || IsValid(*pData) );
		}
#endif
		if ( IsReading() ) 
			pData->Set( CastToUserObject( LoadObject(), (T1*)0 ) ); 
		else 
			StoreObject( pData->GetBarePtr() );
	}
private:
	// Compile time error detection
	template <class T1, class T2, class T3>
	void Add( const chunk_id idChunk, typename std::unordered_map<T1, T2, T3>::iterator *pData, int nChunkNumber = 1 )
	{
		ASSERT( 0 && "Hash table iterator can't be serialized" );
	}
	template <class T>
		void Add( const chunk_id idChunk, T **pData, int nChunkNumber = 1 ) 
	{
		ASSERT( 0 && "Pointer or vector iterator can't be serialized" );
	}

	template<class T1, class T2>//, class _Traits>
		void Add( const chunk_id idChunk, typename std::list<T1, T2>::iterator *pData, int nChunkNumber = 1 )
	{
		ASSERT( 0 && "List iterator can't be serialized" );
	}
};

template <class T>
inline char operator&( T &c, IBinSaver &ss ) { return 0; }
// realisation of forward declared serialisation operator
template< class TUserObj, class TRef>
int CPtrBase<TUserObj,TRef>::operator&( IBinSaver &f )
{
	f.DoPtr( this );
	return 0;
}

// specify object that can be referenced but should not be stored to save
// nID is used to restore pointers on such objects after load
struct SBinSaverExternalObject
{
	int nID;
	CObjectBase *pObj;

	SBinSaverExternalObject() {}
	SBinSaverExternalObject( int _nID, CObjectBase *_pObj ) : nID(_nID), pObj(_pObj) {}
};

// objects checker in debug mode
struct IDebugSaveCheckObj : public CObjectBase
{
	virtual bool CheckObj( CObjectBase *pObj ) = 0;
};

class CFileStream;
SYSTEM_EXPORT IBinSaver *CreateBinSaver( CDataStream *pStream, ESaverMode mode, const std::vector<SBinSaverExternalObject> &ext );
SYSTEM_EXPORT IBinSaver *CreateBinSaverWithCheckers( CDataStream *pStream, const std::vector<SBinSaverExternalObject> &external, std::vector< CPtr<IDebugSaveCheckObj> > &objToCheck );

inline IBinSaver *CreateBinSaver( CDataStream *pStream, ESaverMode mode )
{
	return CreateBinSaver( pStream, mode, std::vector<SBinSaverExternalObject>() );
}

