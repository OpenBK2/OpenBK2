#pragma once

template<class TID>
class CIndexCollector
{
public:
	typedef hash_map<TID, unsigned> CIDToIndexMap;
	//
private:
	TID invalidID;
	CIDToIndexMap idToIndexMap;
public:
	CIndexCollector( const TID &rInvalidID ) : invalidID( rInvalidID ) {}

	const CIDToIndexMap& GetIDToIndexMap() { return idToIndexMap; }
	// медленные операции
	bool Insert( const TID &rID, unsigned nIndex, bool bSearchIndices )
	{
		if ( ( rID == invalidID ) || ( nIndex == INVALID_NODE_ID ) )
		{
			return false;
		}
		typename CIDToIndexMap::iterator posID = idToIndexMap.find( rID );
		if ( posID != idToIndexMap.end() )
		{
			return false;
		}
		if ( bSearchIndices )
		{
			for ( typename CIDToIndexMap::iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
			{
				if ( itID->second >= nIndex )
				{
					++( itID->second );
				}
			}
		}
		idToIndexMap[rID] = nIndex;
		return true;
	}
	//
	bool Remove( const TID &rID, bool bSearchIndices )
	{
		if ( rID == invalidID )
		{
			return false;
		}
		//
		typename CIDToIndexMap::iterator posID = idToIndexMap.find( rID );
		if ( posID == idToIndexMap.end() )
		{
			return false;
		}
		const unsigned nIndex = posID->second;
		idToIndexMap.erase( posID );
		if ( bSearchIndices )
		{
			for ( typename CIDToIndexMap::iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
			{
				if ( itID->second > nIndex )
				{
					--( itID->second );
				}
			}
		}
		return true;
	}
	//
	void Clear()
	{
		idToIndexMap.clear();
	}
	//
	bool Swap( const TID &rID0, const TID &rID1 )
	{
		if ( ( rID0 == invalidID ) || ( rID1 == invalidID ) )
		{
			return false;
		}
		//
		typename CIDToIndexMap::iterator posID0 = idToIndexMap.find( rID0 );
		typename CIDToIndexMap::iterator posID1 = idToIndexMap.find( rID1 );
		if ( ( posID0 == idToIndexMap.end() ) || ( posID1 == idToIndexMap.end() ) )
		{
			return false;
		}
		const unsigned nIndex = posID0->second;
		posID0->second = posID1->second;
		posID1->second = nIndex;
		return true;
	}
	//
	bool Move( const TID &rID, const unsigned nNewIndex, bool bSearchIndices )
	{
		if ( ( rID == invalidID ) || ( nIndex == invalidID ) )
		{
			return false;
		}
		//
		typename CIDToIndexMap::iterator posID = idToIndexMap.find( rID );
		if ( posID == idToIndexMap.end() )
		{
			return false;
		}
		if ( bSearchIndices )
		{
			const unsigned nOldIndex = posID->second;
			if ( nNewIndex < nOldIndex )
			{
				for ( typename CIDToIndexMap::iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
				{
					if ( ( itID->second >= nNewIndex ) && ( itID->second < nOldIndex ) )
					{
						++( itID->second );
					}
				}
			}
			else if ( nNewIndex > nOldIndex )
			{
				for ( typename CIDToIndexMap::iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
				{
					if ( ( itID->second <= nNewIndex ) && ( itID->second > nOldIndex ) )
					{
						--( itID->second );
					}
				}
			}
		}
		posID->second = nNewIndex;
		return true;
	}
	// быстрые операции
	unsigned Get( const TID &rID ) const
	{
		if ( rID == invalidID )
		{
			return invalidID;
		}
		typename CIDToIndexMap::const_iterator posID = idToIndexMap.find( rID );
		if ( posID == idToIndexMap.end() )
		{
			return invalidID;
		}
		else
		{
			return posID->second;
		}
	}
	// медленные операции
	TID GetID( const unsigned nIndex ) const
	{
		for ( typename CIDToIndexMap::const_iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
		{
			if ( itID->second == nIndex )
			{
				return itID->first;
			}
		}
		return invalidID;
	}

	//
	unsigned Size() const
	{
		return idToIndexMap.size();
	}
	// дебажный метод (требует очень много памяти)
	void Trace() const
	{
		int nMaxIndex = 0;
		for ( typename CIDToIndexMap::const_iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
		{
			if ( itID->second > nMaxIndex )
			{
				nMaxIndex = itID->second;
			}
		}
		vector<TID> indices;
		indices.resize( nMaxIndex + 1, invalidID );
		for ( typename CIDToIndexMap::const_iterator itID = idToIndexMap.begin(); itID != idToIndexMap.end(); ++itID )
		{
			indices[itID->second] = itID->first;
		}
		DebugTrace( "index collector, begin" );
		for ( unsigned i = 0; i <= nMaxIndex; ++i )
		{
			if ( indices[i] != invalidID )
			{
				DebugTrace( "index: %d, ID: %d", i, indices[i] );
			}
		}
		DebugTrace( "index collector, end" );
	}
};


