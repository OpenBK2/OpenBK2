#pragma once

#include "System_export.h"

#include "BinSaver.h"
#include "XmlResource.h"

template <class T> class CArray2D;
template <class TUserObj, typename TPtr> class CDBPtr;
class CDBID;
namespace NFile
{
	class CFilePath;
}

// ************************************************************************************************************************ //
// **
// ** XML saver
// **
// **
// **
// ************************************************************************************************************************ //

interface IXmlSaver : public CObjectBase
{
	typedef const char* chunk_id;
private:
	char __cdecl TestDataPath(...) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( CArray2D<T1>* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( basic_string<T1>* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( vector<T1>* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( list<T1>* ) { return 0; }
	template<class T1, class T2, class T3>
		int __cdecl TestDataPath( hash_map<T1, T2, T3>* ) { return 0; }
	// add boolean built-in type
	template <class TYPE>
		void AddBoolData( chunk_id idChunk, TYPE *pData, int nChunkNumber ) 
		{ 
			bool bData = *pData;
			const bool bRetVal = DataChunk( idChunk, &bData, nChunkNumber );
			if ( IsReading() && bRetVal ) 
				*pData = TYPE( bData );
		}
	// add integer built-in types
	template <class TYPE>
		void AddIntData( chunk_id idChunk, TYPE *pData, int nChunkNumber ) 
		{ 
			int nData = *pData;
			const bool bRetVal = DataChunk( idChunk, &nData, nChunkNumber );
			if ( IsReading() && bRetVal ) 
				*pData = TYPE( nData );
		}
	// add fp built-in types
	template <class TYPE>
		void AddFPData( chunk_id idChunk, TYPE *pData, int nChunkNumber ) 
		{ 
			float fData = *pData;
			const bool bRetVal = DataChunk( idChunk, &fData, nChunkNumber );
			if ( IsReading() && bRetVal ) 
				*pData = TYPE( fData );
		}
	//
	template<class T>
		void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, ... )
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			p->operator&( *this );
			FinishChunk();
		}
	// enum or raw object?
	template<class T>
		void __cdecl CallRawObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, SInt2Type<1> *pp )
		{
			if ( StartChunk(idChunk, nChunkNumber) != false )
			{
				string szValue;
				if ( IsReading() )
				{
					DataChunkString( szValue );
					*p = SKnownEnum<T>::ToEnum( szValue );
				}
				else
				{
					szValue = SKnownEnum<T>::ToString( *p );
					DataChunkString( szValue );
				}
				FinishChunk();
			}
		}
	template<class T>
		void __cdecl CallRawObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, SInt2Type<0> *pp )
		{
			DataChunk( idChunk, p, sizeof(T), nChunkNumber );
		}
	template<class T>
		void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, SInt2Type<1> *pp )
		{
			const int N_KNOWN_ENUM = SKnownEnum<T>::isKnown;
			SInt2Type<N_KNOWN_ENUM> separator;
			CallRawObjectSerialize( idChunk, nChunkNumber, p, &separator );
		}
	// simple built-in data specialization
	template <> 
		void __cdecl CallObjectSerialize<GUID>( const chunk_id idChunk, int nChunkNumber, GUID *pData, SInt2Type<1> *pp )
		{
			DataChunk( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<bool>( const chunk_id idChunk, int nChunkNumber, bool *pData, SInt2Type<1> *pp )
		{
			AddBoolData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<char>( const chunk_id idChunk, int nChunkNumber, char *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<signed char>( const chunk_id idChunk, int nChunkNumber, signed char *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<unsigned char>( const chunk_id idChunk, int nChunkNumber, unsigned char *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<short>( const chunk_id idChunk, int nChunkNumber, short *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<unsigned short>( const chunk_id idChunk, int nChunkNumber, unsigned short *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<long>( const chunk_id idChunk, int nChunkNumber, long *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<unsigned long>( const chunk_id idChunk, int nChunkNumber, unsigned long *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<int>( const chunk_id idChunk, int nChunkNumber, int *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<unsigned int>( const chunk_id idChunk, int nChunkNumber, unsigned int *pData, SInt2Type<1> *pp )
		{
			AddIntData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<float>( const chunk_id idChunk, int nChunkNumber, float *pData, SInt2Type<1> *pp )
		{
			AddFPData( idChunk, pData, nChunkNumber );
		}
	template <> 
		void __cdecl CallObjectSerialize<double>( const chunk_id idChunk, int nChunkNumber, double *pData, SInt2Type<1> *pp )
		{
			AddFPData( idChunk, pData, nChunkNumber );
		}
	//
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, ... )
		{
			const int N_HAS_SERIALIZE_TEST = sizeof( (*p) & (*this) );
			SInt2Type<N_HAS_SERIALIZE_TEST> separator;
			CallObjectSerialize( idChunk, nChunkNumber, p, &separator );
		}
	// file path reference (must be before string due to it is derived from it)
	template <class T> 
		void AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, NFile::CFilePath *pData ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DataChunkFilePath( pData );
			FinishChunk();
		}
	template <class T, class T1>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, basic_string<T1> *pStr ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			DataChunkString( *pStr );
			FinishChunk();
		}
	template<class T,class T1>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CArray2D<T1> *pArr ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			if ( sizeof( TestDataPath(&(*pArr)[0][0]) ) == 1 && sizeof( (*pArr)[0][0]&(*this) ) == 1 )
				Do2DArrayData( *pArr );
			else
				Do2DArray( *pArr );
			FinishChunk();
		}
	template <class T, class T1>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, vector<T1> *pVec ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			/*
			if ( sizeof( TestDataPath(&(*pVec)[0]) ) == 1 && sizeof( (*pVec)[0]&(*this) ) == 1 )
				DoDataVector( *p );
			else
			*/
				DoVector( *p );
			FinishChunk();
		}
	template <class T, class T1>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, list<T1> *pList ) 
	{
		if ( !StartChunk(idChunk, nChunkNumber) )
			return;
		list<T1> &data = *pList;
		if ( IsReading() )
		{
			pList->clear();
			pList->insert( data.begin(), CountChunks(), T1() );
		}
		int i = 1;
		for ( list<T1>::iterator it = data.begin(); it != data.end(); ++it, ++i )
			Add( "Item", &(*it), i );
		FinishChunk();
	}
	template<class T,class T1, class T2, class T3>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, hash_map<T1,T2,T3> *pHash ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			DoHashMap( *pHash );
			FinishChunk();
		}
	template <class T, class T1, class T2> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, pair<T1, T2> *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "First", &( pData->first ) );
			Add( "Second", &( pData->second ) );
			FinishChunk();
		}
	template <class T> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CVec2 *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "x", &( pData->x ) );
			Add( "y", &( pData->y ) );
			FinishChunk();
		}
	template <class T> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CVec3 *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "x", &( pData->x ) );
			Add( "y", &( pData->y ) );
			Add( "z", &( pData->z ) );
			FinishChunk();
		}
	template <class T> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CVec4 *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "x", &( pData->x ) );
			Add( "y", &( pData->y ) );
			Add( "z", &( pData->z ) );
			Add( "w", &( pData->w ) );
			FinishChunk();
		}
	template <class T> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CQuat *pData ) 
		{
			AddInternal( idChunk, nChunkNumber, p, const_cast<CVec4*>( &pData->GetInternalVector() ) );
		}
	template <class T, class T1> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CTRect<T1> *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "x1", &( pData->x1 ) );
			Add( "y1", &( pData->y1 ) );
			Add( "x2", &( pData->x2 ) );
			Add( "y2", &( pData->y2 ) );
			FinishChunk();
		}
	template <class T, class T1> 
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CTPoint<T1> *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			Add( "x", &( pData->x ) );
			Add( "y", &( pData->y ) );
			FinishChunk();
		}
	// smart pointers specialization
	// general smart ptr/obj
	template <class T, class T1, class T2> 
		void AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CPtrBase<T1, T2> *pData ) 
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			//
			if ( IsReading() ) 
				pData->Set( CastToUserObject( LoadObject(), (T1*)0 ) ); 
			else 
				StoreObject( pData->GetBarePtr() );
			//
			FinishChunk();
		}
	// database ptr
	template <class T, class T1, class T2> 
		void AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CDBPtr<T1, T2> *pData ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			if ( IsReading() ) 
			{
				CDBID dbid;
				DataChunkDBID( &dbid );
				*pData = CastToDBUserObject( NDb::GetObject(dbid), (T1*)0 );
			}
			else
			{
				CDBID dbid;
				if ( *pData )
					dbid = CastToDBResource(pData->GetPtr())->GetDBID();
				DataChunkDBID( &dbid );
			}
			FinishChunk();
		}
	// database DBID
	template <class T> 
		void AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CDBID *pData ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DataChunkDBID( pData );
			FinishChunk();
		}
	// vector
	template <class T> 
		void DoVector( vector<T> &data )
		{
			int i, nSize;
			if ( IsReading() )
			{
				data.clear();
				data.resize( nSize = CountChunks() );
			}
			else
				nSize = data.size();
			for ( i = 0; i < nSize; ++i )
				Add( "Item", &data[i], i + 1 );
		}
	template <class T> 
		void DoDataVector( vector<T> &data )
		{
			int nSize = data.size();
			Add( "Size", &nSize );
			if ( IsReading() )
			{
				data.clear();
				data.resize( nSize );
			}
			if ( nSize > 0 )
				DataChunk( "Data", &data[0], sizeof(T) * nSize, 0 );
		}
	// hash_map
	template <class T1, class T2, class T3> 
		void DoHashMap( hash_map<T1, T2, T3> &data )
		{
			if ( IsReading() )
			{
				data.clear();
				const int nSize = CountChunks();
				vector<T1> indices;
				indices.resize( nSize );
				for ( int i = 0; i < nSize; ++i )
				{
					if ( StartChunk("Item", i + 1) ) 
					{
						Add( "Key", &indices[i] );
						Add( "Data", &data[ indices[i] ] );
						FinishChunk();
					}
				}
			}
			else
			{
				int i = 1;
				for ( hash_map<T1, T2, T3>::iterator pos = data.begin(); pos != data.end(); ++pos, ++i )
				{
					if ( StartChunk("Item", i) ) 
					{
						T1 idx = pos->first;
						Add( "Key", &idx );
						Add( "Data", &pos->second );
						FinishChunk();
					}
				}
			}
		}
	// 2D Array
	template <class T> 
		void Do2DArray( CArray2D<T> &a )
		{
			int nSizeX = a.GetSizeX(), nSizeY = a.GetSizeX();
			Add( "SizeX", &nSizeX );
			Add( "SizeY", &nSizeY );
			if ( StartChunk("Data", 0) )
			{
				if ( IsReading() )
					a.SetSizes( nSizeY, nSizeY );
				//
				for ( int i = 0; i < nSizeX * nSizeY; i++ )
					Add( "Item", &a[i/nSizeX][i%nSizeX], i + 1 );
				FinishChunk();
			}
		}
	template <class T> 
		void Do2DArrayData( CArray2D<T> &a )
		{
			int nSizeX = a.GetSizeX(), nSizeY = a.GetSizeX();
			Add( "SizeX", &nSizeX );
			Add( "SizeY", &nSizeY );
			if ( IsReading() )
				a.SetSizes( nSizeX, nSizeY );
			if ( nSizeX * nSizeY > 0 )
				DataChunk( "Data", &a[0][0], sizeof(T) * nSizeX * nSizeY, 0 );
		}
	virtual bool StartChunk( const chunk_id idChunk, int nChunkNumber ) = 0;
	virtual void FinishChunk() = 0;
	virtual int CountChunks() = 0;

	virtual bool DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) = 0;
	virtual bool DataChunk( const chunk_id idChunk, int *pnData, int nChunkNumber ) = 0;
	virtual bool DataChunk( const chunk_id idChunk, float *pfData, int nChunkNumber ) = 0;
	virtual bool DataChunk( const chunk_id idChunk, bool *pData, int nChunkNumber ) = 0;
	virtual bool DataChunk( const chunk_id idChunk, GUID *pgData, int nChunkNumber ) = 0;
	virtual bool DataChunkDBID( CDBID *pDBID ) = 0;
	virtual bool DataChunkFilePath( NFile::CFilePath *pFilePath ) = 0;
	virtual bool DataChunkString( string &data ) = 0;
	virtual bool DataChunkString( wstring &data ) = 0;
	//
	virtual void StoreObject( CObjectBase *pObject ) = 0;
	virtual CObjectBase* LoadObject() = 0;
public:
	virtual bool IsReading() const = 0;
	virtual void ReportCurrentObject( const CDBID &dbid ) = 0;
	virtual void PushCurrentObject( const CDBID &dbid ) = 0;
	virtual void PopCurrentObject() = 0;
	// xml node attributes
	virtual bool AddAttribute( const chunk_id attrName, bool *pData ) = 0;
	virtual bool AddAttribute( const chunk_id attrName, int *pData ) = 0;
	virtual bool AddAttribute( const chunk_id attrName, float *pData ) = 0;
	virtual bool AddAttribute( const chunk_id attrName, string *pData ) = 0;
	virtual bool AddAttribute( const chunk_id attrName, wstring *pData ) = 0;
	// add raw data of specified size (in bytes)
	void AddRawData( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber = 0 ) { DataChunk( idChunk, pData, nSize, nChunkNumber ); }
	// main add function - add all structures/classes/datas through it
	template<class T>
		void Add( const chunk_id idChunk, T *p, int nChunkNumber = 0 ) { AddInternal( idChunk, nChunkNumber, p, p ); }
	// adding typed super class - use it only for super class members serialization
	template <class T>
		void AddTypedSuper( T *pData )
		{
			if ( !StartChunk(0, 0) )
				return;
			pData->T::operator&( *this );
			FinishChunk();
		}
	template <class T>
		void AddPolymorphicBase( const chunk_id idChunk, T *pData, int nChunkNumber = 0 )
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			pData->operator&( *this );
			FinishChunk();
		}
	template <class TUserObj, class TRef>
		void AddInPlace( const chunk_id idChunk, CPtrBase<TUserObj, TRef> *pData, int nChunkNumber = 0 )
		{
			if ( !StartChunk(idChunk, nChunkNumber) )
				return;
			if ( IsReading() ) 
			{
				int nTypeID = -1;
				Add( "__ClassTypeID", &nTypeID );
				pData->Set( CastToUserObject( NObjectFactory::MakeObject(nTypeID), (TUserObj*)0 ) ); 
				if ( pData->GetPtr() != 0 )
					Add( "__ObjectData", pData->GetPtr() );
			}
			else
			{
				if ( pData->GetPtr() != 0 )
				{
					int nTypeID = NObjectFactory::GetObjectTypeID( pData->GetPtr() );
					Add( "__ClassTypeID", &nTypeID );
					Add( "__ObjectData", pData->GetPtr() );
				}
			}
			FinishChunk();
		}
};

template <class T>
inline char operator&( T &c, IXmlSaver &ss ) { return 0; }

namespace NXml
{
	struct SPool;
}
SYSTEM_EXPORT IXmlSaver *CreateXmlSaver( CDataStream *pStream, ESaverMode mode );

