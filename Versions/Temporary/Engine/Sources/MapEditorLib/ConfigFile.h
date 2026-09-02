#pragma once


struct SConfigFile
{
	static const char DIVIDERS[];

	struct SConfigEntry
	{
		std::string szLine;
		std::string szKeyword;
		std::string szParams;
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
		void Load( const std::string &rszLine );
	};
	typedef std::list<SConfigEntry> CConfigEntryList;
	typedef std::list<std::string> CParamsList;
	//
	CConfigEntryList configEntryList;
	//
	// return number of entries loadeed
	bool Empty() { return configEntryList.empty(); }
	void Clear() { configEntryList.clear(); }
	int Load( const std::string &rszFileName );
	void Save( const std::string &rszFileName );
	//
	// true - keyword is present
	bool GetParams( CParamsList *pParamsList, const std::string &rszKeyword, bool bIgnoreCase );
	//
	void AddLine( const std::string &rszLine );
	void AddKeyword( const std::string &rszKeyword, const std::string &rszParams );
	//
	// return number of entries removed
	int RemoveKeyword( const std::string &rszKeyword, bool bIgnoreCase );
};


