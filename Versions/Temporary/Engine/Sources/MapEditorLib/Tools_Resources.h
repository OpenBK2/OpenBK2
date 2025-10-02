#pragma once

#include "../System/VFSOperations.h"

// Legacy
template<class TResource>
bool LoadXMLResource( const string &rszResourceFileName, const string &rszExtention, const string &rszChunkLabel, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadXMLResource( szResourceFileName, rszChunkLabel, rResource );
}

// New Legacy

struct SFileStreamHolder
{
	CDataStream *pStream;

	SFileStreamHolder() : pStream( 0 ) { }
	~SFileStreamHolder()
	{
		delete pStream;
	}
};


void OpenStreamHolder( SFileStreamHolder *pStreamHolder, const string &rszTextPath );
void CreateStreamHolder( SFileStreamHolder *pStreamHolder, const string &rszTextPath );


template<class TResource>
bool LoadXMLResource( const string &rszResourceFileName, const string &rszChunkLabel, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->Add( rszChunkLabel.c_str(), &rResource );
			return true;
		}
	}
	return false;
}


// Legacy
template<class TResource>
bool LoadTypedSuperXMLResource( const string &rszResourceFileName, const string &rszExtention, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadTypedSuperXMLResource( szResourceFileName, rResource );
}


template<class TResource>
bool LoadTypedSuperXMLResource( const string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool LoadBINResource( const string &rszResourceFileName, const string &rszExtention, int nChunkNumber, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadBINResource( szResourceFileName, nChunkNumber, rResource );
}


template<class TResource>
bool LoadBINResource( const string &rszResourceFileName, int nChunkNumber, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->Add( nChunkNumber, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool LoadTypedSuperBINResource( const string &rszResourceFileName, const string &rszExtention, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadTypedSuperBINResource( szResourceFileName, rResource );
}


template<class TResource>
bool LoadTypedSuperBINResource( const string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->AddTypedSuper( 1, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveXMLResource( const string &rszResourceFileName, const string &rszExtention, const string &rszChunkLabel, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveXMLResource( szResourceFileName, rszChunkLabel, rResource );
}


template<class TResource>
bool SaveXMLResource( const string &rszResourceFileName, const string &rszChunkLabel, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( rszChunkLabel.c_str(), &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveTypedSuperXMLResource( const string &rszResourceFileName, const string &rszExtention, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveTypedSuperXMLResource( szResourceFileName, rResource );
}


template<class TResource>
bool SaveTypedSuperXMLResource( const string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveBINResource( const string &rszResourceFileName, const string &rszExtention, int nChunkNumber, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveBINResource( szResourceFileName, nChunkNumber, rResource );
}


template<class TResource>
bool SaveBINResource( const string &rszResourceFileName, int nChunkNumber, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( nChunkNumber, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveTypedSuperBINResource( const string &rszResourceFileName, const string &rszExtention, TResource &rResource )
{
	string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveTypedSuperBINResource( szResourceFileName, rResource );
}


template<class TResource>
bool SaveTypedSuperBINResource( const string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


//bool CheckLatestBINResource( const string &rszResourceFileName, const string &rszXMLExtention, const string &rszBINExtention );


// возвращаемое значение:
// true - путь существует в указанном каталоге
// false - путь не существует в указанном каталоге
//  
// параметры:
// pszPath							- начальное значение пути, также сюда записывается результат
// bFile								- ищем файл или каталог, если каталог, то на конец обязательно прибавляется '\\' (эсли необходимо)
// bExists							- вляет на значение возвращаемое функцией ( true / false )
// bReturnAbsolutePath	- в путь добавляется или отрезается указанный путь
// замечания:
// если путь уже был абсолютным и начало пути не совпадает с указанным, то путь не изменяется
// если путь уже был абсолютным и начало пути совпадает с указанным, то путь изменяется на относительный ( при bReturnAbsolutePath == false )
// если путь был относительным, к нему прибавляется указаанный путь ( при bReturnAbsolutePath == true )
bool NormalizePath( string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, const string &rszPathPrefix, bool *pbAbsolutePath );
bool IsValidFileName( const string &rszFileName, bool bAbsolutePath ); 



struct SEnumFilesInDataStorageParameter
{
	list<string> fileNameList;
	string szPath;
	string szExtention;

	int nPathLength;
	int nExtentionLength;
};


typedef hash_map<string, hash_map<string, UINT> > CEnumFolderMap;
struct SEnumFolderStructureParameter
{
	int nIgnoreFolderCount;
	CEnumFolderMap enumFolderMap;

	SEnumFolderStructureParameter() : nIgnoreFolderCount( 1 ) {}

	bool IsFolderRelative( const string &rszFolder, const string &rszRelativeFolder );
	void SetRelativeFolder( const string &rszFolder, const string &rszRelativeFolder );

	static bool IsFolderRelative( const CEnumFolderMap &rEnumFolderMap, const string &rszFolder, const string &rszRelativeFolder );
	static void SetRelativeFolder( CEnumFolderMap *pEnumFolderMap, const string &rszFolder, const string &rszRelativeFolder );
};


void EnumFilesInDataStorage( vector<SEnumFilesInDataStorageParameter> *pParameters, SEnumFolderStructureParameter *pEnumFolderStructureParameter = 0 );


bool ExecuteProcess( const string &rszCommand, const string &rszCmdLine, const string &rszDirectory, bool bWait );


void Unicode2MBSC( CString *pstrText, const wstring &rwszText, int nCodePage );
void MBSC2Unicode( wstring *pwszText, const CString &rstrText, int nCodePage );


void File2String( CString *pstrText, bool *pbUnicode, const vector<BYTE> &rBuffer, int nCodePage, bool bRemove_0D );
void File2String( CString *pstrText, bool *pbUnicode, const string &rszTextPath, int nCodePage, bool bRemove_0D );
void File2String( string *pszText, bool *pbUnicode, const string &rszTextPath, int nCodePage, bool bRemove_0D );
void File2String( wstring *pwszText, const vector<BYTE> &rBuffer, bool bRemove_0D );
void File2String( wstring *pwszText, const string &rszTextPath, bool bRemove_0D );


void String2File( vector<BYTE> *pBuffer, const CString &rstrText, bool bUnicode, int nCodePage, bool bAdd_0D );
void String2File( const CString &rstrText, bool bUnicode, const string &rszTextPath, int nCodePage, bool bAdd_0D );
void String2File( const string &rszText, bool bUnicode, const string &rszTextPath, int nCodePage, bool bAdd_0D );
void String2File( vector<BYTE> *pBuffer, const wstring &rwszText, bool bAdd_0D );
void String2File( const wstring &rwszText, const string &rszTextPath, bool bAdd_0D );


