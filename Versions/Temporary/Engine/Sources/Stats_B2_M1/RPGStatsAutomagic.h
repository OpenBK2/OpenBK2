#pragma once

#include "Stats_B2_M1_export.h"

struct IRPGStatsAutomagic : public CObjectBase
{
	enum
	{
		tidTypeID = 0x11078380
	};

public:
	virtual const char *ToStr( const int nVal ) const = 0;
	virtual const int ToInt( const char *pszVal ) const = 0;

	virtual bool IsLastStr( const char *pszVal ) const = 0;
	virtual bool IsLastInt( const int nVal ) const = 0;

	virtual const char *GetFirstStr() const = 0;
	virtual const int GetFirstInt() const = 0;

	virtual const char *GetNextStr( const char *pszVal ) = 0;
	virtual const int GetNextInt( const int nVal ) = 0;
};

class CRPGStatsAutomagic : public IRPGStatsAutomagic
{
	OBJECT_BASIC_METHODS( CRPGStatsAutomagic );

	typedef std::unordered_map<int, std::string> CI2SMap;
	typedef std::unordered_map<std::string, int> CS2IMap;
	//
	CI2SMap i2s;
	CS2IMap s2i;
	std::string szUnknown;
	//
public:
	CRPGStatsAutomagic();

	//
	const char *ToStr( const int nVal ) const override
	{
		CI2SMap::const_iterator it = i2s.find( nVal );
		return it != i2s.end() ? it->second.c_str() : szUnknown.c_str();
	}

	const int ToInt( const char *pszVal ) const override
	{
		CS2IMap::const_iterator it = s2i.find( pszVal );
		return it != s2i.end() ? it->second : -1;
	}

	const char *GetFirstStr() const override;
	const int GetFirstInt() const override;
	bool IsLastStr( const char *pszVal ) const override;
	bool IsLastInt( const int nVal ) const override;
	const char *GetNextStr( const char *pszVal ) override;
	const int GetNextInt( const int nVal ) override;
};

STATS_B2_M1_EXPORT int StatsB2M1LinkCheatFunction();
