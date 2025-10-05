#pragma once


class CStringManager
{
public:
	static void CreateRecentListName( std::string *pszName, const struct SObjectSet &rObjectSet, bool bMainObject );
	static void CreateObjectSet( SObjectSet *pObjectSet, const std::string &rszName, bool bMainObject );
	static void AddToRecentList( const std::string &rszName, bool bMainObject );
	static void RemoveFromRecentList( const std::string &rszName, bool bMainObject );
	static bool		GetStringValueFromString(	const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, const std::string &rszDefaultValue, std::string *pszString );
	static int		GetIntValueFromString(		const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, int nDefaultValue );
	static float	GetFloatValueFromString(	const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, float fDefaultValue );
	static bool		GetBoolValueFromString(		const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, bool bDefaultValue );
	static int NormalizeValue( int nValue, int nStep );
	static int GetPowerPrecision( int nPrercision );
	static void GetTypeAndNameFromRefValue( std::string *pszTypeName, std::string *pszName, const std::string &rszRefValue, char cSeparator, const std::string &rszDefaultTypeName );
	static void GetRefValueFromTypeAndName( std::string *pszRefValue, const std::string &rszTypeName, const std::string &rszName, char cSeparator );
	static void CutFileName( std::string *pszFileName );
	static bool CutFileExtention( std::string *pszFileName );
	static bool CutFileExtention( std::string *pszFileName, const std::string &rszFileExtention );
	static void ExtendFileExtention( std::string *pszFileName, const std::string &rszFileExtention );
	static void ExtendFileExtention( CString *pstrFileName, const CString &rstrFileExtention );
	static std::string GetFloatStringWithPrecision( const float fValue, const int nPrecision );
	static void SplitFileName( std::string *pszFilePath, std::string *pszFileName, std::string *pszFileExtention, const std::string &rszFullFileName );
	static void RemoveDoubleSlashes( std::string *pszFilePath );
	static int Compare( const std::string &rszLeft, const std::string &rszRight, bool bIgnoreCase, bool bIgnoreSlash, bool bSubString );
};


