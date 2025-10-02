#if !defined(__COMMON_TOOLS__CONFIG_FILE__)
#define __COMMON_TOOLS__CONFIG_FILE__
#pragma once


struct SConfigFile
{
	static const char DIVIDERS[];

	struct SConfigEntry
	{
		string szLine;
		string szKeyword;
		string szParams;
		//
		SConfigEntry() {}
		SConfigEntry( const SConfigEntry &rConfigEntry )
			: szLine( rConfigEntry.szLine ),
				szKeyword( rConfigEntry.szKeyword ),
				szParams( rConfigEntry.szParams ) {}
		SConfigEntry& operator=( const SConfigEntry &rConfigEntry )
		{
			if( &rConfigEntry != this )
			{
				szLine = rConfigEntry.szLine;
				szKeyword = rConfigEntry.szKeyword;
				szParams = rConfigEntry.szParams;
			}
			return *this;
		}
		void Load( const string &rszLine );
	};
	typedef list<SConfigEntry> CConfigEntryList;
	typedef list<string> CParamsList;
	//
	CConfigEntryList configEntryList;
	//
	// return number of entries loadeed
	bool Empty() { return configEntryList.empty(); }
	void Clear() { configEntryList.clear(); }
	int Load( const string &rszFileName );
	void Save( const string &rszFileName );
	//
	// true - keyword is present
	bool GetParams( CParamsList *pParamsList, const string &rszKeyword, bool bIgnoreCase );
	//
	void AddLine( const string &rszLine );
	void AddKeyword( const string &rszKeyword, const string &rszParams );
	//
	// return number of entries removed
	int RemoveKeyword( const string &rszKeyword, bool bIgnoreCase );
};

#endif // !defined(__COMMON_TOOLS__CONFIG_FILE__)
