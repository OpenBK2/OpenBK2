#include "stdafx.h"

#include "Resources.h"

#include <vector>

namespace
{
	std::vector<HINSTANCE>& Modules()
	{
		static std::vector<HINSTANCE> modules;
		return modules;
	}

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
}


namespace NResources
{
	HINSTANCE FindModule( LPCSTR pszName, LPCSTR pszType )
	{
		const HINSTANCE hExecutable = ::GetModuleHandleA( nullptr );
		if ( ::FindResourceA( hExecutable, pszName, pszType ) != 0 )
		{
			return hExecutable;
		}
		for ( const HINSTANCE hModule : Modules() )
		{
			if ( ::FindResourceA( hModule, pszName, pszType ) != 0 )
			{
				return hModule;
			}
		}
		return 0;
	}


	void RegisterModule( HINSTANCE hModule )
	{
		if ( hModule != 0 )
		{
			Modules().push_back( hModule );
		}
	}


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
