#if !defined(__MAIN_FRAME__PARAMS__)
#define __MAIN_FRAME__PARAMS__
#pragma once


struct SMainFrameParams
{
	//save maximized form
	bool bMaximized;
	CTRect<int> rect;

	SMainFrameParams();

	void GetRegistryKey( string *pszRegistryKey );
	void GetXMLFilePath( string *pszXMLFilePath );

	// serializing...
	int operator&( IBinSaver &bs );
	int operator&( IXmlSaver &xs );

	void ClearRegistry();
	void Load( bool bFromRegistry );
	void Save( bool bToRegistry );
};

#endif // !defined(__MAIN_FRAME__PARAMS__)
