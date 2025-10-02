#pragma once

namespace NDb
{

template<class T>
class CRndInstance
{
	struct SDbElement
	{
		int nFlag;
		float fWeight;
		int nTypeID;
		int nObjectID;
	};

	struct SElement
	{
		int nFlag;
		float fWeight;
		CDBPtr<T> pObj;
	};

	vector<SElement> elements;

	struct SSector
	{
		float fValue;
		int nIndex;

		SSector() : fValue( 0.0f ), nIndex( 0 ) { }
		SSector( int _nIndex, float _fValue ) : fValue( _fValue ), nIndex( _nIndex ) { }
	};

	template<class TGetFloatRandomFunc>
	const T* Get( TGetFloatRandomFunc randomFunc, const vector<int> &filter, bool bWithFilter ) const
	{
		vector<SSector> sectors;
		sectors.reserve( elements.size() );

		for ( int i = 0; i < elements.size(); ++i )
		{
			if ( !bWithFilter || find( filter.begin(), filter.end(), elements[i].nFlag ) != filter.end() )
			{
				if ( sectors.empty() )
					sectors.push_back( SSector( i, elements[i].fWeight ) );
				else
					sectors.push_back( SSector( i, sectors.back().fValue + elements[i].fWeight ) );
			}
		}
		
		if ( sectors.empty() )
			return 0;

		float fValue = randomFunc( 0.0f, sectors.back().fValue );
		
		int nLeft = 0;
		int nRight = sectors.size() - 1;
		while ( nLeft < nRight )
		{
			int nMed = (nLeft + nRight) / 2;
			if ( fValue >= sectors[nMed].fValue )
				nLeft = nMed + 1;
			else
				nRight = nMed;
		}
		NI_VERIFY( nLeft < sectors.size(), "something is wrong", return elements[sectors.back().nIndex].pObj );

		return elements[sectors[nLeft].nIndex].pObj;
	}

public:
	CRndInstance() { }

	template<class TGetFloatRandomFunc>
	const T* Get( TGetFloatRandomFunc randomFunc ) const
	{
		vector<int> filter;
		return Get( randomFunc, filter, false );
	}

	template<class TGetFloatRandomFunc>
	const T* Get( TGetFloatRandomFunc randomFunc, const vector<int> &filter ) const
	{
		return Get( randomFunc, filter, true );
	}
	
	int operator&( IBinSaver &saver )
	{
		if ( saver.IsReading() ) 
		{
			vector<SDbElement> dbElements;
			saver.Add( 1, &dbElements );

			elements.resize( dbElements.size() );
			for ( int i = 0; i < dbElements.size(); ++i )
			{
				elements[i].nFlag = dbElements[i].nFlag;
				elements[i].fWeight = dbElements[i].fWeight;
				if ( elements[i].fWeight < 0.0f )
					elements[i].fWeight = 0.0f;

				elements[i].pObj = 0;
				if ( dbElements[i].nTypeID != 0 && dbElements[i].nObjectID != 0 ) 
					elements[i].pObj = const_cast<T*>( CastToDBUserObject( NDb::GetResource(dbElements[i].nTypeID, dbElements[i].nObjectID), (T*)0 ) );
			}
		}
		else
		{
			vector<SDbElement> dbElements( elements.size() );
			for ( int i = 0; i < elements.size(); ++i )
			{
				dbElements[i].nFlag = elements[i].nFlag;
				dbElements[i].fWeight = elements[i].fWeight;
				if ( elements[i].pObj )
				{
					dbElements[i].nTypeID = CastToDBResource( elements[i].pObj.GetPtr() )->GetTypeID();
					dbElements[i].nObjectID = CastToDBResource( elements[i].pObj.GetPtr() )->GetRecordID();
				}
				else
					dbElements[i].nTypeID = dbElements[i].nObjectID = 0;
			}

			saver.Add( 1, &dbElements );
		}

		return 0;
	}
};



struct SDBRndSector
{
	float fValue;
	int nIndex;

	SDBRndSector() : fValue( 0.0f ), nIndex( 0 ) { }
	SDBRndSector( int _nIndex, float _fValue ) : fValue( _fValue ), nIndex( _nIndex ) { }
};

template<class TRndType, class TGetFloatRandomFunc>
const typename TRndType::TRefType* GetRnd( const TRndType &rndType, TGetFloatRandomFunc randomFunc, const vector<typename TRndType::EFlagType> &filter, bool bWithFilter )
{
	vector<SDBRndSector> sectors;
	sectors.reserve( rndType.elements.size() );

	for ( int i = 0; i < rndType.elements.size(); ++i )
	{
		if ( !bWithFilter || find( filter.begin(), filter.end(), rndType.elements[i].eFlag ) != filter.end() )
		{
			if ( sectors.empty() )
				sectors.push_back( SDBRndSector( i, rndType.elements[i].fWeight ) );
			else
				sectors.push_back( SDBRndSector( i, sectors.back().fValue + rndType.elements[i].fWeight ) );
		}
	}

	if ( sectors.empty() || sectors.back().fValue == 0.0f )
		return 0;

	float fValue = randomFunc( 0.0f, sectors.back().fValue );

	int nLeft = 0;
	int nRight = sectors.size() - 1;
	while ( nLeft < nRight )
	{
		int nMed = (nLeft + nRight) / 2;
		if ( fValue >= sectors[nMed].fValue )
			nLeft = nMed + 1;
		else
			nRight = nMed;
	}
	NI_VERIFY( nLeft < sectors.size(), "something is wrong", return rndType.elements[sectors.back().nIndex].pObj );

	return rndType.elements[sectors[nLeft].nIndex].pObj;
}

template<class TRndType, class TGetFloatRandomFunc>
const typename TRndType::TRefType* GetRnd( const TRndType &rndType, TGetFloatRandomFunc randomFunc )
{
	vector<typename TRndType::EFlagType> filter;
	return GetRnd( rndType, randomFunc, filter, false );	
}

template<class TRndType, class TGetFloatRandomFunc>
const typename TRndType::TRefType* GetRnd( const TRndType &rndType, TGetFloatRandomFunc randomFunc, const vector<typename TRndType::EFlagType> &filter )
{
	return GetRnd( rndType, randomFunc, filter, true );
}

struct SRndReference
{
	void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const { }
};

}

