#if !defined(__COMMON_TOOLS__STRING_MANAGER__)
#define __COMMON_TOOLS__STRING_MANAGER__
#pragma once


class CStringManager
{
public:
	static void CreateRecentListName( string *pszName, const struct SObjectSet &rObjectSet, bool bMainObject );
	static void CreateObjectSet( SObjectSet *pObjectSet, const string &rszName, bool bMainObject );
	static void AddToRecentList( const string &rszName, bool bMainObject );
	static void RemoveFromRecentList( const string &rszName, bool bMainObject );
	static bool		GetStringValueFromString(	const string &rszString, const string &rszLabel, const int nPos, const string &rszDividers, const string &rszDefaultValue, string *pszString ); 
	static int		GetIntValueFromString(		const string &rszString, const string &rszLabel, const int nPos, const string &rszDividers, int nDefaultValue ); 
	static float	GetFloatValueFromString(	const string &rszString, const string &rszLabel, const int nPos, const string &rszDividers, float fDefaultValue );
	static bool		GetBoolValueFromString(		const string &rszString, const string &rszLabel, const int nPos, const string &rszDividers, bool bDefaultValue ); 
	static int NormalizeValue( int nValue, int nStep );
	static int GetPowerPrecision( int nPrercision );
	static void GetTypeAndNameFromRefValue( string *pszTypeName, string *pszName, const string &rszRefValue, char cSeparator, const string &rszDefaultTypeName );
	static void GetRefValueFromTypeAndName( string *pszRefValue, const string &rszTypeName, const string &rszName, char cSeparator );
	static void CutFileName( string *pszFileName );
	static bool CutFileExtention( string *pszFileName );
	static bool CutFileExtention( string *pszFileName, const string &rszFileExtention );
	static void ExtendFileExtention( string *pszFileName, const string &rszFileExtention );
	static void ExtendFileExtention( CString *pstrFileName, const CString &rstrFileExtention );
	static string GetFloatStringWithPrecision( const float fValue, const int nPrecision );
	static void SplitFileName( string *pszFilePath, string *pszFileName, string *pszFileExtention, const string &rszFullFileName );
	static void RemoveDoubleSlashes( string *pszFilePath );
	static int Compare( const string &rszLeft, const string &rszRight, bool bIgnoreCase, bool bIgnoreSlash, bool bSubString );
};

#endif // #if !defined(__COMMON_TOOLS__STRING_MANAGER__)

