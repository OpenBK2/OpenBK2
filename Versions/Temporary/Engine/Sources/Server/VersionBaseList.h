#pragma once

#define MAX_HISTORY_LENGTH 500

template <class TData>
class TVersionBaseList
{
	std::list<TData> nowList;
	std::vector<TData> history;
	std::vector<bool> dataAdded;
	DWORD dwVersion;

public:
	TVersionBaseList();
	void Add( const TData &data );
	void Remove( const TData &data );
	const std::list<TData>& GetNow() const { return nowList; }
	const std::list<TData> GetAddDiff( const DWORD _dwVersion ) const;
	const std::list<TData> GetRemoveDiff( const DWORD _dwVersion ) const;
	const DWORD GetVersion() const { return dwVersion; }
	inline bool NeedFullUpdate( const DWORD _dwVersion ) const; 
};

template< class TData >
TVersionBaseList<TData>::TVersionBaseList() : dwVersion( MAX_HISTORY_LENGTH + 1 )
{
	history = std::vector<TData>( MAX_HISTORY_LENGTH );
	dataAdded = std::vector<bool>( MAX_HISTORY_LENGTH );
}

template< class TData >
void TVersionBaseList<TData>::Add( const TData &data )
{
	++dwVersion;
	nowList.push_back( data );
	history[ dwVersion % MAX_HISTORY_LENGTH ] = data;
	dataAdded[ dwVersion % MAX_HISTORY_LENGTH ] = true;
}

template< class TData >
void TVersionBaseList<TData>::Remove( const TData &data )
{
	++dwVersion;
	nowList.remove( data );
	history[ dwVersion % MAX_HISTORY_LENGTH ] = data;
	dataAdded[ dwVersion % MAX_HISTORY_LENGTH ] = false;
}

template< class TData >
inline bool TVersionBaseList<TData>::NeedFullUpdate( const DWORD _dwVersion ) const
{
	return ( dwVersion + 1 > _dwVersion + MAX_HISTORY_LENGTH );
}

template< class TData >
const std::list<TData> TVersionBaseList<TData>::GetAddDiff( const DWORD _dwVersion ) const
{
	if ( !NeedFullUpdate( _dwVersion ) )
	{
		std::list<TData> result;
		for ( DWORD v = _dwVersion + 1; v <= dwVersion; ++v )
		{
			if ( dataAdded[ v % MAX_HISTORY_LENGTH ] )
				result.push_back( history[ v % MAX_HISTORY_LENGTH ] );
		}
		return result;
	}
	else
	{
		return nowList;
	}
}

template< class TData >
const std::list<TData> TVersionBaseList<TData>::GetRemoveDiff( const DWORD _dwVersion ) const
{
	std::list<TData> result;
	if ( !NeedFullUpdate( _dwVersion ) )
	{
		for ( DWORD v = _dwVersion + 1; v <= dwVersion; ++v )
		{
			if ( !dataAdded[ v % MAX_HISTORY_LENGTH ] )
				result.push_back( history[ v % MAX_HISTORY_LENGTH ] );
		}
	}
	return result;
}

#undef MAX_HISTORY_LENGTH

