#if !defined(__COMMON_DATA_ID__)
#define __COMMON_DATA_ID__
#pragma once


/**
struct STypeID
{
private:
	int nTypeID;
	//
public:
	STypeID() : nTypeID( -1 ) {}
	STypeID( const STypeID &rTypeID ) : nTypeID( rTypeID.nTypeID ) {}
	STypeID& operator=( const STypeID &rTypeID )
	{
		if( &rTypeID != this )
		{
			nTypeID = rTypeID.nTypeID;
		}
		return *this;
	}
	//
	inline bool IsInvalid() const { return nTypeID == ( -1 ); }
	inline void Invalidate() { nTypeID = -1; }
	//
	inline int operator&( IBinSaver &rBS )
	{
		rBS.Add( 1, &nTypeID );	
		return 0;
	}
	//
	inline int operator&( IXmlSaver &rXS )
	{
		rXS.Add( "TypeID", &nTypeID );	
		return 0;
	}
};


struct SRecordID
{
private:
	int nRecordID;
	//
public:
	SRecordID() : nRecordID( -1 ) {}
	SRecordID( const SRecordID &rRecordID ) : nRecordID( rRecordID.nRecordID ) {}
	SRecordID& operator=( const SRecordID &rRecordID )
	{
		if( &rRecordID != this )
		{
			nRecordID = rRecordID.nRecordID;
		}
		return *this;
	}
	//
	inline bool IsInvalid() const { return nRecordID == ( -1 ); }
	inline void Invalidate() { nRecordID = -1; }
	//
	inline int operator&( IBinSaver &rBS )
	{
		rBS.Add( 1, &nRecordID );	
		return 0;
	}
	//
	inline int operator&( IXmlSaver &rXS )
	{
		rXS.Add( "RecordID", &nRecordID );	
		return 0;
	}
};


struct SDataID
{
private:
	STypeID typeID;
	SRecordID recordID;
public:
	SDataID() {}
	SDataID( const STypeID &rTypeID, const SRecordID &rRecordID ) : typeID( rTypeID ), recordID( rRecordID ) {}
	SDataID( const SDataID &rDataID ) : typeID( rDataID.typeID ), recordID( rDataID.recordID ) {}
	SDataID& operator=( const SDataID &rDataID )
	{
		if( &rDataID != this )
		{
			typeID = rDataID.typeID;
			recordID = rDataID.recordID;
		}
		return *this;
	}
	//
	inline bool IsInvalid() const { return ( typeID.IsInvalid() || recordID.IsInvalid() ); }
	inline void Invalidate() { typeID.Invalidate(); recordID.Invalidate(); }
	//
	inline int operator&( IBinSaver &rBS )
	{
		rBS.Add( 1, &typeID );	
		rBS.Add( 2, &recordID );	
		return 0;
	}
	//
	inline int operator&( IXmlSaver &rXS )
	{
		rXS.AddTypedSuper( &typeID );	
		rXS.AddTypedSuper( &recordID );	
		return 0;
	}
};

template <class TID> bool IsInvalid( const TID &rID ) { return rID.IsInvalid(); }
template <> bool IsInvalid( const int &rID ) { return ( rID == ( -1 ) ); }
template <> bool IsInvalid( const string &rID ) { return ( rID.empty() ); }

template <class TID> void Invalidate( TID *pID ) { if ( pID != 0 ) { pID->Invalidate(); } }
template <> void Invalidate( int *pID ) { if ( pID != 0 ) { ( *pID ) = ( -1 ); } }
template <> void Invalidate( string *pID ) { if ( pID != 0 ) { pID->clear(); } }
/**/

#endif // !defined(__COMMON_DATA_ID__)
