#pragma once

struct IScriptDictionary : public CObjectBase
{
	enum { tidTypeID = 0x1C191C80 };

	virtual int GetDictionaryCount() = 0;
	virtual void GetKeywords( int nDictionary, vector< string > &vszKeywords ) = 0;
	virtual DWORD GetKeywordsColor( int nDictionary ) = 0;
};

