#pragma once

#include <cstdint>

struct IScriptDictionary : public CObjectBase
{
	enum { tidTypeID = 0x1C191C80 };

	virtual int GetDictionaryCount() = 0;
	virtual void GetKeywords( int nDictionary, vector< string > &vszKeywords ) = 0;
	virtual uint32_t GetKeywordsColor( int nDictionary ) = 0;
};

