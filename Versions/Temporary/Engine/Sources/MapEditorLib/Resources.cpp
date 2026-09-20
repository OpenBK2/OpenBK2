#include "stdafx.h"

#include "Resources.h"

#include <vector>

namespace
{
	struct STable
	{
		const NResources::SStringEntry *pEntries;
		size_t nCount;
	};

	// A function-local static, because the tables register themselves from
	// namespace-scope objects in other modules and there is no order between
	// those and a plain global here.
	std::vector<STable>& Tables()
	{
		static std::vector<STable> tables;
		return tables;
	}

	struct SBinaryTable
	{
		const NResources::SBinaryEntry *pEntries;
		size_t nCount;
	};

	std::vector<SBinaryTable>& BinaryTables()
	{
		static std::vector<SBinaryTable> tables;
		return tables;
	}

	// One registry per kind, all the same shape: a module's array of entries,
	// each entry keyed by the id the .rc gave it.
	template <class TEntry>
	struct STableOf
	{
		const TEntry *pEntries;
		size_t nCount;
	};

	template <class TEntry>
	std::vector<STableOf<TEntry>>& TablesOf()
	{
		static std::vector<STableOf<TEntry>> tables;
		return tables;
	}

	template <class TEntry>
	void Register( const TEntry *pEntries, size_t nCount )
	{
		if ( ( pEntries != nullptr ) && ( nCount > 0 ) )
		{
			TablesOf<TEntry>().push_back( STableOf<TEntry>{ pEntries, nCount } );
		}
	}

	template <class TEntry>
	const TEntry* Find( unsigned nID )
	{
		for ( const STableOf<TEntry> &rTable : TablesOf<TEntry>() )
		{
			for ( size_t nEntry = 0; nEntry < rTable.nCount; ++nEntry )
			{
				if ( rTable.pEntries[nEntry].nID == nID )
				{
					return &( rTable.pEntries[nEntry] );
				}
			}
		}
		return nullptr;
	}
}


namespace NResources
{
	CStringTable::CStringTable( const SStringEntry *pEntries, size_t nCount )
	{
		if ( ( pEntries != nullptr ) && ( nCount > 0 ) )
		{
			Tables().push_back( STable{ pEntries, nCount } );
		}
	}


	CBinaryTable::CBinaryTable( const SBinaryEntry *pEntries, size_t nCount )
	{
		if ( ( pEntries != nullptr ) && ( nCount > 0 ) )
		{
			BinaryTables().push_back( SBinaryTable{ pEntries, nCount } );
		}
	}


	bool GetBinaryResource( unsigned nID, const unsigned char **ppData, size_t *pnSize )
	{
		if ( ( ppData == nullptr ) || ( pnSize == nullptr ) )
		{
			return false;
		}
		for ( const SBinaryTable &rTable : BinaryTables() )
		{
			for ( size_t nEntry = 0; nEntry < rTable.nCount; ++nEntry )
			{
				if ( rTable.pEntries[nEntry].nID == nID )
				{
					( *ppData ) = rTable.pEntries[nEntry].pData;
					( *pnSize ) = rTable.pEntries[nEntry].nSize;
					return true;
				}
			}
		}
		return false;
	}


	CMenuTable::CMenuTable( const SMenuEntry *pEntries, size_t nCount )
	{
		Register( pEntries, nCount );
	}


	bool GetMenu( unsigned nID, const SMenuItem **ppItems, size_t *pnCount )
	{
		const SMenuEntry *const pEntry = Find<SMenuEntry>( nID );
		if ( ( pEntry == nullptr ) || ( ppItems == nullptr ) || ( pnCount == nullptr ) )
		{
			return false;
		}
		( *ppItems ) = pEntry->pItems;
		( *pnCount ) = pEntry->nCount;
		return true;
	}


	CAcceleratorTable::CAcceleratorTable( const SAcceleratorTableEntry *pEntries, size_t nCount )
	{
		Register( pEntries, nCount );
	}


	bool GetAccelerators( unsigned nID, const SAcceleratorEntry **ppEntries, size_t *pnCount )
	{
		const SAcceleratorTableEntry *const pEntry = Find<SAcceleratorTableEntry>( nID );
		if ( ( pEntry == nullptr ) || ( ppEntries == nullptr ) || ( pnCount == nullptr ) )
		{
			return false;
		}
		( *ppEntries ) = pEntry->pEntries;
		( *pnCount ) = pEntry->nCount;
		return true;
	}


	CToolBarTable::CToolBarTable( const SToolBarEntry *pEntries, size_t nCount )
	{
		Register( pEntries, nCount );
	}


	const SToolBarEntry* GetToolBar( unsigned nID )
	{
		return Find<SToolBarEntry>( nID );
	}


	bool GetString( unsigned nID, std::string *pszText )
	{
		if ( pszText == nullptr )
		{
			return false;
		}
		pszText->clear();
		// Linear over a couple of hundred entries. The tables are in the order
		// the .rc wrote them, which is not id order, so this is a scan; at this
		// size and called from menu and dialog set-up, that is not worth an
		// index.
		for ( const STable &rTable : Tables() )
		{
			for ( size_t nEntry = 0; nEntry < rTable.nCount; ++nEntry )
			{
				if ( rTable.pEntries[nEntry].nID == nID )
				{
					( *pszText ) = rTable.pEntries[nEntry].pszText;
					return true;
				}
			}
		}
		return false;
	}


	std::string GetString( unsigned nID )
	{
		std::string szText;
		GetString( nID, &szText );
		return szText;
	}
}
